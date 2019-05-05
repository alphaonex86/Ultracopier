#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <QThread>

#ifdef Q_OS_LINUX
#include "CallBackEventLoop.h"
#include <sys/epoll.h>
#define MAXEVENTS 64

class EventLoop : public QThread
{
public:
    EventLoop();
     static EventLoop eventLoop;
     void watchSource(CallBackEventLoop * const object,const int &fd);
     void watchDestination(CallBackEventLoop * const object,const int &fd);
protected:
    void run();
private:
    epoll_event events[MAXEVENTS];
    int efd;
};
#endif

#endif // EVENTLOOP_H
