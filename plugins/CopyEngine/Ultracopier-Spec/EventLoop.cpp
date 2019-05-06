#include "EventLoop.h"
#include "CallBackEventLoop.h"
#include <sys/epoll.h>
#include <signal.h>

#ifdef Q_OS_LINUX
EventLoop EventLoop::eventLoop;

EventLoop::EventLoop()
{
    int efd = epoll_create1(0);
    if(efd==-1)
    {
        fprintf(stderr,"%s, errno %i\n", strerror(errno), errno);
        abort();
    }
    start();
    stopIt=false;
}

EventLoop::~EventLoop()
{
    stop();
    QThread::wait();
}

void EventLoop::stop()
{
    stopIt=true;
}

void EventLoop::run()
{
    while(!stopIt)
    {
        int number_of_events = epoll_wait(efd, events, MAXEVENTS, -1);
        if (-1 == number_of_events && EINTR == errno)
            return;
        for(int i = 0; i < number_of_events; i++)
            static_cast<CallBackEventLoop *>(events[i].data.ptr)->callBack();
    }
}

void EventLoop::watchSource(CallBackEventLoop * const object,const int &fd)
{
    epoll_event event;
    event.events = EPOLLIN | EPOLLPRI | EPOLLERR | EPOLLET;
    event.data.ptr = object;
    if(epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event)!=0)
    {
        printf("%s, errno %i\n", strerror(errno), errno);
        //abort();
    }
}

void EventLoop::watchDestination(CallBackEventLoop * const object,const int &fd)
{
    epoll_event event;
    event.events = EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLET;
    event.data.ptr = object;
    if(epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event)!=0)
    {
        printf("%s, errno %i\n", strerror(errno), errno);
        //abort();
    }
}
#endif
