/*
 * fs_preload.c -- LD_PRELOAD filesystem-condition shim for the Ultracopier test suite.
 *
 * GOAL (see ../../CLAUDE.md "override the API with minimal code changes"):
 *   Simulate filesystem conditions (I/O errors, open failures, slow I/O, short
 *   writes) for the REAL shipping ultracopier binary -- WITHOUT touching its C++
 *   source and WITHOUT needing privileged/special real folders (e.g. a read-only
 *   mount, a full disk, or a flaky device). We do this by interposing the libc
 *   symbols that the copy + scan path goes through, via dlsym(RTLD_NEXT, ...).
 *
 * SELECTION: the environment variable UC_FS_SCENARIO (a comma-separated list)
 *   chooses the behaviour. Each rule is "<verb>:<arg>". When unset/empty the shim
 *   is a PURE PASS-THROUGH: every wrapped call forwards to the real libc symbol
 *   with zero behaviour change (important -- it must not perturb non-error tests).
 *
 *   Supported rules:
 *     efail:<path-substr>      write()/pwrite() to a path whose name contains
 *                              <path-substr> fails with -1/EIO (simulate a media
 *                              write error mid-transfer).
 *     openfail:<substr>        open()/open64()/openat() of a path containing
 *                              <substr> fails with -1/EACCES (simulate a file we
 *                              are not allowed to open -- e.g. denied source).
 *     gone:<substr>            open()/open64()/openat() of a path containing
 *                              <substr> fails with -1/ENOENT, WHILE stat/lstat keep
 *                              succeeding -- so the scan still LISTS the file (it
 *                              exists at scan time) but the later transfer cannot
 *                              open its source: models a source file REMOVED after
 *                              being listed. The engine must report "source not
 *                              found", skip that one file, and copy the rest.
 *     slow:<ms>               every read()/write()/pread()/pwrite() sleeps <ms>
 *                              milliseconds before doing the real call (simulate
 *                              slow I/O; useful for back-pressure / progress UI).
 *     shortwrite:<substr>      write()/pwrite() to a path containing <substr>
 *                              returns fewer bytes than requested (returns ~half,
 *                              at least 1) to exercise the partial-write loop.
 *     statfail:<substr>        stat/lstat/fstatat/statx of a matching path fails
 *                              with -1/EACCES (simulate metadata access denial).
 *     readfail:<substr>        read()/pread() of a path containing <substr> fails
 *                              with -1/EIO (a fully unreadable SOURCE file / dead
 *                              sector run -- the whole file cannot be read).
 *     eio_after:<substr>:<n>   read()/pread() of a matching fd succeed until <n>
 *                              cumulative bytes have been read from that fd, then
 *                              every further read returns -1/EIO (a bad sector
 *                              PARTWAY through a SOURCE file -- the readable prefix
 *                              is delivered, then the defect). pread uses its offset.
 *     flaky:<substr>:<n>       the first <n> read attempts on a matching SOURCE path
 *                              fail -1/EIO, then succeed (a transient/recoverable
 *                              sector). The count is tracked PER PATH (not per fd),
 *                              so a close()+reopen()+retry still sees the remaining
 *                              budget and eventually succeeds (exercises Retry).
 *     disconnect:<substr>:<off>:<n>   SOURCE media unplug/replug: read()/pread() succeed
 *                              up to <off> cumulative bytes, then fail -1/EIO for <n>
 *                              attempts (the volume is gone), then succeed (it came back).
 *                              The <n> budget is PER PATH (survives close()+reopen()), so a
 *                              put-to-end retry drains it and the file eventually completes.
 *     wdisconnect:<substr>:<off>:<n>  DEST media unplug/replug: write()/pwrite() succeed up
 *                              to <off> bytes written, then fail -1/EIO for <n> attempts,
 *                              then succeed. Mirror of `disconnect` for the destination side.
 *
 *   READ-ACCOUNTING (independent of UC_FS_SCENARIO): set UC_FS_READLOG_MATCH=<substr> and
 *   UC_FS_READLOG_PATH=<file>; every SUCCESSFUL read of a path containing <substr> is summed
 *   and the total (decimal bytes) is flushed to <file> periodically. Lets a test tell, objec-
 *   tively, whether the engine RE-READ the prefix after a recovered fault: resume => total ~=
 *   file size; restart-from-0 => total ~= file size + (fault-offset * retries).
 *
 *   Examples:
 *     UC_FS_SCENARIO="efail:/dst/"                 -> EIO on writes under /dst/
 *     UC_FS_SCENARIO="slow:5,shortwrite:/dst/big"  -> 5ms-slow I/O + short writes
 *     UC_FS_SCENARIO="openfail:secret.bin"         -> EACCES opening secret.bin
 *     UC_FS_SCENARIO="readfail:/src/bad.iso"       -> EIO reading bad.iso (dead file)
 *     UC_FS_SCENARIO="eio_after:/src/movie:65536"  -> first 64 KiB read OK then EIO
 *     UC_FS_SCENARIO="flaky:/src/photo.jpg:3"      -> 3 read failures then success
 *     UC_FS_SCENARIO="disconnect:movie:1048576:3"  -> source drops at 1 MiB, back after 3
 *     UC_FS_SCENARIO="wdisconnect:backup:1048576:3"-> dest drops at 1 MiB, back after 3
 *
 *   FAILING-DISK / BACKUP USE CASE: readfail/eio_after/flaky fault the SOURCE reads,
 *   modelling a dying HDD being backed up. They let a test prove ultracopier "tries,
 *   works around, and informs": it copies every readable file in full, SKIPs/defers
 *   the unreadable ones (per the fileError policy / fileErrorDialog), retries
 *   recoverable sectors, and never aborts the whole backup, hangs, or crashes.
 *   The async ReadThread reads the source through libc ::read/::pread, so these fire
 *   for the async backend; the io_uring DATA plane cannot be faulted this way (its
 *   reads bypass libc) -- only its metadata/scan path is affected (see SCOPE below).
 *
 * ============================== SCOPE / LIMITATION ==============================
 * This shim catches:
 *   - the ASYNC (pthread) data-plane backend, whose I/O is plain libc
 *     read()/write()/pread()/pwrite()/open()/close(); AND
 *   - the SHARED scan + metadata path used by ALL backends (open/openat for
 *     directory + file handles, lstat/stat/statx, readlink, utimensat, chmod,
 *     mkdir, unlink, rename, symlink).
 *
 * This shim DOES NOT catch the io_uring data-plane I/O. The io_uring backend
 * submits reads/writes through io_uring_enter(2) (SQEs queued in a shared ring),
 * NOT through the libc read()/write() symbols -- so 'efail'/'slow'/'shortwrite'
 * on the DATA bytes will not fire for io_uring transfers. The io_uring backend's
 * *metadata/scan* path (lstat/open/statx/etc.) DOES still go through libc and so
 * IS affected by 'openfail'/'statfail'. To fault-inject io_uring data I/O you must
 * instead use a real fault-injecting block device / FUSE layer (out of scope here).
 * ===============================================================================
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>   /* SYS_gettid, for the OPS-TRACE tid column */
#include <utime.h>
#include <sys/time.h>

/* ------------------------------------------------------------------ */
/* Scenario parsing (parsed once, lazily, on first interposed call).   */
/* ------------------------------------------------------------------ */

#define UC_MAX_RULES 32
#define UC_MAX_ARG   512

