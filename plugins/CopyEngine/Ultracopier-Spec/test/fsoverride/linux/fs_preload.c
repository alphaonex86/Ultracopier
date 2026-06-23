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
    UC_SHORTWRITE,   /* write returns fewer bytes on matching path */
    UC_STATFAIL,     /* stat family fails EACCES on matching path  */
    UC_READFAIL,     /* read fails EIO on matching path            */
    UC_EIO_AFTER,    /* read fails EIO after N cumulative bytes/fd  */
    UC_FLAKY,        /* first N read attempts/path fail EIO        */
    UC_GONE,         /* open fails ENOENT (stat OK): removed-after-listing */
    UC_DISCONNECT,   /* SOURCE media: read OK to <offset>, then EIO for <n> attempts, then OK */
    UC_WDISCONNECT   /* DEST media: write OK to <offset>, then EIO for <n> attempts, then OK */
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

static void uc_parse(void)
{
    if (g_parsed)
        return;
    g_parsed = 1;
    /* Read-accounting is independent of UC_FS_SCENARIO: set even when no fault is injected. */
    g_readlog_match = getenv("UC_FS_READLOG_MATCH");
    g_readlog_on = (g_readlog_match != NULL && g_readlog_match[0] != '\0');
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
        } else if (strcmp(tok, "shortwrite") == 0) {
            r->verb = UC_SHORTWRITE;
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

static void uc_sleep_ms(long ms)
{
    if (ms <= 0)
        return;
    struct timespec ts;
    ts.tv_sec  = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
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
    if (uc_match(UC_OPENFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    if (uc_match(UC_GONE, pathname) != NULL) {
        errno = ENOENT;     /* listed by the scan (stat OK) but gone at transfer-open */
        return -1;
    }
    int fd = real_open(pathname, flags, mode);
    if (fd >= 0)
        uc_remember_fd(fd, pathname);
    return fd;
}

int open64(const char *pathname, int flags, ...)
{
    UC_REAL(open64);
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap; va_start(ap, flags); mode = (mode_t)va_arg(ap, int); va_end(ap);
    }
    if (uc_match(UC_OPENFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    if (uc_match(UC_GONE, pathname) != NULL) {
        errno = ENOENT;     /* listed by the scan (stat OK) but gone at transfer-open */
        return -1;
    }
    int fd = real_open64(pathname, flags, mode);
    if (fd >= 0)
        uc_remember_fd(fd, pathname);
    return fd;
}

int openat(int dirfd, const char *pathname, int flags, ...)
{
    UC_REAL(openat);
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list ap; va_start(ap, flags); mode = (mode_t)va_arg(ap, int); va_end(ap);
    }
    if (uc_match(UC_OPENFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    if (uc_match(UC_GONE, pathname) != NULL) {
        errno = ENOENT;     /* listed by the scan (stat OK) but gone at transfer-open */
        return -1;
    }
    int fd = real_openat(dirfd, pathname, flags, mode);
    if (fd >= 0)
        uc_remember_fd(fd, pathname);
    return fd;
}

int close(int fd)
{
    UC_REAL(close);
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
    if (path != NULL) {
        size_t do_count = count;
        int dec = uc_read_fault(path, count, uc_fd_nread(fd), &do_count);
        if (dec < 0)
            return -1;                      /* errno already EIO */
        if (dec > 0)
            count = do_count;               /* clamp to the bad-sector boundary */
    }
    ssize_t n = real_read(fd, buf, count);
    if (n > 0)
        uc_fd_nread_add(fd, (long long)n);  /* advance the per-fd cursor */
    uc_account_read(path, n);
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

ssize_t write(int fd, const void *buf, size_t count)
{
    UC_REAL(write);
    const char *path = uc_fd_path(fd);
    const struct uc_rule *slow = uc_slow();
    if (slow != NULL)
        uc_sleep_ms(slow->num);
    if (path != NULL) {
        if (uc_match(UC_EFAIL, path) != NULL) {
            errno = EIO;
            return -1;
        }
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
            ssize_t w = real_write(fd, buf, half);
            if (w > 0)
                uc_fd_nwritten_add(fd, (long long)w);
            return w;
        }
    }
    ssize_t w = real_write(fd, buf, count);
    if (w > 0)
        uc_fd_nwritten_add(fd, (long long)w);
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
    }
    ssize_t n = real_pread(fd, buf, count, offset);
    uc_account_read(path, n);
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
        /* pwrite is positioned: the cumulative-so-far for wdisconnect is the OFFSET. */
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
    }
    return real_pwrite(fd, buf, count, offset);
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
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real_stat(pathname, statbuf);
}

int lstat(const char *pathname, struct stat *statbuf)
{
    UC_REAL(lstat);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real_lstat(pathname, statbuf);
}

int fstat(int fd, struct stat *statbuf)
{
    UC_REAL(fstat);
    const char *path = uc_fd_path(fd);
    if (path != NULL && uc_match(UC_STATFAIL, path) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real_fstat(fd, statbuf);
}

int fstatat(int dirfd, const char *pathname, struct stat *statbuf, int flags)
{
    UC_REAL(fstatat);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real_fstatat(dirfd, pathname, statbuf, flags);
}

/* Older-glibc ABI symbols. `ver` is the stat struct version (_STAT_VER). */
int __xstat(int ver, const char *pathname, struct stat *statbuf)
{
    UC_REAL(__xstat);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real___xstat(ver, pathname, statbuf);
}

int __lxstat(int ver, const char *pathname, struct stat *statbuf)
{
    UC_REAL(__lxstat);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real___lxstat(ver, pathname, statbuf);
}

int __fxstat(int ver, int fd, struct stat *statbuf)
{
    UC_REAL(__fxstat);
    const char *path = uc_fd_path(fd);
    if (path != NULL && uc_match(UC_STATFAIL, path) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real___fxstat(ver, fd, statbuf);
}

int __fxstatat(int ver, int dirfd, const char *pathname, struct stat *statbuf, int flags)
{
    UC_REAL(__fxstatat);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real___fxstatat(ver, dirfd, pathname, statbuf, flags);
}

/* statx(2): the modern path Qt/glibc may use for metadata. */
int statx(int dirfd, const char *pathname, int flags, unsigned int mask,
          struct statx *statxbuf)
{
    UC_REAL(statx);
    if (uc_match(UC_STATFAIL, pathname) != NULL) {
        errno = EACCES;
        return -1;
    }
    return real_statx(dirfd, pathname, flags, mask, statxbuf);
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
    return real_mkdir(pathname, mode);
}

int unlink(const char *pathname)
{
    UC_REAL(unlink);
    return real_unlink(pathname);
}

int rename(const char *oldpath, const char *newpath)
{
    UC_REAL(rename);
    return real_rename(oldpath, newpath);
}

int symlink(const char *target, const char *linkpath)
{
    UC_REAL(symlink);
    return real_symlink(target, linkpath);
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
    if (uc_match(UC_EFAIL, pathname) != NULL) {
        errno = EIO;
        return -1;
    }
    return real_utimensat(dirfd, pathname, times, flags);
}

int chmod(const char *pathname, mode_t mode)
{
    UC_REAL(chmod);
    if (uc_match(UC_EFAIL, pathname) != NULL) {
        errno = EIO;
        return -1;
    }
    return real_chmod(pathname, mode);
}
