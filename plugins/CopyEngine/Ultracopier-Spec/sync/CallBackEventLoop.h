#ifndef CALLBACKEVENTLOOP_H
#define CALLBACKEVENTLOOP_H

#include <QObject>
#include "Variable.h"

#ifdef POSIXFILEMANIP
class CallBackEventLoop
{
public:
    virtual void callBack() = 0;
};
#endif

#endif // CALLBACKEVENTLOOP_H