enum uc_verb {
    UC_NONE = 0,
    UC_EFAIL,        /* write fails EIO on matching path           */
    UC_OPENFAIL,     /* open fails EACCES on matching path         */
    UC_SLOW,         /* sleep ms in read/write                     */
    UC_SLOWWRITE,    /* sleep ms in write/pwrite ONLY -> the dest lags the source read (I/O-order control) */
    UC_DELAYOPEN,    /* sleep ms before open() of a matching path -> control open ORDERING (I/O-order control:
                      * e.g. delay the SOURCE open so the dest write opens first, deterministically) */
    UC_SHORTWRITE,   /* write returns fewer bytes on matching path */
    UC_WFLIP,        /* write SILENTLY bit-flips the byte at <off> but returns the FULL count --
                      * a content corruption the dest looks complete after; the checksum-verify
                      * re-read must catch it (and the engine remove the corrupt dest). */
    UC_STATFAIL,     /* stat family fails EACCES on matching path  */
    UC_READFAIL,     /* read fails EIO on matching path            */
    UC_EIO_AFTER,    /* read fails EIO after N cumulative bytes/fd  */
    UC_FLAKY,        /* first N read attempts/path fail EIO        */
    UC_GONE,         /* open fails ENOENT (stat OK): removed-after-listing */
    UC_DISCONNECT,   /* SOURCE media: read OK to <offset>, then EIO for <n> attempts, then OK */
    UC_WDISCONNECT,  /* DEST media: write OK to <offset>, then EIO for <n> attempts, then OK */
    UC_DATEFAIL,     /* ONLY the date-set (utimensat/futimens/utime[s]) fails EPERM; data writes OK */
    UC_STATMUT,      /* stat of a matching path returns a MUTATED size/mtime after the Nth call */
    UC_SHORTREAD,    /* read() on matching path returns at most <n> bytes (default 64KiB): forces */
                     /* the engine's short-read REFILL path; data is identical, just chunked     */
    UC_ENOSPC        /* write() on matching path succeeds to <bytes> cumulative, then fails ENOSPC: */
                     /* models the destination filling up partway through a file (dest-full)       */
};

struct uc_rule {
    enum uc_verb verb;
    char arg[UC_MAX_ARG];   /* path-substr, or unused for slow */
    long num;               /* numeric arg (ms for slow; offset for [w]disconnect) */
    long num2;              /* second numeric arg (retry budget for [w]disconnect) */
};

static struct uc_rule g_rules[UC_MAX_RULES];
static int            g_rule_count = 0;
static int            g_parsed     = 0;

/* fd -> remembered pathname, so we can match write()/pwrite()/fstat() by path.
 * Small fixed table; ultracopier keeps few fds open per transfer. On overflow we
 * simply stop remembering (the rule then just won't match that fd -- safe).
 * `nread` tracks cumulative bytes successfully read from this fd, used by the
 * eio_after verb (per-fd, reset on every (re)open since the table entry is
 * re-strdup'd). */
#define UC_MAX_FD 4096
struct uc_fdent {
    char *path;             /* strdup'd absolute-or-relative path, or NULL */
    long long nread;        /* cumulative bytes read from this fd (eio_after) */
    long long nwritten;     /* cumulative bytes written to this fd (wdisconnect) */
};
static struct uc_fdent g_fdtab[UC_MAX_FD];

/* Read-accounting (used by the resume-vs-restart tests to tell, objectively, whether the
 * engine re-read the prefix after a recovered fault). When UC_FS_READLOG_MATCH is set, every
 * SUCCESSFUL read of a path containing that substring adds to g_src_read_total; an atexit
 * destructor writes the total (decimal bytes) to UC_FS_READLOG_PATH. resume => total ~= file
 * size; restart-from-0 => total ~= file size + fault-offset (the prefix re-read). */
static long long       g_src_read_total = 0;
static pthread_mutex_t g_src_read_mtx   = PTHREAD_MUTEX_INITIALIZER;
static const char     *g_readlog_match  = NULL;   /* resolved lazily in uc_parse() */
static int             g_readlog_on      = 0;     /* 1 if UC_FS_READLOG_MATCH is set */

/* path -> remaining flaky failure budget. Keyed by the matched <substr> so that a
 * close()+reopen() of the same logical file keeps draining the SAME budget. The
 * count is initialised lazily from the rule's `num` the first time the path is
 * seen, then decremented on each faulted attempt. Guarded by g_flaky_mtx. */
#define UC_MAX_FLAKY 64
struct uc_flaky {
    char substr[UC_MAX_ARG];   /* the rule's matched substring (the key) */
    long  remaining;           /* failures still owed before reads succeed */
    int   used;
};
static struct uc_flaky  g_flaky[UC_MAX_FLAKY];
static pthread_mutex_t   g_flaky_mtx = PTHREAD_MUTEX_INITIALIZER;

/* STAT-ACCOUNTING (independent of UC_FS_SCENARIO, like read-accounting): set
 * UC_FS_STATLOG_MATCH=<substr> and UC_FS_STATLOG_PATH=<file>; every stat/lstat/statx of a
 * path containing <substr> increments a counter flushed (decimal) to <file>. Lets a coalesce
 * test tell, objectively, whether the engine RE-STATs the source: coalesceSourceStat on =>
 * fewer source stats than off. Counted on entry (the engine "asked"), regardless of result. */
static long long       g_stat_count      = 0;
static long long       g_stat_last_flush = 0;
static pthread_mutex_t g_stat_mtx        = PTHREAD_MUTEX_INITIALIZER;
static const char     *g_statlog_match   = NULL;   /* resolved lazily in uc_parse() */
static int             g_statlog_on      = 0;

/* Per-substr call counter (used by statmut to fire from the Nth call). Same fixed-table
 * shape as g_flaky; keyed by the matched substring so close()+reopen() keep one count. */
#define UC_MAX_COUNTER 64
struct uc_counter {
    char substr[UC_MAX_ARG];
    long long count;
    int  used;
};
static struct uc_counter g_counters[UC_MAX_COUNTER];
static pthread_mutex_t    g_counter_mtx = PTHREAD_MUTEX_INITIALIZER;

/* OPS-TRACE (independent of UC_FS_SCENARIO, like read/stat accounting): set UC_FS_OPTRACE_PATH=<file>
 * to append one line per basic FS operation the engine performs, so a test can verify Ultracopier
 * decomposes each copy into the RIGHT basic ops (no duplicate/forgotten op) in a valid ORDER (open
 * before write, no data op after close, file/folder date after close, ...). See test/lib/optrace.py.
 *
 * Line format (TAB-separated):  <seq>\t<tid>\t<verb>\t<path>\t<a1>\t<a2>\n
 *   verb  a1        a2
 *   OPEN  fd        flags (decimal; Python decodes O_CREAT/O_ACCMODE)
 *   CLOSE fd        -
 *   READ  offset    bytes (actual)      -- SOURCE read (async data plane + all scan reads)
 *   WRITE offset    bytes (actual)      -- DEST write  (async data plane only; io_uring/IOCP bypass libc)
 *   MKDIR rc        mode                -- rc: 0 created, -1 failed (e.g. EEXIST on a merge)
 *   CHMOD mode      rc
 *   UTIME rc        -                   -- set-date on a file OR folder (utimensat/futimens/utime[s])
 *   TRUNC len       rc                  -- (f)truncate: dest pre-size/trim
 *   UNLINK rc       -                   RENAME rc  -    SYMLINK rc  -
 * Only paths we can attribute are logged (an unremembered fd is skipped). Data ops are logged AFTER
 * the real call with the ACTUAL byte count; the offset is the per-fd cumulative-before cursor (== the
 * true file offset for the sequential copy path), or the explicit offset for p{read,write}.
 *
 * Thread-safety: one process-wide append fd (O_APPEND) opened lazily via the REAL libc symbols (so we
 * never re-enter our own wrappers or log our own writes), guarded by g_optrace_mtx together with the
 * monotonic seq counter. Written line-at-a-time; O_APPEND keeps concurrent writers from interleaving. */
static const char     *g_optrace_path = NULL;
static int             g_optrace_on   = 0;
static int             g_optrace_fd   = -1;
static long long       g_optrace_seq  = 0;
static pthread_mutex_t g_optrace_mtx  = PTHREAD_MUTEX_INITIALIZER;

/* Increment and return the new call-count for `substr` (lazily created). On table
 * overflow returns a huge number so statmut still fires (fail-toward-mutating). */
