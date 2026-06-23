/*
 * fuse_fault.c -- read-only passthrough FUSE filesystem that injects a recoverable mid-file
 * READ fault, used to verify the pipelined backends' media-reconnect RESUME end-to-end.
 *
 * WHY FUSE (and not the LD_PRELOAD shim): the io_uring / IOCP data planes submit reads through
 * the ring / overlapped ReadFile, NOT libc read(), so the LD_PRELOAD shim cannot fault them.
 * A read on a FUSE-mounted file, however, is serviced by THIS daemon regardless of how the
 * caller issued it (io_uring included) -- so faulting here reaches the ring. The daemon also
 * COUNTS bytes actually read, which lets a test tell RESUME (total ~= file size) from RESTART
 * (total ~= size + fault_offset * retries).
 *
 * It mirrors the shim's `disconnect` verb:
 *   UC_FUSE_BACKING=<dir>              the real directory whose contents are exposed read-only
 *   UC_FUSE_FAULT=<substr>:<off>:<n>   reads of a path containing <substr> succeed up to <off>
 *                                      bytes, then fail EIO for <n> attempts (media unplugged),
 *                                      then succeed (replugged). <n> is a PER-PATH budget.
 *   UC_FUSE_READLOG_MATCH=<substr>     count successful read bytes of matching paths ...
 *   UC_FUSE_READLOG_PATH=<file>        ... and flush the running total (decimal) here.
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
static char g_log_match[512];   static char g_log_path[4096];
static long long g_read_total=0, g_read_flushed=0;
static pthread_mutex_t g_mtx=PTHREAD_MUTEX_INITIALIZER;

/* per-path remaining fault budget (keyed by the matched substring, like the shim's flaky) */
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
    char b[4608]; backpath(b,sizeof(b),path);
    int fd=open(b,O_RDONLY); if(fd<0) return -errno; fi->fh=fd; return 0;
}
static int ff_read(const char *path,char *buf,size_t size,off_t off,struct fuse_file_info *fi){
    /* fault at/after the disconnect offset (so the readable prefix is delivered first) */
    if(off>=g_fault_off && should_fail(path)) return -EIO;
    ssize_t r=pread((int)fi->fh,buf,size,off); if(r<0) return -errno;
    account(path,r); return (int)r;
}
static int ff_release(const char *path,struct fuse_file_info *fi){(void)path; if(fi->fh) close((int)fi->fh); return 0;}
static int ff_readdir(const char *path,void *buf,fuse_fill_dir_t filler,off_t off,
                      struct fuse_file_info *fi,enum fuse_readdir_flags flags){
    (void)off;(void)fi;(void)flags; char b[4608]; backpath(b,sizeof(b),path);
    DIR *d=opendir(b); if(!d) return -errno; struct dirent *de;
    while((de=readdir(d))!=NULL) filler(buf,de->d_name,NULL,0,0);
    closedir(d); return 0;
}
static int ff_access(const char *path,int mask){char b[4608]; backpath(b,sizeof(b),path); return access(b,mask)==0?0:-errno;}

static struct fuse_operations ff_ops = {
    .getattr=ff_getattr, .readlink=ff_readlink, .open=ff_open, .read=ff_read,
    .release=ff_release, .readdir=ff_readdir, .access=ff_access,
};

int main(int argc,char *argv[])
{
    const char *bk=getenv("UC_FUSE_BACKING"); if(!bk){fprintf(stderr,"UC_FUSE_BACKING unset\n");return 2;}
    strncpy(g_backing,bk,sizeof(g_backing)-1);
    const char *fl=getenv("UC_FUSE_FAULT");
    if(fl){ /* <substr>:<off>:<n> -- split on the last two colons */
        char tmp[1024]; strncpy(tmp,fl,sizeof(tmp)-1);
        char *c2=strrchr(tmp,':');
        if(c2){*c2='\0'; g_fault_n=strtol(c2+1,NULL,10);
            char *c1=strrchr(tmp,':'); if(c1){*c1='\0'; g_fault_off=strtoll(c1+1,NULL,10);}}
        strncpy(g_fault_sub,tmp,sizeof(g_fault_sub)-1);
    }
    const char *lm=getenv("UC_FUSE_READLOG_MATCH"); if(lm) strncpy(g_log_match,lm,sizeof(g_log_match)-1);
    const char *lp=getenv("UC_FUSE_READLOG_PATH");  if(lp) strncpy(g_log_path,lp,sizeof(g_log_path)-1);
    return fuse_main(argc,argv,&ff_ops,NULL);
}
