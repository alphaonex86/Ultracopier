#include <QObject>

#if defined(Q_OS_LINUX)
#  include "qstorageinfo_linux_p.h"
#elif defined(Q_OS_WIN)
#  include "qstorageinfo_win_p.h"
#elif defined(Q_OS_MAC)
#  include "qstorageinfo_mac_p.h"
#endif