static long long uc_path_count(const char *substr)
{
    long long c = 1LL << 60;
    pthread_mutex_lock(&g_counter_mtx);
    struct uc_counter *slot = NULL;
    for (int i = 0; i < UC_MAX_COUNTER; i++)
        if (g_counters[i].used && strcmp(g_counters[i].substr, substr) == 0) {
            slot = &g_counters[i];
            break;
        }
    if (slot == NULL)
        for (int i = 0; i < UC_MAX_COUNTER; i++)
            if (!g_counters[i].used) {
                slot = &g_counters[i];
                slot->used = 1;
                size_t len = strlen(substr);
                if (len > UC_MAX_ARG - 1)
                    len = UC_MAX_ARG - 1;
                memcpy(slot->substr, substr, len);
                slot->substr[len] = '\0';
                slot->count = 0;
                break;
            }
    if (slot != NULL)
        c = ++slot->count;
    pthread_mutex_unlock(&g_counter_mtx);
    return c;
}

static void uc_parse(void)
{
    if (g_parsed)
        return;
    g_parsed = 1;
    /* Read-accounting is independent of UC_FS_SCENARIO: set even when no fault is injected. */
    g_readlog_match = getenv("UC_FS_READLOG_MATCH");
    g_readlog_on = (g_readlog_match != NULL && g_readlog_match[0] != '\0');
    /* Stat-accounting, likewise independent of UC_FS_SCENARIO. */
    g_statlog_match = getenv("UC_FS_STATLOG_MATCH");
    g_statlog_on = (g_statlog_match != NULL && g_statlog_match[0] != '\0');
    /* OPS-TRACE, likewise independent of UC_FS_SCENARIO. */
    g_optrace_path = getenv("UC_FS_OPTRACE_PATH");
    g_optrace_on = (g_optrace_path != NULL && g_optrace_path[0] != '\0');
    const char *s = getenv("UC_FS_SCENARIO");
    if (s == NULL || s[0] == '\0')
        return;   /* pure pass-through */

    char buf[UC_MAX_RULES * (UC_MAX_ARG + 16)];
    strncpy(buf, s, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *saveptr = NULL;
    for (char *tok = strtok_r(buf, ",", &saveptr);
         tok != NULL && g_rule_count < UC_MAX_RULES;
         tok = strtok_r(NULL, ",", &saveptr))
    {
        /* trim leading spaces */
        while (*tok == ' ')
            tok++;
        char *colon = strchr(tok, ':');
        char *arg = "";
        if (colon != NULL) {
            *colon = '\0';
            arg = colon + 1;
        }
        struct uc_rule *r = &g_rules[g_rule_count];
        r->verb = UC_NONE;
        r->arg[0] = '\0';
        r->num = 0;

        if (strcmp(tok, "efail") == 0) {
            r->verb = UC_EFAIL;
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "openfail") == 0) {
            r->verb = UC_OPENFAIL;
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "gone") == 0) {
            r->verb = UC_GONE;
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "slow") == 0) {
            r->verb = UC_SLOW;
            r->num = strtol(arg, NULL, 10);
        } else if (strcmp(tok, "slowwrite") == 0) {   /* I/O-order control: write-only lag */
            r->verb = UC_SLOWWRITE;
            r->num = strtol(arg, NULL, 10);
        } else if (strcmp(tok, "delayopen") == 0) {   /* I/O-order control: "<substr>:<ms>" delay an open */
            r->verb = UC_DELAYOPEN;
            char *lastcolon = strrchr(arg, ':');
            if (lastcolon != NULL) {
                *lastcolon = '\0';
                r->num = strtol(lastcolon + 1, NULL, 10);
            }
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "shortwrite") == 0) {
            r->verb = UC_SHORTWRITE;
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "wflip") == 0) {
            /* arg is "<substr>:<off>"; split on the LAST colon (see eio_after). */
            r->verb = UC_WFLIP;
            char *lastcolon = strrchr(arg, ':');
            if (lastcolon != NULL) {
                *lastcolon = '\0';
                r->num = strtoll(lastcolon + 1, NULL, 10);
            }
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "statfail") == 0) {
            r->verb = UC_STATFAIL;
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "readfail") == 0) {
            r->verb = UC_READFAIL;
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "eio_after") == 0) {
            /* arg is "<substr>:<bytes>"; split on the LAST colon so a path-substr
             * may itself contain ':'. */
            r->verb = UC_EIO_AFTER;
            char *lastcolon = strrchr(arg, ':');
            if (lastcolon != NULL) {
                *lastcolon = '\0';
                r->num = strtol(lastcolon + 1, NULL, 10);
            }
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "flaky") == 0) {
            /* arg is "<substr>:<n>"; split on the LAST colon (see eio_after). */
            r->verb = UC_FLAKY;
            char *lastcolon = strrchr(arg, ':');
            if (lastcolon != NULL) {
                *lastcolon = '\0';
                r->num = strtol(lastcolon + 1, NULL, 10);
            }
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "datefail") == 0) {
            r->verb = UC_DATEFAIL;
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "statmut") == 0) {
            /* arg is "<substr>:<n>"; split on the LAST colon (see eio_after). The
             * mutation fires from call number (n+1) onward, so n=1 leaves the FIRST
             * stat (the scan's, which coalesce caches) intact and mutates the rest. */
            r->verb = UC_STATMUT;
            char *lastcolon = strrchr(arg, ':');
            if (lastcolon != NULL) {
                *lastcolon = '\0';
                r->num = strtol(lastcolon + 1, NULL, 10);
            }
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "disconnect") == 0 || strcmp(tok, "wdisconnect") == 0) {
            /* arg is "<substr>:<offset>:<retries>" -- a recoverable MID-FILE fault that
             * models a media disconnect at <offset> bytes which reconnects after <retries>
             * faulted attempts. Split off the last two colon fields (so a path-substr may
             * itself contain ':'); num=offset, num2=retries. */
            r->verb = (tok[0] == 'w') ? UC_WDISCONNECT : UC_DISCONNECT;
            char *c2 = strrchr(arg, ':');
            if (c2 != NULL) {
                *c2 = '\0';
                r->num2 = strtol(c2 + 1, NULL, 10);
                char *c1 = strrchr(arg, ':');
                if (c1 != NULL) {
                    *c1 = '\0';
                    r->num = strtol(c1 + 1, NULL, 10);
                }
            }
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "shortread") == 0) {
            /* arg is "<substr>[:<n>]" -- read() on the matched path returns at most <n> bytes
             * (default 64 KiB). Split on the LAST colon for the optional <n> ONLY if it is a
             * pure-decimal field (so a path-substr containing ':' but no count still works). */
            r->verb = UC_SHORTREAD;
            r->num = 65536;
            char *lastcolon = strrchr(arg, ':');
            if (lastcolon != NULL) {
                char *p = lastcolon + 1, *end = NULL;
                long v = strtol(p, &end, 10);
                if (*p != '\0' && end != NULL && *end == '\0') {   /* trailing field is all digits */
                    *lastcolon = '\0';
                    r->num = v;
                }
            }
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else if (strcmp(tok, "enospc") == 0) {
            /* arg is "<substr>:<bytes>" -- write() on the matched path succeeds until <bytes>
             * cumulative have been written, then fails ENOSPC (models the dest filling up
             * partway). Split on the LAST colon (see eio_after); num=bytes. */
            r->verb = UC_ENOSPC;
            char *lastcolon = strrchr(arg, ':');
            if (lastcolon != NULL) {
                *lastcolon = '\0';
                r->num = strtol(lastcolon + 1, NULL, 10);
            }
            strncpy(r->arg, arg, UC_MAX_ARG - 1);
        } else {
            continue;   /* unknown verb: ignore */
        }
        r->arg[UC_MAX_ARG - 1] = '\0';
        g_rule_count++;
    }
}

/* Does any rule of `verb` match the path? Returns the rule, else NULL. */
static const struct uc_rule *uc_match(enum uc_verb verb, const char *path)
{
    uc_parse();
    if (path == NULL)
        return NULL;
    for (int i = 0; i < g_rule_count; i++) {
        if (g_rules[i].verb == verb) {
            if (g_rules[i].arg[0] == '\0' || strstr(path, g_rules[i].arg) != NULL)
                return &g_rules[i];
        }
    }
    return NULL;
}

