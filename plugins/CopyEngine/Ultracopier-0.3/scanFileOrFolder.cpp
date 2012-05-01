#include "scanFileOrFolder.h"

#include <QMessageBox>

scanFileOrFolder::scanFileOrFolder(CopyMode mode)
{
	stopped	= true;
	stopIt	= false;
	this->mode=mode;
	setObjectName("ScanFileOrFolder");
	folder_isolation=QRegExp("^(.*/)?([^/]+)/$");
}

scanFileOrFolder::~scanFileOrFolder()
{
	stop();
	quit();
	wait();
}

bool scanFileOrFolder::isFinished()
{
	return stopped;
}

void scanFileOrFolder::addToList(const QStringList& sources,const QString& destination)
{
	stopIt=false;
	this->sources=sources;
        this->destination=destination;
	if(sources.size()>1 || QFileInfo(destination).isDir())
		/* Disabled because the separator transformation product bug
		 * if(!destination.endsWith(QDir::separator()))
			this->destination+=QDir::separator();*/
		if(!destination.endsWith("/") && !destination.endsWith("\\"))
			this->destination+="/";//put unix separator because it's transformed into that's under windows too
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"addToList("+sources.join(";")+","+destination+")");
}

void scanFileOrFolder::setFilters(QList<Filters_rules> include,QList<Filters_rules> exclude)
{
	QMutexLocker lock(&filtersMutex);
	this->include_send=include;
	this->exclude_send=exclude;
	reloadTheNewFilters=true;
	haveFilters=include_send.size()>0 || exclude_send.size()>0;
}

//set action if Folder are same or exists
void scanFileOrFolder::setFolderExistsAction(FolderExistsAction action,QString newName)
{
	this->newName=newName;
	folderExistsAction=action;
	waitOneAction.release();
}

//set action if error
void scanFileOrFolder::setFolderErrorAction(FileErrorAction action)
{
	fileErrorAction=action;
	waitOneAction.release();
}

void scanFileOrFolder::stop()
{
	stopIt=true;
	waitOneAction.release();
}

void scanFileOrFolder::run()
{
	stopped=false;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start the listing with destination: "+destination+", mode: "+QString::number(mode));
        QDir destinationFolder(destination);
	int sourceIndex=0;
	while(sourceIndex<sources.size())
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"size source to list: "+QString::number(sourceIndex)+"/"+QString::number(sources.size()));
		if(stopIt)
		{
			stopped=true;
			return;
		}
		QFileInfo source=sources.at(sourceIndex);
		if(source.isDir())
		{
			/* Bad way; when you copy c:\source\folder into d:\destination, you wait it create the folder d:\destination\folder
			//listFolder(source.absoluteFilePath()+QDir::separator(),destination);
			listFolder(source.absoluteFilePath()+"/",destination);//put unix separator because it's transformed into that's under windows too
			*/
			//put unix separator because it's transformed into that's under windows too
                        listFolder(source.absolutePath()+"/",destinationFolder.absolutePath()+"/",source.fileName()+"/",source.fileName()+"/");
		}
		else
			emit fileTransfer(source,destination+source.fileName(),mode);
		sourceIndex++;
	}
	stopped=true;
	if(stopIt)
		return;
	emit finishedTheListing();
}

