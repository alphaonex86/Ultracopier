#include "EventLoop.h"
#include "CallBackEventLoop.h"
#include <sys/epoll.h>

#ifdef Q_OS_LINUX
EventLoop EventLoop::eventLoop;

EventLoop::EventLoop()
{
    int efd = epoll_create1(0);
    if(efd==-1)
        abort();
    start();
}

void EventLoop::run()
{
    while(1)
    {
        int number_of_events = epoll_wait(efd, events, MAXEVENTS, -1);
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
        abort();
}

void EventLoop::watchDestination(CallBackEventLoop * const object,const int &fd)
{
    epoll_event event;
    event.events = EPOLLOUT | EPOLLPRI | EPOLLERR | EPOLLET;
    event.data.ptr = object;
    if(epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event)!=0)
        abort();
}
#endif
