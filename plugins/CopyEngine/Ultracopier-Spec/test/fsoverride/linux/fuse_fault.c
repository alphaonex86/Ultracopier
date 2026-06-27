/*
 * fuse_fault.c -- passthrough FUSE filesystem for faulting the io_uring / IOCP DATA plane, which
 * the LD_PRELOAD shim cannot reach (those backends submit reads/writes through the ring / overlapped
 * ReadFile/WriteFile, NOT libc, but a FUSE-mounted file is serviced by THIS daemon regardless of how
 * the caller issued the I/O -- io_uring included).
 *
 * Two independent fault modes (both optional; unset == pure passthrough):
 *
 *   READ fault (source-side, recoverable -- media reconnect/resume):
 *     UC_FUSE_BACKING=<dir>            the real directory exposed at the mountpoint
 *     UC_FUSE_FAULT=<substr>:<off>:<n> reads of a path containing <substr> succeed up to <off> bytes,
 *                                      then fail EIO for <n> attempts (unplugged), then succeed. PER-PATH.
 *     UC_FUSE_READLOG_MATCH=<substr> / UC_FUSE_READLOG_PATH=<file>  count+flush read bytes (resume proof).
 *
 *   WRITE corruption (dest-side, SILENT -- must be caught by transferChecksum):
 *     UC_FUSE_WFAULT=<substr>:<off>    a write to a path containing <substr> that covers byte <off> has
 *                                      that byte BIT-FLIPPED on its way to the backing, but the write
 *                                      still returns the FULL count -- so the engine believes it
 *                                      succeeded. A re-read (also via this FS) returns the corrupt byte,
 *                                      so transferChecksum's verify must detect the mismatch and NOT
 *                                      claim the file complete. (Flip is buf[i]^0xFF, deterministic and
 *                                      idempotent across re-writes, so a retry re-corrupts identically.)
 *
 * Build: cc -O2 -Wall $(pkg-config --cflags fuse3) fuse_fault.c -o build/fuse_fault $(pkg-config --libs fuse3)
 * Run:   ./build/fuse_fault <mountpoint> -f -s   (foreground, single-threaded)
 */
#define FUSE_USE_VERSION 31
#define _GNU_SOURCE
#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>

static char g_backing[4096];
static char g_fault_sub[512];   static long long g_fault_off=0;   static long g_fault_n=0;
static char g_wfault_sub[512];  static long long g_wfault_off=-1;
static char g_openfail_sub[512];   /* open() of a matching path -> ENOENT, but getattr still succeeds:
                                    * a TOCTOU source-vanish (listed at scan, gone at transfer-open) that
                                    * reaches the io_uring openat, which the libc shim's `gone` cannot. */
static char g_log_match[512];   static char g_log_path[4096];
static long long g_read_total=0, g_read_flushed=0;
static pthread_mutex_t g_mtx=PTHREAD_MUTEX_INITIALIZER;

/* per-path remaining read-fault budget (keyed by the matched substring, like the shim's flaky) */
#define MAXF 64
static struct { int used; char key[512]; long remaining; } g_budget[MAXF];

static void backpath(char *out,size_t n,const char *path){ snprintf(out,n,"%s%s",g_backing,path); }

static int should_fail(const char *path)
{
    if(g_fault_sub[0]=='\0' || strstr(path,g_fault_sub)==NULL) return 0;
    int fail=0; pthread_mutex_lock(&g_mtx);
    int slot=-1;
    for(int i=0;i<MAXF;i++) if(g_budget[i].used && strcmp(g_budget[i].key,g_fault_sub)==0){slot=i;break;}
    if(slot<0) for(int i=0;i<MAXF;i++) if(!g_budget[i].used){slot=i;g_budget[i].used=1;
        strncpy(g_budget[i].key,g_fault_sub,sizeof(g_budget[i].key)-1);g_budget[i].remaining=g_fault_n;break;}
    if(slot>=0 && g_budget[slot].remaining>0){g_budget[slot].remaining--;fail=1;}
    pthread_mutex_unlock(&g_mtx); return fail;
}

