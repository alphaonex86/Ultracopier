#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "Variable.h"

#ifdef ASYNCFILEMANIP
#include <QThread>
#include "CallBackEventLoop.h"
#include <sys/epoll.h>
#define MAXEVENTS 64

class EventLoop : public QThread
{
public:
    EventLoop();
    ~EventLoop();
     static EventLoop eventLoop;
     void watchSource(CallBackEventLoop * const object,const int &fd);
     void watchDestination(CallBackEventLoop * const object,const int &fd);
protected:
    void run();
    void stop();
private:
    epoll_event events[MAXEVENTS];
    int efd;
    bool stopIt;
};
#endif

#endif // EVENTLOOP_H
