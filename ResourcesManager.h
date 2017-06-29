/** \file ResourcesManager.h
\brief Define the class to manage and load the resources linked with the themes
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef RESOURCES_MANAGER_H
#define RESOURCES_MANAGER_H

#include <QStringList>
#include <QString>
#include <QObject>
#include <regex>

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
        std::string getFolderReadPath(const std::string &path) const;
        /** \brief Get folder presence, the path and check in the folder and sub-folder the file presence
        \return Empty QString if not found */
        std::string getFolderReadPathMultiple(const std::string &path,const std::vector<std::string> &fileToCheck) const;
        bool checkFolderContent(const std::string &path,const std::vector<std::string> &fileToCheck) const;
        /// \brief add / or \ in function of the platform at the end of path if both / and \ are not found
        static std::string AddSlashIfNeeded(const std::string &path);
        /// \brief get the writable path
        std::string getWritablePath() const;
        /// \brief disable the writable path, if ultracopier is unable to write into
        bool disableWritablePath();
        /// \brief get the read path
        std::vector<std::string> getReadPath() const;
        /// \brief remove folder
        static bool removeFolder(const std::string &dir);
    private:
        /// \brief List of the path to read only access
        std::vector<std::string> searchPath;
        /// \brief The writable path, empty if not found
        std::string writablePath;
        /// \brief match with slash end
        static std::regex slashEnd;
};

#endif // RESOURCES_MANAGER_H