static void account(const char *path,long long n)
{
    if(n<=0 || g_log_match[0]=='\0' || strstr(path,g_log_match)==NULL) return;
    pthread_mutex_lock(&g_mtx);
    g_read_total+=n;
    if(g_log_path[0] && g_read_total-g_read_flushed>=262144){
        g_read_flushed=g_read_total;
        FILE *f=fopen(g_log_path,"w"); if(f){fprintf(f,"%lld\n",g_read_total);fclose(f);}
    }
    pthread_mutex_unlock(&g_mtx);
}

static int ff_getattr(const char *path,struct stat *st,struct fuse_file_info *fi){
    (void)fi; char b[4608]; backpath(b,sizeof(b),path);
    return lstat(b,st)==0?0:-errno;
}
static int ff_readlink(const char *path,char *buf,size_t size){
    char b[4608]; backpath(b,sizeof(b),path);
    ssize_t r=readlink(b,buf,size-1); if(r<0) return -errno; buf[r]='\0'; return 0;
}
static int ff_open(const char *path,struct fuse_file_info *fi){
    /* TOCTOU source-vanish: the file was listed (getattr/readdir still succeed) but its open now fails
     * ENOENT, modelling a source removed between scan and transfer. Reaches the io_uring openat. */
    if(g_openfail_sub[0]!='\0' && strstr(path,g_openfail_sub)!=NULL) return -ENOENT;
    char b[4608]; backpath(b,sizeof(b),path);
    int fd=open(b,fi->flags); if(fd<0) return -errno; fi->fh=fd; return 0;
}
static int ff_create(const char *path,mode_t mode,struct fuse_file_info *fi){
    char b[4608]; backpath(b,sizeof(b),path);
    int fd=open(b,fi->flags|O_CREAT,mode); if(fd<0) return -errno; fi->fh=fd; return 0;
}
static int ff_read(const char *path,char *buf,size_t size,off_t off,struct fuse_file_info *fi){
    /* fault at/after the disconnect offset (so the readable prefix is delivered first) */
    if(off>=g_fault_off && should_fail(path)) return -EIO;
    ssize_t r=pread((int)fi->fh,buf,size,off); if(r<0) return -errno;
    account(path,r); return (int)r;
}
static int ff_write(const char *path,const char *buf,size_t size,off_t off,struct fuse_file_info *fi){
    if(g_wfault_sub[0] && g_wfault_off>=0 && strstr(path,g_wfault_sub)
       && off<=g_wfault_off && off+(off_t)size>(off_t)g_wfault_off){
        /* SILENT corruption: flip the byte at g_wfault_off, return the FULL count. The flip is
         * deterministic (buf^0xFF) so a retry re-corrupts identically; the backing therefore always
         * differs from the source by exactly one byte. transferChecksum's re-read must catch it. */
        char *tmp=malloc(size); if(!tmp) return -ENOMEM;
        memcpy(tmp,buf,size);
        tmp[(size_t)(g_wfault_off-off)]^=0xFF;
        ssize_t w=pwrite((int)fi->fh,tmp,size,off); free(tmp);
        return w<0?-errno:(int)size;
    }
    ssize_t w=pwrite((int)fi->fh,buf,size,off);
    return w<0?-errno:(int)w;
}
static int ff_truncate(const char *path,off_t off,struct fuse_file_info *fi){
    if(fi && fi->fh) return ftruncate((int)fi->fh,off)==0?0:-errno;
    char b[4608]; backpath(b,sizeof(b),path); return truncate(b,off)==0?0:-errno;
}
static int ff_fsync(const char *path,int ds,struct fuse_file_info *fi){
    (void)path; if(!fi||!fi->fh) return 0;
    return (ds?fdatasync((int)fi->fh):fsync((int)fi->fh))==0?0:-errno;
}
static int ff_release(const char *path,struct fuse_file_info *fi){(void)path; if(fi->fh) close((int)fi->fh); return 0;}
static int ff_readdir(const char *path,void *buf,fuse_fill_dir_t filler,off_t off,
                      struct fuse_file_info *fi,enum fuse_readdir_flags flags){
    (void)off;(void)fi;(void)flags; char b[4608]; backpath(b,sizeof(b),path);
    DIR *d=opendir(b); if(!d) return -errno; struct dirent *de;
    while((de=readdir(d))!=NULL) filler(buf,de->d_name,NULL,0,0);
    closedir(d); return 0;
}
static int ff_mkdir(const char *path,mode_t mode){char b[4608]; backpath(b,sizeof(b),path); return mkdir(b,mode)==0?0:-errno;}
static int ff_rmdir(const char *path){char b[4608]; backpath(b,sizeof(b),path); return rmdir(b)==0?0:-errno;}
static int ff_unlink(const char *path){char b[4608]; backpath(b,sizeof(b),path); return unlink(b)==0?0:-errno;}
static int ff_symlink(const char *target,const char *path){char b[4608]; backpath(b,sizeof(b),path); return symlink(target,b)==0?0:-errno;}
static int ff_rename(const char *from,const char *to,unsigned int flags){
    if(flags) return -EINVAL; char a[4608],b[4608]; backpath(a,sizeof(a),from); backpath(b,sizeof(b),to);
    return rename(a,b)==0?0:-errno;
}
static int ff_chmod(const char *path,mode_t mode,struct fuse_file_info *fi){
    (void)fi; char b[4608]; backpath(b,sizeof(b),path); return chmod(b,mode)==0?0:-errno;
}
static int ff_chown(const char *path,uid_t u,gid_t g,struct fuse_file_info *fi){
    (void)fi; char b[4608]; backpath(b,sizeof(b),path); return lchown(b,u,g)==0?0:-errno;
}
static int ff_utimens(const char *path,const struct timespec tv[2],struct fuse_file_info *fi){
    (void)fi; char b[4608]; backpath(b,sizeof(b),path);
    return utimensat(AT_FDCWD,b,tv,AT_SYMLINK_NOFOLLOW)==0?0:-errno;
}
static int ff_access(const char *path,int mask){char b[4608]; backpath(b,sizeof(b),path); return access(b,mask)==0?0:-errno;}

