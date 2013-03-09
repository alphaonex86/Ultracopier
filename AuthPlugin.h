/** \file AuthPlugin.h
\brief Define the authentication plugin
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef AUTHPLUGIN_H
#define AUTHPLUGIN_H

#include <QThread>
#include <QObject>
#include <QSemaphore>
#include <QStringList>
#include <QString>
#include <QCryptographicHash>

#include "Environment.h"

#ifndef ULTRACOPIER_PLUGIN_ALL_IN_ONE
/** \brief allow authentify the plugin */
class AuthPlugin : public QThread
{
    Q_OBJECT
public:
    AuthPlugin();
    ~AuthPlugin();
    /** \brief stop all action to close ultracopier */
    void stop();
    /** \brief add path to check
      \param readPath list of read place
      \param searchPathPlugin list of plugin place
      */
    void loadSearchPath(const QStringList &readPath,const QStringList &searchPathPlugin);
protected:
    void run();
private:
    bool stopIt;
    QSemaphore sem;
    QStringList readPath;
    QStringList searchPathPlugin;
    static QStringList getFileList(const QString &path);
signals:
    void authentifiedPath(QString);
};
#endif

#endif // AUTHPLUGIN_H
