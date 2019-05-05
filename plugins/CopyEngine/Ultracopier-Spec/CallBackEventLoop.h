#ifndef CALLBACKEVENTLOOP_H
#define CALLBACKEVENTLOOP_H

#include <QObject>

#ifdef Q_OS_LINUX
class CallBackEventLoop
{
public:
    virtual void callBack() = 0;
};
#endif

#endif // CALLBACKEVENTLOOP_H