void scanFileOrFolder::listFolder(const QString& source,const QString& destination,const QString& sourceSuffixPath,QString destinationSuffixPath)
{
        ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"source: "+source+", destination: "+destination+", sourceSuffixPath: "+sourceSuffixPath+", destinationSuffixPath: "+destinationSuffixPath);
	if(stopIt)
		return;
        QString newSource	= source+sourceSuffixPath;
        QString finalDest	= destination+destinationSuffixPath;
	//if is same
	if(newSource==finalDest)
	{
		QDir dirSource(newSource);
		emit folderAlreadyExists(dirSource.absolutePath(),finalDest,true);
		waitOneAction.acquire();
		switch(folderExistsAction)
		{
			case FolderExists_Merge:
			break;
			case FolderExists_Skip:
				return;
			break;
			case FolderExists_Rename:
				if(newName=="")
				{
					/// \todo use facility here
					if(destinationSuffixPath.contains(folder_isolation))
					{
						prefix=destinationSuffixPath;
						suffix=destinationSuffixPath;
						prefix.replace(folder_isolation,"\\1");
						suffix.replace(folder_isolation,"\\2");
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"pattern: "+folder_isolation.pattern());
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"full: "+destinationSuffixPath);
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"prefix: "+prefix);
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"suffix: "+suffix);
						destinationSuffixPath = prefix+tr("Copy of ")+suffix;
					}
					else
						destinationSuffixPath = tr("Copy of ")+"Unknow";
				}
				else
					destinationSuffixPath = newName+"/";
				destinationSuffixPath+="/";
				finalDest = destination+destinationSuffixPath;
			break;
			default:
				return;
			break;
		}
	}
	//check if destination exists
	if(checkDestinationExists)
	{
		QDir finalSource(newSource);
		QDir destinationDir(finalDest);
		if(destinationDir.exists())
		{
			emit folderAlreadyExists(finalSource.absolutePath(),destinationDir.absolutePath(),false);
			waitOneAction.acquire();
			switch(folderExistsAction)
			{
				case FolderExists_Merge:
				break;
				case FolderExists_Skip:
					return;
				break;
				case FolderExists_Rename:
					if(newName=="")
					{
						/// \todo use facility here
						if(destinationSuffixPath.contains(folder_isolation))
						{
							prefix=destinationSuffixPath;
							suffix=destinationSuffixPath;
							prefix.replace(folder_isolation,"\\1");
							suffix.replace(folder_isolation,"\\2");
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"pattern: "+folder_isolation.pattern());
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"full: "+destinationSuffixPath);
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"prefix: "+prefix);
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"suffix: "+suffix);
							destinationSuffixPath = prefix+tr("Copy of ")+suffix;
						}
						else
							destinationSuffixPath = tr("Copy of ")+"Unknow";
					}
					else
						destinationSuffixPath = newName;
					destinationSuffixPath+="/";
					finalDest = destination+destinationSuffixPath;
				break;
				default:
					return;
				break;
			}
		}
	}
	//do source check
	QDir finalSource(newSource);
	QFileInfo dirInfo(newSource);
	//check of source is readable
	do
	{
		fileErrorAction=FileError_NotSet;
		if(!dirInfo.isReadable() || !dirInfo.isExecutable() || !dirInfo.exists())
		{
			if(!dirInfo.exists())
				emit errorOnFolder(dirInfo,tr("The folder not exists"));
			else
				emit errorOnFolder(dirInfo,tr("The folder is not readable"));
			waitOneAction.acquire();
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"actionNum: "+QString::number(fileErrorAction));
		}
	} while(fileErrorAction==FileError_Retry);
	/// \todo check here if the folder is not readable or not exists
	QFileInfoList entryList=finalSource.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst);//possible wait time here
	int sizeEntryList=entryList.size();
	emit newFolderListing(newSource);
	if(sizeEntryList==0)
		emit addToMkPath(finalDest);
	for (int index=0;index<sizeEntryList;++index)
	{
		QFileInfo fileInfo=entryList.at(index);
		if(stopIt)
			return;
		if(haveFilters)
		{
			if(reloadTheNewFilters)
			{
				QMutexLocker lock(&filtersMutex);
				QCoreApplication::processEvents(QEventLoop::AllEvents);
				reloadTheNewFilters=false;
			}
			QString fileName=fileInfo.fileName();
			if(fileInfo.isDir())
			{
				bool excluded=false,included=(include.size()==0);
				int filters_index=0;
				while(filters_index<exclude.size())
				{
					if(exclude.at(filters_index).apply_on==ApplyOn_folder || exclude.at(filters_index).apply_on==ApplyOn_fileAndFolder)
					{
						if(fileName.contains(exclude.at(filters_index).regex))
						{
							excluded=true;
							break;
						}
					}
				}
				if(excluded)
					break;
				filters_index=0;
				while(filters_index<include.size())
				{
					if(include.at(filters_index).apply_on==ApplyOn_folder || include.at(filters_index).apply_on==ApplyOn_fileAndFolder)
					{
						if(fileName.contains(include.at(filters_index).regex))
						{
							included=true;
							break;
						}
					}
				}
				if(!included)
					break;
				listFolder(source,destination,sourceSuffixPath+fileInfo.fileName()+"/",destinationSuffixPath+fileName+"/");
			}
			else
			{
				bool excluded=false,included=(include.size()==0);
				int filters_index=0;
				while(filters_index<exclude.size())
				{
					if(exclude.at(filters_index).apply_on==ApplyOn_file || exclude.at(filters_index).apply_on==ApplyOn_fileAndFolder)
					{
						if(fileName.contains(exclude.at(filters_index).regex))
						{
							excluded=true;
							break;
						}
					}
				}
				if(excluded)
					break;
				filters_index=0;
				while(filters_index<include.size())
				{
					if(include.at(filters_index).apply_on==ApplyOn_file || include.at(filters_index).apply_on==ApplyOn_fileAndFolder)
					{
						if(fileName.contains(include.at(filters_index).regex))
						{
							included=true;
							break;
						}
					}
				}
				if(!included)
					break;
				emit fileTransfer(fileInfo.absoluteFilePath(),finalDest+fileName,mode);
			}
			continue;
		}
		if(fileInfo.isDir())//possible wait time here
			//listFolder(source,destination,suffixPath+fileInfo.fileName()+QDir::separator());
			listFolder(source,destination,sourceSuffixPath+fileInfo.fileName()+"/",destinationSuffixPath+fileInfo.fileName()+"/");//put unix separator because it's transformed into that's under windows too
		else
			emit fileTransfer(fileInfo.absoluteFilePath(),finalDest+fileInfo.fileName(),mode);
	}
	if(mode==Move)
	{
		ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"newSource: "+newSource+", sizeEntryList: "+QString::number(sizeEntryList));
		emit addToRmPath(newSource,sizeEntryList);
	}
}

//set if need check if the destination exists
void scanFileOrFolder::setCheckDestinationFolderExists(const bool checkDestinationFolderExists)
{
	this->checkDestinationExists=checkDestinationFolderExists;
}
