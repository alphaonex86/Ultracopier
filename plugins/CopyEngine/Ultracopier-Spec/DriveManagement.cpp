#include "DriveManagement.h"

#include <QDir>
#include <QFileInfoList>
#include <QStorageInfo>

#include "../../../cpp11addition.h"

DriveManagement::DriveManagement()
{
    tryUpdate();
    #ifdef Q_OS_WIN32
    reg3=std::regex("^[a-zA-Z]:[\\\\/].*");
    reg4=std::regex("^([a-zA-Z]:[\\\\/]).*$");
    #endif
    /// \warn ULTRACOPIER_DEBUGCONSOLE() don't work here because the sinal slot is not connected!
}

//get drive of an file or folder
/// \todo do network drive support for windows
std::string DriveManagement::getDrive(const std::string &fileOrFolder) const
{
    const std::string &inode=fileOrFolder;
    Q_UNUSED(inode);
    #ifdef Q_OS_WIN32
    //optimized to windows version:
    if(fileOrFolder.size()>=3)
    {
        if(fileOrFolder.at(1)==L':' && (fileOrFolder.at(2)==L'\\' || fileOrFolder.at(2)==L'/'))
        {
            char driveLetter=toupper(fileOrFolder.at(0));
            return driveLetter+std::string(":/");
        }
    }

    if(fileOrFolder.size()>=5)
    {
        char f1=fileOrFolder.at(0);
        char f2=fileOrFolder.at(1);
        if(f1=='/' || f1=='\\')
            if(f2=='/' || f2=='\\')
            {
                bool postSeparador=false;
                std::string post;
                unsigned int index=2;
                unsigned int s=2;
                while(index<fileOrFolder.size())
                {
                    const char c=fileOrFolder.at(index);
                    if(c=='/' || c=='\\')
                    {
                        if(postSeparador==false)
                        {
                            post="//"+fileOrFolder.substr(2,index-2);
                            postSeparador=true;
                            char c;
                            do
                            {
                                index++;
                                c=fileOrFolder.at(index);
                            } while((c=='/' || c=='\\') && index<fileOrFolder.size());
                            s=index;
                        }
                        else
                            return post+"/"+fileOrFolder.substr(s,index-s);
                    }
                    index++;
                }
                return post;
            }
        /*std::string returnString=fileOrFolder;
        std::regex_replace(returnString,reg2,"$1");
        return returnString;*/
    }
    //due to lack of WMI support into mingw, the new drive event is never called, this is a workaround
    if(std::regex_match(fileOrFolder,reg3))
    {
        std::string returnString=fileOrFolder;
        std::regex_replace(returnString,reg4,"$1");
        return QDir::toNativeSeparators(QString::fromStdString(returnString)).toUpper().toStdString();
    }
    #else
    int size=mountSysPoint.size();
    for (int i = 0; i < size; ++i) {
        if(stringStartWith(inode,mountSysPoint.at(i)))
            return mountSysPoint.at(i);
    }
    #endif
    //if unable to locate the right mount point
    ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"unable to locate the right mount point for: "+inode+", mount point: "+stringimplode(mountSysPoint,";"));
    return std::string();
}

std::string DriveManagement::getDriveType(const std::string &drive) const
{
    int index=vectorindexOf(mountSysPoint,drive);
    if(index!=-1)
        return driveType.at(index);
    return std::string();
}

bool DriveManagement::isSameDrive(const std::string &file1,const std::string &file2) const
{
    if(file1.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"file1 is empty");
        return false;
    }
    if(file2.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Critical,"file2 is empty");
        return false;
    }
    if(mountSysPoint.size()==0)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"no mount point found");
        return false;
    }
    const std::string &drive1=getDrive(file1);
    if(drive1.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"drive for the file1 not found: "+file1);
        return false;
    }
    const std::string &drive2=getDrive(file2);
    if(drive2.empty())
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,"drive for the file2 not found: "+file2);
        return false;
    }
    if(drive1==drive2)
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,drive1+" is egal to "+drive2);
        return true;
    }
    else
    {
        ULTRACOPIER_DEBUGCONSOLE(Ultracopier::DebugLevel_Notice,drive1+" is NOT egal to "+drive2);
        return false;
    }
}

void DriveManagement::tryUpdate()
{
    mountSysPoint.clear();
    driveType.clear();
    std::vector<std::pair<std::string/*mountSysPoint*/,std::string/*driveType*/> > temp;
    const QList<QStorageInfo> mountedVolumesList=QStorageInfo::mountedVolumes();
    int index=0;
    while(index<mountedVolumesList.size())
    {
        std::string mountSysPoint=QDir::toNativeSeparators(mountedVolumesList.at(index).rootPath()).toStdString();
        #ifdef Q_OS_WIN32
        std::string driveType;
        if(mountSysPoint!="A:\\" && mountSysPoint!="A:/" && mountSysPoint!="A:" && mountSysPoint!="A" &&
                mountSysPoint!="a:\\" && mountSysPoint!="a:/" && mountSysPoint!="a:" && mountSysPoint!="a")
        {
            const QByteArray &data=mountedVolumesList.at(index).fileSystemType();
            driveType=std::string(data.constData(),data.size());
        }
        #else
        const QByteArray &data=mountedVolumesList.at(index).fileSystemType();
        std::string driveType=std::string(data.constData(),data.size());
        #endif
        temp.push_back(std::pair<std::string/*mountSysPoint*/,std::string/*driveType*/>(mountSysPoint,driveType));
        index++;
    }
    /*sort larger to small mount point, to correctly detect it: /mnt, /
    then /mnt/folder/file will be detected as /mnt
    then /folder/file will be detected as / */
    std::sort(temp.begin(), temp.end(), [](
              std::pair<std::string/*mountSysPoint*/,std::string/*driveType*/> a,
              std::pair<std::string/*mountSysPoint*/,std::string/*driveType*/> b) {
            return a.first.size() > b.first.size();
        });
    for(const std::pair<std::string/*mountSysPoint*/,std::string/*driveType*/> &a : temp) {
        mountSysPoint.push_back(a.first);
        driveType.push_back(a.second);
    }
}