static struct fuse_operations ff_ops = {
    .getattr=ff_getattr, .readlink=ff_readlink, .open=ff_open, .create=ff_create,
    .read=ff_read, .write=ff_write, .truncate=ff_truncate, .fsync=ff_fsync,
    .release=ff_release, .readdir=ff_readdir, .mkdir=ff_mkdir, .rmdir=ff_rmdir,
    .unlink=ff_unlink, .symlink=ff_symlink, .rename=ff_rename, .chmod=ff_chmod,
    .chown=ff_chown, .utimens=ff_utimens, .access=ff_access,
};

/* split "<substr>:<num>" on the LAST colon (substr may itself contain ':') */
static void split_last(const char *in,char *sub,size_t subn,long long *num){
    char tmp[1024]; strncpy(tmp,in,sizeof(tmp)-1); tmp[sizeof(tmp)-1]='\0';
    char *c=strrchr(tmp,':'); if(c){*c='\0'; *num=strtoll(c+1,NULL,10);}
    strncpy(sub,tmp,subn-1);
}

int main(int argc,char *argv[])
{
    const char *bk=getenv("UC_FUSE_BACKING"); if(!bk){fprintf(stderr,"UC_FUSE_BACKING unset\n");return 2;}
    strncpy(g_backing,bk,sizeof(g_backing)-1);
    const char *fl=getenv("UC_FUSE_FAULT");
    if(fl){ /* <substr>:<off>:<n> -- split on the last two colons */
        char tmp[1024]; strncpy(tmp,fl,sizeof(tmp)-1); tmp[sizeof(tmp)-1]='\0';
        char *c2=strrchr(tmp,':');
        if(c2){*c2='\0'; g_fault_n=strtol(c2+1,NULL,10);
            char *c1=strrchr(tmp,':'); if(c1){*c1='\0'; g_fault_off=strtoll(c1+1,NULL,10);}}
        strncpy(g_fault_sub,tmp,sizeof(g_fault_sub)-1);
    }
    const char *wf=getenv("UC_FUSE_WFAULT");
    if(wf) split_last(wf,g_wfault_sub,sizeof(g_wfault_sub),&g_wfault_off);
    const char *of=getenv("UC_FUSE_OPENFAIL"); if(of) strncpy(g_openfail_sub,of,sizeof(g_openfail_sub)-1);
    const char *lm=getenv("UC_FUSE_READLOG_MATCH"); if(lm) strncpy(g_log_match,lm,sizeof(g_log_match)-1);
    const char *lp=getenv("UC_FUSE_READLOG_PATH");  if(lp) strncpy(g_log_path,lp,sizeof(g_log_path)-1);
    return fuse_main(argc,argv,&ff_ops,NULL);
}
