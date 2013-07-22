/** \file ResourcesManager.h
\brief Define the class to manage and load the resources linked with the themes
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef RESOURCES_MANAGER_H
#define RESOURCES_MANAGER_H

#include <QStringList>
#include <QString>
#include <QObject>
#include <QRegularExpression>

#include "Environment.h"

/** \brief Define the class to manage and load the resources linked with the themes

This class provide a core load and manage the resources */
class ResourcesManager : public QObject
{
    Q_OBJECT
    public:
        /// \brief Create the manager and load the default variable
        ResourcesManager();
        /// \brief Destroy the resource manager
        ~ResourcesManager();
        static ResourcesManager *resourcesManager;
        /** \brief Get folder presence and the path
        \return Empty QString if not found */
        QString getFolderReadPath(const QString &path) const;
        /** \brief Get folder presence, the path and check in the folder and sub-folder the file presence
        \return Empty QString if not found */
        QString getFolderReadPathMultiple(const QString &path,const QStringList &fileToCheck) const;
        bool checkFolderContent(const QString &path,const QStringList &fileToCheck) const;
        /// \brief add / or \ in function of the platform at the end of path if both / and \ are not found
        static QString AddSlashIfNeeded(const QString &path);
        /// \brief get the writable path
        QString getWritablePath() const;
        /// \brief disable the writable path, if ultracopier is unable to write into
        bool disableWritablePath();
        /// \brief get the read path
        QStringList getReadPath() const;
        /// \brief remove folder
        static bool removeFolder(const QString &dir);
    private:
        /// \brief List of the path to read only access
        QStringList searchPath;
        /// \brief The writable path, empty if not found
        QString writablePath;
        /// \brief match with slash end
        static QRegularExpression slashEnd;
};

#endif // RESOURCES_MANAGER_H