/* The first 'slow' rule (path-independent), or NULL. */
static const struct uc_rule *uc_slow(void)
{
    uc_parse();
    for (int i = 0; i < g_rule_count; i++)
        if (g_rules[i].verb == UC_SLOW)
            return &g_rules[i];
    return NULL;
}

/* The first 'slowwrite' rule (I/O-order control; path-independent, applied in write/pwrite ONLY), or NULL. */
static const struct uc_rule *uc_slowwrite(void)
{
    uc_parse();
    for (int i = 0; i < g_rule_count; i++)
        if (g_rules[i].verb == UC_SLOWWRITE)
            return &g_rules[i];
    return NULL;
}

static long long g_readlog_last_flush = 0;   /* total at last on-disk flush */

/* Write the current g_src_read_total to UC_FS_READLOG_PATH (O_TRUNC), via the REAL libc
 * symbols (RTLD_NEXT) so we never re-enter our own interposed open/close. Best-effort.
 * Called PERIODICALLY (not just at exit): the test process is killed with SIGTERM by
 * `pkill`, which does NOT run __attribute__((destructor)) -- so a purely at-exit dump
 * would never land. Periodic flushing guarantees the final total (reads stop when the
 * copy completes, well before the kill) is on disk. */
static void uc_readlog_write(void)
{
    if (!g_readlog_on)
        return;
    const char *path = getenv("UC_FS_READLOG_PATH");
    if (path == NULL || path[0] == '\0')
        return;
    int (*real_open_p)(const char *, int, ...) =
        (int (*)(const char *, int, ...))dlsym(RTLD_NEXT, "open");
    ssize_t (*real_write_p)(int, const void *, size_t) =
        (ssize_t (*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");
    int (*real_close_p)(int) = (int (*)(int))dlsym(RTLD_NEXT, "close");
    if (real_open_p == NULL || real_write_p == NULL)
        return;
    int fd = real_open_p(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%lld\n", g_src_read_total);
        if (n > 0)
            (void)real_write_p(fd, buf, (size_t)n);
        if (real_close_p != NULL)
            real_close_p(fd);
    }
}

__attribute__((destructor))
static void uc_readlog_dump(void) { uc_readlog_write(); }

/* Flush g_stat_count to UC_FS_STATLOG_PATH (O_TRUNC) via the REAL libc symbols. Same
 * periodic-flush rationale as uc_readlog_write (pkill SIGTERM skips destructors). */
static void uc_statlog_write(void)
{
    if (!g_statlog_on)
        return;
    const char *path = getenv("UC_FS_STATLOG_PATH");
    if (path == NULL || path[0] == '\0')
        return;
    int (*real_open_p)(const char *, int, ...) =
        (int (*)(const char *, int, ...))dlsym(RTLD_NEXT, "open");
    ssize_t (*real_write_p)(int, const void *, size_t) =
        (ssize_t (*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");
    int (*real_close_p)(int) = (int (*)(int))dlsym(RTLD_NEXT, "close");
    if (real_open_p == NULL || real_write_p == NULL)
        return;
    int fd = real_open_p(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        char buf[32];
        int n = snprintf(buf, sizeof(buf), "%lld\n", g_stat_count);
        if (n > 0)
            (void)real_write_p(fd, buf, (size_t)n);
        if (real_close_p != NULL)
            real_close_p(fd);
    }
}

__attribute__((destructor))
static void uc_statlog_dump(void) { uc_statlog_write(); }

/* Append one OPS-TRACE line for a basic FS op. `path` may be NULL (then nothing is logged: an op we
 * cannot attribute to a file is useless to the validator). Uses the REAL libc symbols so it never
 * re-enters our wrappers (no recursion, and our own log writes never appear in the trace). The append
 * fd + seq counter are guarded by g_optrace_mtx. Best-effort: any failure is silently ignored. */
static void uc_optrace(const char *verb, const char *path, long long a1, long long a2)
{
    if (!g_optrace_on || path == NULL)
        return;
    pthread_mutex_lock(&g_optrace_mtx);
    if (g_optrace_fd < 0) {
        int (*real_open_p)(const char *, int, ...) =
            (int (*)(const char *, int, ...))dlsym(RTLD_NEXT, "open");
        if (real_open_p != NULL)
            g_optrace_fd = real_open_p(g_optrace_path, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (g_optrace_fd < 0) {
            pthread_mutex_unlock(&g_optrace_mtx);
            return;
        }
    }
    ssize_t (*real_write_p)(int, const void *, size_t) =
        (ssize_t (*)(int, const void *, size_t))dlsym(RTLD_NEXT, "write");
    if (real_write_p != NULL) {
        long long seq = ++g_optrace_seq;
        long tid = (long)syscall(SYS_gettid);
        char line[UC_MAX_ARG + 128];
        int n = snprintf(line, sizeof(line), "%lld\t%ld\t%s\t%s\t%lld\t%lld\n",
                         seq, tid, verb, path, a1, a2);
        if (n > 0) {
            if (n > (int)sizeof(line))
                n = (int)sizeof(line);
            (void)real_write_p(g_optrace_fd, line, (size_t)n);
        }
    }
    pthread_mutex_unlock(&g_optrace_mtx);
}

/* Count one stat of `path` (on entry) when stat-accounting is on and the path matches.
 * Flushes every 16 counts so the value survives the SIGTERM kill. */
static void uc_account_stat(const char *path)
{
    uc_parse();   /* uc_account_stat runs BEFORE uc_match in the stat wrappers, so it must
                   * trigger the lazy parse itself or it would miss the first matching stat. */
    if (!g_statlog_on || path == NULL || g_statlog_match == NULL)
        return;
    if (strstr(path, g_statlog_match) != NULL) {
        pthread_mutex_lock(&g_stat_mtx);
        g_stat_count++;
        /* Flush on EVERY increment: the count is small (hundreds), and pkill's SIGTERM skips
         * the destructor, so a coarse periodic flush would QUANTIZE the final value and mask a
         * small coalesce delta (the exact reason on==off appeared at a 16-flush granularity). */
        g_stat_last_flush = g_stat_count;
        uc_statlog_write();
        pthread_mutex_unlock(&g_stat_mtx);
    }
}

/* statmut: if a statmut rule matches `path` AND this is at least its (num+1)th stat call,
 * return 1 (caller should mutate the returned size/mtime). Increments the per-path counter. */
static int uc_statmut_should(const char *path)
{
    const struct uc_rule *r = uc_match(UC_STATMUT, path);
    if (r == NULL)
        return 0;
    long long c = uc_path_count(r->arg);
    return (c > r->num) ? 1 : 0;
}

/* Apply the statmut mutation to a struct stat: grow the size and bump the mtime so the
 * "source changed while cached" condition is unambiguous to the test. */
static void uc_statmut_apply(struct stat *st)
{
    if (st == NULL)
        return;
    st->st_size = st->st_size * 2 + 4096;   /* clearly different from the cached size */
    st->st_mtime += 100000;                 /* clearly newer */
}

static void uc_sleep_ms(long ms)
{
    if (ms <= 0)
        return;
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

/* I/O-order control: if a delayopen rule matches, sleep its <ms> BEFORE the real open() runs. The arg
 * selects WHICH opens: "r" = READ-opens only (O_RDONLY -> the SOURCE), "w" = WRITE-opens only
 * (O_WRONLY/O_RDWR/O_CREAT -> the DESTINATION), anything else = a path-substring match. Lets a test
 * deterministically order opens (e.g. "delayopen:r:10" = "first write open, after 10ms the read open"). */
static void uc_maybe_delay_open(const char *path, int flags)
{
    if (path == NULL)
        return;
    uc_parse();
    const int is_write = (flags & (O_WRONLY | O_RDWR | O_CREAT)) != 0;
    for (int i = 0; i < g_rule_count; i++)
    {
        if (g_rules[i].verb != UC_DELAYOPEN)
            continue;
        const char *a = g_rules[i].arg;
        int match;
        if (strcmp(a, "r") == 0)        match = !is_write;                 /* read-opens (source) */
        else if (strcmp(a, "w") == 0)   match = is_write;                  /* write-opens (dest) */
        else                            match = (strstr(path, a) != NULL); /* path-substring */
        if (match)
        {
            uc_sleep_ms(g_rules[i].num);
            return;
        }
    }
}

static void uc_remember_fd(int fd, const char *path)
{
    if (fd < 0 || fd >= UC_MAX_FD || path == NULL)
        return;
    if (g_fdtab[fd].path != NULL) {
        free(g_fdtab[fd].path);
        g_fdtab[fd].path = NULL;
    }
    g_fdtab[fd].path = strdup(path);
    g_fdtab[fd].nread = 0;
    g_fdtab[fd].nwritten = 0;
}

static const char *uc_fd_path(int fd)
{
    if (fd < 0 || fd >= UC_MAX_FD)
        return NULL;
    return g_fdtab[fd].path;
}

static void uc_forget_fd(int fd)
{
    if (fd < 0 || fd >= UC_MAX_FD)
        return;
    if (g_fdtab[fd].path != NULL) {
        free(g_fdtab[fd].path);
        g_fdtab[fd].path = NULL;
    }
    g_fdtab[fd].nread = 0;
    g_fdtab[fd].nwritten = 0;
}

/* Current cumulative bytes written to this fd (for wdisconnect). 0 if out of range. */
static long long uc_fd_nwritten(int fd)
{
    if (fd < 0 || fd >= UC_MAX_FD)
        return 0;
    return g_fdtab[fd].nwritten;
}

static void uc_fd_nwritten_add(int fd, long long add)
{
    if (fd < 0 || fd >= UC_MAX_FD)
        return;
    g_fdtab[fd].nwritten += add;
}

/* Current cumulative bytes read from this fd (for eio_after). 0 if out of range. */
static long long uc_fd_nread(int fd)
{
    if (fd < 0 || fd >= UC_MAX_FD)
        return 0;
    return g_fdtab[fd].nread;
}

/* Advance the per-fd cumulative-read cursor by `add` bytes (ignored if out of range). */
static void uc_fd_nread_add(int fd, long long add)
{
    if (fd < 0 || fd >= UC_MAX_FD)
        return;
    g_fdtab[fd].nread += add;
}

/* Flaky per-path budget: look up (or lazily create) the entry for `substr`,
 * seeded with `initial`. If it still owes a failure, decrement and return 1
 * (caller should fail this read EIO); otherwise return 0 (read should proceed).
 * Thread-safe via g_flaky_mtx. On table overflow we return 0 (fail-open: the
 * read proceeds -- safer than wedging the backup). */
static int uc_flaky_should_fail(const char *substr, long initial)
{
    int fail = 0;
    pthread_mutex_lock(&g_flaky_mtx);
    struct uc_flaky *slot = NULL;
    for (int i = 0; i < UC_MAX_FLAKY; i++) {
        if (g_flaky[i].used && strcmp(g_flaky[i].substr, substr) == 0) {
            slot = &g_flaky[i];
            break;
        }
    }
    if (slot == NULL) {
        for (int i = 0; i < UC_MAX_FLAKY; i++) {
            if (!g_flaky[i].used) {
                slot = &g_flaky[i];
                slot->used = 1;
                /* substr originates from a rule->arg already bounded to
                 * UC_MAX_ARG-1 and NUL-terminated; copy len+1 so the terminator
                 * comes along and the buffer always stays NUL-terminated. */
                size_t len = strlen(substr);
                if (len > UC_MAX_ARG - 1)
                    len = UC_MAX_ARG - 1;
                memcpy(slot->substr, substr, len);
                slot->substr[len] = '\0';
                slot->remaining = initial;
                break;
            }
        }
    }
    if (slot != NULL && slot->remaining > 0) {
        slot->remaining--;
        fail = 1;
    }
    pthread_mutex_unlock(&g_flaky_mtx);
    return fail;
}

/* ------------------------------------------------------------------ */
/* Real symbol resolution (RTLD_NEXT). Each looked up once, cached.    */
/* ------------------------------------------------------------------ */

#define UC_REAL(name) \
    static __typeof__(name) *real_##name = NULL; \
    if (real_##name == NULL) \
        real_##name = (__typeof__(name) *)dlsym(RTLD_NEXT, #name);

/* ------------------------------------------------------------------ */
/* open family                                                         */
/* ------------------------------------------------------------------ */

int open(const char *pathname, int flags, ...)
{
    UC_REAL(open);
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap; va_start(ap, flags); mode = (mode_t)va_arg(ap, int); va_end(ap);
    }
    uc_maybe_delay_open(pathname, flags);   /* I/O-order control: deterministic open ordering */
    if (uc_match(UC_OPENFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    if (uc_match(UC_GONE, pathname) != NULL) {
        errno = ENOENT;     /* listed by the scan (stat OK) but gone at transfer-open */
        return -1;
    }
    int fd = real_open(pathname, flags, mode);
    if (fd >= 0) {
        uc_remember_fd(fd, pathname);
        uc_optrace("OPEN", pathname, fd, flags);
    }
    return fd;
}

int open64(const char *pathname, int flags, ...)
{
    UC_REAL(open64);
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap; va_start(ap, flags); mode = (mode_t)va_arg(ap, int); va_end(ap);
    }
    uc_maybe_delay_open(pathname, flags);   /* I/O-order control: deterministic open ordering */
    if (uc_match(UC_OPENFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    if (uc_match(UC_GONE, pathname) != NULL) {
        errno = ENOENT;     /* listed by the scan (stat OK) but gone at transfer-open */
        return -1;
    }
    int fd = real_open64(pathname, flags, mode);
    if (fd >= 0) {
        uc_remember_fd(fd, pathname);
        uc_optrace("OPEN", pathname, fd, flags);
    }
    return fd;
}

int openat(int dirfd, const char *pathname, int flags, ...)
{
    UC_REAL(openat);
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap; va_start(ap, flags); mode = (mode_t)va_arg(ap, int); va_end(ap);
    }
    uc_maybe_delay_open(pathname, flags);   /* I/O-order control: deterministic open ordering */
    if (uc_match(UC_OPENFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    if (uc_match(UC_GONE, pathname) != NULL) {
        errno = ENOENT;     /* listed by the scan (stat OK) but gone at transfer-open */
        return -1;
    }
    int fd = real_openat(dirfd, pathname, flags, mode);
    if (fd >= 0) {
        uc_remember_fd(fd, pathname);
        uc_optrace("OPEN", pathname, fd, flags);
    }
    return fd;
}

int close(int fd)
{
    UC_REAL(close);
    uc_optrace("CLOSE", uc_fd_path(fd), fd, 0);   /* log with the path BEFORE we forget the fd */
    uc_forget_fd(fd);
    return real_close(fd);
}

/* ------------------------------------------------------------------ */
/* read / write data plane (ASYNC backend + scan reads)                */
/* ------------------------------------------------------------------ */

/* Shared SOURCE-read fault decision for read()/pread().
 * `cum_before` is the bytes already read from this fd at offset 0 (for read(), the
 * running per-fd counter; for pread(), the explicit offset argument). Returns:
 *    -1  -> the caller must fail this read with errno=EIO
 *     0  -> proceed with the real read of `count` bytes
 *   >0   -> proceed, but only read this many bytes (a partial read up to the bad
 *           sector boundary, so the readable prefix is delivered before the EIO).
 * `*do_count` receives the >0 clamp when returned. */
static int uc_read_fault(const char *path, size_t count, long long cum_before,
                         size_t *do_count)
{
    if (path == NULL)
        return 0;
    /* readfail: whole file unreadable. */
    if (uc_match(UC_READFAIL, path) != NULL) {
        errno = EIO;
        return -1;
    }
    /* flaky: first N attempts on this path fail, then succeed. */
    const struct uc_rule *fl = uc_match(UC_FLAKY, path);
    if (fl != NULL) {
        if (uc_flaky_should_fail(fl->arg, fl->num)) {
            errno = EIO;
            return -1;
        }
    }
    /* eio_after: reads succeed up to N cumulative bytes, then fail. If this read
     * straddles the boundary, deliver the readable prefix first (clamp), and let
     * the NEXT read (now fully past the boundary) be the one that returns EIO. */
    const struct uc_rule *ea = uc_match(UC_EIO_AFTER, path);
    if (ea != NULL) {
        long long limit = ea->num;
        if (cum_before >= limit) {
            errno = EIO;
            return -1;
        }
        long long room = limit - cum_before;
        if ((long long)count > room) {
            *do_count = (size_t)room;
            return (int)1;
        }
    }
    /* disconnect: reads succeed up to <offset>, then EIO for <retries> attempts (the
     * media is "unplugged"), then succeed again (reconnected). Models a USB/network
     * SOURCE vanishing mid-file and coming back -- the engine must recover the file. */
    const struct uc_rule *dc = uc_match(UC_DISCONNECT, path);
    if (dc != NULL) {
        long long off = dc->num;
        if (cum_before >= off) {
            /* at/past the disconnect point: drain the per-path retry budget */
            if (uc_flaky_should_fail(dc->arg, dc->num2)) {
                errno = EIO;
                return -1;
            }
            /* budget spent -> reconnected, read proceeds normally */
        } else {
            long long room = off - cum_before;
            if ((long long)count > room) {
                *do_count = (size_t)room;   /* deliver the pre-disconnect prefix first */
                return (int)1;
            }
        }
    }
    return 0;
}

/* Charge `n` successfully-read bytes of `path` to the global source-read accumulator
 * (only when read-accounting is on AND the path matches UC_FS_READLOG_MATCH). */
static void uc_account_read(const char *path, ssize_t n)
{
    if (n <= 0 || !g_readlog_on || path == NULL || g_readlog_match == NULL)
        return;
    if (strstr(path, g_readlog_match) != NULL) {
        pthread_mutex_lock(&g_src_read_mtx);
        g_src_read_total += (long long)n;
        /* Flush every 256 KiB of progress so the value survives a SIGTERM kill. */
        if (g_src_read_total - g_readlog_last_flush >= 256 * 1024) {
            g_readlog_last_flush = g_src_read_total;
            uc_readlog_write();
        }
        pthread_mutex_unlock(&g_src_read_mtx);
    }
}

ssize_t read(int fd, void *buf, size_t count)
{
    UC_REAL(read);
    const struct uc_rule *slow = uc_slow();
    if (slow != NULL)
        uc_sleep_ms(slow->num);
    const char *path = uc_fd_path(fd);
    long long roff = uc_fd_nread(fd);       /* file offset BEFORE this read (== cumulative-so-far) */
    if (path != NULL) {
        size_t do_count = count;
        int dec = uc_read_fault(path, count, roff, &do_count);
        if (dec < 0)
            return -1;                      /* errno already EIO */
        if (dec > 0)
            count = do_count;               /* clamp to the bad-sector boundary */
        const struct uc_rule *sr = uc_match(UC_SHORTREAD, path);
        if (sr != NULL && sr->num > 0 && (long long)count > sr->num)
            count = (size_t)sr->num;        /* force a short read -> engine refill path */
    }
    ssize_t n = real_read(fd, buf, count);
    if (n > 0)
        uc_fd_nread_add(fd, (long long)n);  /* advance the per-fd cursor */
    uc_account_read(path, n);
    if (n > 0)
        uc_optrace("READ", path, roff, (long long)n);
    return n;
}

/* Shared DEST-write disconnect decision for write()/pwrite(). `cum_before` is the bytes
 * already written at offset 0 (per-fd running counter for write(), the explicit offset for
 * pwrite()). Returns -1 (caller fails EIO -- still disconnected) or a possibly-reduced count
 * via *do_count (clamp to the pre-disconnect boundary). 0 means proceed with full count. */
static int uc_write_fault(const char *path, size_t count, long long cum_before,
                          size_t *do_count)
{
    const struct uc_rule *wd = uc_match(UC_WDISCONNECT, path);
    if (wd != NULL) {
        long long off = wd->num;
        if (cum_before >= off) {
            if (uc_flaky_should_fail(wd->arg, wd->num2)) {
                errno = EIO;
                return -1;
            }
        } else {
            long long room = off - cum_before;
            if ((long long)count > room) {
                *do_count = (size_t)room;
                return 1;
            }
        }
    }
    return 0;
}

/* Shared DEST-full (ENOSPC) decision for write()/pwrite(). `cum_before` is the bytes already
 * written before this call (per-fd counter for write(), the explicit offset for pwrite()).
 * Returns -1 (caller fails ENOSPC -- already at/over the cap), 1 with a reduced *do_count (write
 * only the bytes that still fit; the NEXT call then fails ENOSPC), or 0 (proceed with full count). */
static int uc_enospc_fault(const char *path, size_t count, long long cum_before, size_t *do_count)
{
    const struct uc_rule *ns = uc_match(UC_ENOSPC, path);
    if (ns != NULL) {
        long long cap = ns->num;
        if (cum_before >= cap) {
            errno = ENOSPC;
            return -1;
        }
        long long room = cap - cum_before;
        if ((long long)count > room) {
            *do_count = (size_t)room;
            return 1;
        }
    }
    return 0;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    UC_REAL(write);
    const char *path = uc_fd_path(fd);
    const struct uc_rule *slow = uc_slow();
    if (slow != NULL)
        uc_sleep_ms(slow->num);
    const struct uc_rule *sloww = uc_slowwrite();   /* I/O-order control: write-only lag fills the buffer */
    if (sloww != NULL)
        uc_sleep_ms(sloww->num);
    if (path != NULL) {
        if (uc_match(UC_EFAIL, path) != NULL) {
            errno = EIO;
            return -1;
        }
        size_t es_count = count;
        int es = uc_enospc_fault(path, count, uc_fd_nwritten(fd), &es_count);
        if (es < 0)
            return -1;                      /* errno ENOSPC: destination full */
        if (es > 0)
            count = es_count;               /* write only what still fits; next call fails ENOSPC */
        size_t do_count = count;
        int dec = uc_write_fault(path, count, uc_fd_nwritten(fd), &do_count);
        if (dec < 0)
            return -1;                      /* errno already EIO (still disconnected) */
        if (dec > 0)
            count = do_count;               /* clamp to the disconnect boundary */
        const struct uc_rule *sw = uc_match(UC_SHORTWRITE, path);
        if (sw != NULL && count > 1) {
            size_t half = count / 2;
            if (half < 1)
                half = 1;
            long long woff = uc_fd_nwritten(fd);
            ssize_t w = real_write(fd, buf, half);
            if (w > 0) {
                uc_fd_nwritten_add(fd, (long long)w);
                uc_optrace("WRITE", path, woff, (long long)w);
            }
            return w;
        }
        const struct uc_rule *wf = uc_match(UC_WFLIP, path);
        if (wf != NULL && count > 0) {
            long long base = uc_fd_nwritten(fd);
            if (wf->num >= base && wf->num < base + (long long)count) {
                char *tmp = malloc(count);
                if (tmp != NULL) {
                    memcpy(tmp, buf, count);
                    tmp[(size_t)(wf->num - base)] ^= 0xFF;   /* SILENT 1-byte corruption */
                    ssize_t w = real_write(fd, tmp, count);
                    free(tmp);
                    if (w > 0) {
                        uc_fd_nwritten_add(fd, (long long)w);
                        uc_optrace("WRITE", path, base, (long long)count);
                    }
                    return (w < 0) ? w : (ssize_t)count;     /* report the FULL count */
                }
            }
        }
    }
    long long woff = uc_fd_nwritten(fd);
    ssize_t w = real_write(fd, buf, count);
    if (w > 0) {
        uc_fd_nwritten_add(fd, (long long)w);
        uc_optrace("WRITE", path, woff, (long long)w);
    }
    return w;
}

ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
    UC_REAL(pread);
    const struct uc_rule *slow = uc_slow();
    if (slow != NULL)
        uc_sleep_ms(slow->num);
    const char *path = uc_fd_path(fd);
    if (path != NULL) {
        /* pread is positioned: the cumulative-so-far for eio_after is the OFFSET. */
        size_t do_count = count;
        int dec = uc_read_fault(path, count, (long long)offset, &do_count);
        if (dec < 0)
            return -1;                      /* errno already EIO */
        if (dec > 0)
            count = do_count;               /* clamp to the bad-sector boundary */
        const struct uc_rule *sr = uc_match(UC_SHORTREAD, path);
        if (sr != NULL && sr->num > 0 && (long long)count > sr->num)
            count = (size_t)sr->num;        /* force a short read -> engine refill path */
    }
    ssize_t n = real_pread(fd, buf, count, offset);
    uc_account_read(path, n);
    if (n > 0)
        uc_optrace("READ", path, (long long)offset, (long long)n);
    return n;
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
    UC_REAL(pwrite);
    const char *path = uc_fd_path(fd);
    const struct uc_rule *slow = uc_slow();
    if (slow != NULL)
        uc_sleep_ms(slow->num);
    if (path != NULL) {
        if (uc_match(UC_EFAIL, path) != NULL) {
            errno = EIO;
            return -1;
        }
        /* pwrite is positioned: the cumulative-so-far for wdisconnect/enospc is the OFFSET. */
        size_t es_count = count;
        int es = uc_enospc_fault(path, count, (long long)offset, &es_count);
        if (es < 0)
            return -1;                      /* errno ENOSPC: destination full */
        if (es > 0)
            count = es_count;               /* write only what still fits; next call fails ENOSPC */
        size_t do_count = count;
        int dec = uc_write_fault(path, count, (long long)offset, &do_count);
        if (dec < 0)
            return -1;                      /* errno already EIO */
        if (dec > 0)
            count = do_count;               /* clamp to the disconnect boundary */
        const struct uc_rule *sw = uc_match(UC_SHORTWRITE, path);
        if (sw != NULL && count > 1) {
            size_t half = count / 2;
            if (half < 1)
                half = 1;
            return real_pwrite(fd, buf, half, offset);
        }
        const struct uc_rule *wf = uc_match(UC_WFLIP, path);
        if (wf != NULL && count > 0 && wf->num >= (long long)offset
            && wf->num < (long long)offset + (long long)count) {
            char *tmp = malloc(count);
            if (tmp != NULL) {
                memcpy(tmp, buf, count);
                tmp[(size_t)(wf->num - (long long)offset)] ^= 0xFF;   /* SILENT 1-byte corruption */
                ssize_t w = real_pwrite(fd, tmp, count, offset);
                free(tmp);
                if (w > 0)
                    uc_optrace("WRITE", path, (long long)offset, (long long)count);
                return (w < 0) ? w : (ssize_t)count;                  /* report the FULL count */
            }
        }
    }
    ssize_t w = real_pwrite(fd, buf, count, offset);
    if (w > 0)
        uc_optrace("WRITE", path, (long long)offset, (long long)w);
    return w;
}

/* ------------------------------------------------------------------ */
/* stat family (metadata / scan path, shared by ALL backends)          */
/*                                                                     */
/* glibc exposes stat/lstat/fstat as inline wrappers around the        */
/* __xstat/__lxstat/__fxstat ABI symbols on older glibc, and as direct */
/* stat/lstat/fstat exports plus statx() on newer glibc. We interpose  */
/* both families so we work across glibc versions.                     */
/* ------------------------------------------------------------------ */

int stat(const char *pathname, struct stat *statbuf)
{
    UC_REAL(stat);
    uc_account_stat(pathname);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    int rc = real_stat(pathname, statbuf);
    if (rc == 0 && uc_statmut_should(pathname))
        uc_statmut_apply(statbuf);
    return rc;
}

int lstat(const char *pathname, struct stat *statbuf)
{
    UC_REAL(lstat);
    uc_account_stat(pathname);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    int rc = real_lstat(pathname, statbuf);
    if (rc == 0 && uc_statmut_should(pathname))
        uc_statmut_apply(statbuf);
    return rc;
}

int fstat(int fd, struct stat *statbuf)
{
    UC_REAL(fstat);
    const char *path = uc_fd_path(fd);
    uc_account_stat(path);
    if (path != NULL && uc_match(UC_STATFAIL, path) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real_fstat(fd, statbuf);
}

int fstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags)
{
    UC_REAL(fstatat);
    uc_account_stat(pathname);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    int rc = real_fstatat(dirfd, pathname, statbuf, flags);
    if (rc == 0 && uc_statmut_should(pathname))
        uc_statmut_apply(statbuf);
    return rc;
}

/* Older-glibc ABI symbols. `ver` is the stat struct version (_STAT_VER). */
int __xstat(int ver, const char *pathname, struct stat *statbuf)
{
    UC_REAL(__xstat);
    uc_account_stat(pathname);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    int rc = real___xstat(ver, pathname, statbuf);
    if (rc == 0 && uc_statmut_should(pathname))
        uc_statmut_apply(statbuf);
    return rc;
}

int __lxstat(int ver, const char *pathname, struct stat *statbuf)
{
    UC_REAL(__lxstat);
    uc_account_stat(pathname);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    int rc = real___lxstat(ver, pathname, statbuf);
    if (rc == 0 && uc_statmut_should(pathname))
        uc_statmut_apply(statbuf);
    return rc;
}

int __fxstat(int ver, int fd, struct stat *statbuf)
{
    UC_REAL(__fxstat);
    const char *path = uc_fd_path(fd);
    uc_account_stat(path);
    if (path != NULL && uc_match(UC_STATFAIL, path) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real___fxstat(ver, fd, statbuf);
}

int __fxstatat(int ver, int dirfd, const char *pathname, struct stat *statbuf, int flags)
{
    UC_REAL(__fxstatat);
    uc_account_stat(pathname);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    int rc = real___fxstatat(ver, dirfd, pathname, statbuf, flags);
    if (rc == 0 && uc_statmut_should(pathname))
        uc_statmut_apply(statbuf);
    return rc;
}

/* statx(2): the modern path Qt/glibc may use for metadata. */
int statx(int dirfd, const char *pathname, int flags, unsigned int mask,
          struct statx *statxbuf)
{
    UC_REAL(statx);
    uc_account_stat(pathname);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    int rc = real_statx(dirfd, pathname, flags, mask, statxbuf);
    if (rc == 0 && uc_statmut_should(pathname)) {
        statxbuf->stx_size = statxbuf->stx_size * 2 + 4096;
        statxbuf->stx_mtime.tv_sec += 100000;
    }
    return rc;
}

/* The *64 / LFS variants. ultracopier builds with _FILE_OFFSET_BITS=64
 * (CopyEngine.pro), so its stat()/lstat()/fstat() calls -- and Qt's QFileSystemEngine --
 * bind to these GLIBC_2.33 symbols, NOT the plain stat/__xstat above. The stat verbs
 * (statfail/statlog/statmut) MUST interpose them too, or they silently never fire (this is
 * exactly why the source-stat count came back empty). Confirmed via `nm -D ultracopier`:
 * it imports stat64/lstat64/fstat64@GLIBC_2.33. */
int stat64(const char *pathname, struct stat64 *statbuf)
{
    UC_REAL(stat64);
    uc_account_stat(pathname);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    int rc = real_stat64(pathname, statbuf);
    if (rc == 0 && uc_statmut_should(pathname)) {
        statbuf->st_size = statbuf->st_size * 2 + 4096;
        statbuf->st_mtime += 100000;
    }
    return rc;
}

int lstat64(const char *pathname, struct stat64 *statbuf)
{
    UC_REAL(lstat64);
    uc_account_stat(pathname);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    int rc = real_lstat64(pathname, statbuf);
    if (rc == 0 && uc_statmut_should(pathname)) {
        statbuf->st_size = statbuf->st_size * 2 + 4096;
        statbuf->st_mtime += 100000;
    }
    return rc;
}

int fstat64(int fd, struct stat64 *statbuf)
{
    UC_REAL(fstat64);
    const char *path = uc_fd_path(fd);
    uc_account_stat(path);
    if (path != NULL && uc_match(UC_STATFAIL, path) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real_fstat64(fd, statbuf);
}

int fstatat64(int dirfd, const char *pathname, struct stat64 *statbuf, int flags)
{
    UC_REAL(fstatat64);
    uc_account_stat(pathname);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    int rc = real_fstatat64(dirfd, pathname, statbuf, flags);
    if (rc == 0 && uc_statmut_should(pathname)) {
        statbuf->st_size = statbuf->st_size * 2 + 4096;
        statbuf->st_mtime += 100000;
    }
    return rc;
}

/* ------------------------------------------------------------------ */
/* directory + namespace ops (scan / collision / post-ops path)        */
/* These are pass-through today but interposed so a future scenario    */
/* can fail them (e.g. mkdirfail) with a one-line addition.            */
/* ------------------------------------------------------------------ */

int mkdir(const char *pathname, mode_t mode)
{
    UC_REAL(mkdir);
    if (uc_match(UC_OPENFAIL, pathname) != NULL) {
        /* reuse openfail to also deny directory creation when asked */
        errno = EACCES;
        return -1;
    }
    int rc = real_mkdir(pathname, mode);
    uc_optrace("MKDIR", pathname, rc, (long long)mode);
    return rc;
}

int unlink(const char *pathname)
{
    UC_REAL(unlink);
    int rc = real_unlink(pathname);
    uc_optrace("UNLINK", pathname, rc, 0);
    return rc;
}

int rmdir(const char *pathname)
{
    UC_REAL(rmdir);
    int rc = real_rmdir(pathname);
    uc_optrace("RMDIR", pathname, rc, 0);
    return rc;
}

int rename(const char *oldpath, const char *newpath)
{
    UC_REAL(rename);
    int rc = real_rename(oldpath, newpath);
    uc_optrace("RENAME", newpath, rc, 0);   /* logged on the DEST side (the path that appears) */
    return rc;
}

int symlink(const char *target, const char *linkpath)
{
    UC_REAL(symlink);
    int rc = real_symlink(target, linkpath);
    uc_optrace("SYMLINK", linkpath, rc, 0);
    return rc;
}

/* (f)truncate: dest pre-size / trim (e.g. destTruncate, corrupt-dest cleanup). Interposed so the
 * op trace sees the resize; pass-through otherwise. */
int truncate(const char *pathname, off_t length)
{
    UC_REAL(truncate);
    int rc = real_truncate(pathname, length);
    uc_optrace("TRUNC", pathname, (long long)length, rc);
    return rc;
}

int ftruncate(int fd, off_t length)
{
    UC_REAL(ftruncate);
    int rc = real_ftruncate(fd, length);
    uc_optrace("TRUNC", uc_fd_path(fd), (long long)length, rc);
    return rc;
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz)
{
    UC_REAL(readlink);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real_readlink(pathname, buf, bufsiz);
}

/* ------------------------------------------------------------------ */
/* post-transfer metadata ops (keepDate / doRight)                     */
/* ------------------------------------------------------------------ */

int utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags)
{
    UC_REAL(utimensat);
    if (uc_match(UC_DATEFAIL, pathname) != NULL) {
        errno = EPERM;          /* ONLY the date-set fails; the data write already succeeded */
        return -1;
    }
    if (uc_match(UC_EFAIL, pathname) != NULL) {
        errno = EIO;
        return -1;
    }
    int rc = real_utimensat(dirfd, pathname, times, flags);
    uc_optrace("UTIME", pathname, rc, 0);
    return rc;
}

/* futimens(): the pipelined backends (io_uring/IOCP) set the date on the STILL-OPEN
 * destination fd via futimens -- which is libc, so datefail reaches them too. */
int futimens(int fd, const struct timespec times[2])
{
    UC_REAL(futimens);
    const char *path = uc_fd_path(fd);
    if (path != NULL && uc_match(UC_DATEFAIL, path) != NULL) {
        errno = EPERM;
        return -1;
    }
    if (path != NULL && uc_match(UC_EFAIL, path) != NULL) {
        errno = EIO;
        return -1;
    }
    int rc = real_futimens(fd, times);
    uc_optrace("UTIME", path, rc, 0);
    return rc;
}

/* utime()/utimes(): the async backend's MkPath date-set path uses utime(). */
int utime(const char *pathname, const struct utimbuf *times)
{
    UC_REAL(utime);
    if (uc_match(UC_DATEFAIL, pathname) != NULL) {
        errno = EPERM;
        return -1;
    }
    if (uc_match(UC_EFAIL, pathname) != NULL) {
        errno = EIO;
        return -1;
    }
    int rc = real_utime(pathname, times);
    uc_optrace("UTIME", pathname, rc, 0);
    return rc;
}

int utimes(const char *pathname, const struct timeval times[2])
{
    UC_REAL(utimes);
    if (uc_match(UC_DATEFAIL, pathname) != NULL) {
        errno = EPERM;
        return -1;
    }
    if (uc_match(UC_EFAIL, pathname) != NULL) {
        errno = EIO;
        return -1;
    }
    int rc = real_utimes(pathname, times);
    uc_optrace("UTIME", pathname, rc, 0);
    return rc;
}

int chmod(const char *pathname, mode_t mode)
{
    UC_REAL(chmod);
    if (uc_match(UC_EFAIL, pathname) != NULL) {
        errno = EIO;
        return -1;
    }
    int rc = real_chmod(pathname, mode);
    uc_optrace("CHMOD", pathname, (long long)mode, rc);
    return rc;
}

/* fallocate(): the io_uring/IOCP backends preallocate a fresh destination's full extent to reduce
 * fragmentation. io_uring calls the libc fallocate() (its ring reads/writes bypass libc, but this
 * metadata op does not), so the OPS-TRACE sees it -- letting preallocate.py assert preallocation
 * engaged for large files and is size-gated away from small ones. Pure pass-through + log. */
/* Trace a fallocate on `fd`. io_uring opens the destination via the ring (io_uring_prep_openat), NOT
 * our open() wrapper, so the fd is absent from g_fdtab -- resolve its REAL path from /proc/self/fd/<fd>
 * (the ring fd is a normal kernel fd) so the preallocation is still attributable to a file. */
static void uc_falloc_trace(int fd, long long len, int rc)
{
    const char *p = uc_fd_path(fd);
    char procbuf[UC_MAX_ARG];
    if (p == NULL) {
        char link[64];
        snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);
        ssize_t (*real_readlink_p)(const char *, char *, size_t) =
            (ssize_t (*)(const char *, char *, size_t))dlsym(RTLD_NEXT, "readlink");
        ssize_t n = (real_readlink_p != NULL) ? real_readlink_p(link, procbuf, sizeof(procbuf) - 1) : -1;
        if (n > 0) { procbuf[n] = '\0'; p = procbuf; }
    }
    uc_optrace("FALLOC", p, len, rc);
}

int fallocate(int fd, int mode, off_t offset, off_t len)
{
    UC_REAL(fallocate);
    int rc = real_fallocate(fd, mode, offset, len);
    uc_falloc_trace(fd, (long long)len, rc);
    return rc;
}

/* fallocate64: ultracopier builds with _FILE_OFFSET_BITS=64, so its fallocate() binds to
 * fallocate64@GLIBC_2.10 (confirmed via `nm -D ultracopier`), NOT the plain fallocate above -- we MUST
 * interpose it too or the preallocation is invisible (same reason the stat verbs interpose stat64). */
int fallocate64(int fd, int mode, off64_t offset, off64_t len)
{
    UC_REAL(fallocate64);
    int rc = real_fallocate64(fd, mode, offset, len);
    uc_falloc_trace(fd, (long long)len, rc);
    return rc;
}
