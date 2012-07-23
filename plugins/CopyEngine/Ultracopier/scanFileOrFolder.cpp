#include "scanFileOrFolder.h"

#include <QMessageBox>

scanFileOrFolder::scanFileOrFolder(CopyMode mode)
{
	stopped	= true;
	stopIt	= false;
	this->mode=mode;
	setObjectName("ScanFileOrFolder");
	folder_isolation=QRegularExpression("^(.*/)?([^/]+)/$");
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
	this->sources=parseWildcardSources(sources);
        this->destination=destination;
	if(sources.size()>1 || QFileInfo(destination).isDir())
		/* Disabled because the separator transformation product bug
		 * if(!destination.endsWith(QDir::separator()))
			this->destination+=QDir::separator();*/
		if(!destination.endsWith("/") && !destination.endsWith("\\"))
			this->destination+="/";//put unix separator because it's transformed into that's under windows too
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"addToList("+sources.join(";")+","+destination+")");
}


QStringList scanFileOrFolder::parseWildcardSources(const QStringList &sources)
{
	QRegularExpression splitFolder("[/\\\\]");
	QStringList returnList;
	int index=0;
	while(index<sources.size())
	{
		if(sources.at(index).contains("*"))
		{
			QStringList toParse=sources.at(index).split(splitFolder);
			ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("before wildcard parse: %1, toParse: %2, is valid: %3").arg(sources.at(index)).arg(toParse.join(", ")).arg(splitFolder.isValid()));
			QList<QStringList> recomposedSource;
			recomposedSource << (QStringList() << "");
			while(toParse.size()>0)
			{
				if(toParse.first().contains('*'))
				{
					QString toParseFirst=toParse.first();
					if(toParseFirst=="")
						toParseFirst+="/";
					QList<QStringList> newRecomposedSource;
					QRegularExpression toResolv=QRegularExpression(toParseFirst.replace('*',"[^/\\\\]*"));
					int index_recomposedSource=0;
					while(index_recomposedSource<recomposedSource.size())//parse each url part
					{
						QFileInfo info(recomposedSource.at(index_recomposedSource).join("/"));
						if(info.isDir())
						{
							QDir folder(info.absoluteFilePath());
							QFileInfoList fileFile=folder.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System);//QStringList() << toResolv
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("list the folder: %1, with the wildcard: %2").arg(info.absoluteFilePath()).arg(toResolv.pattern()));
							int index_fileList=0;
							while(index_fileList<fileFile.size())
							{
								if(fileFile.at(index_fileList).fileName().contains(toResolv))
								{
									QStringList tempList=recomposedSource.at(index_recomposedSource);
									tempList << fileFile.at(index_fileList).fileName();
									newRecomposedSource << tempList;
								}
								index_fileList++;
							}
						}
						index_recomposedSource++;
					}
					recomposedSource=newRecomposedSource;
				}
				else
				{
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("add toParse: %1").arg(toParse.join("/")));
					int index_recomposedSource=0;
					while(index_recomposedSource<recomposedSource.size())
					{
						recomposedSource[index_recomposedSource] << toParse.first();
						if(!QFileInfo(recomposedSource.at(index_recomposedSource).join("/")).exists())
							recomposedSource.removeAt(index_recomposedSource);
						else
							index_recomposedSource++;
					}
				}
				toParse.removeFirst();
			}
			int index_recomposedSource=0;
			while(index_recomposedSource<recomposedSource.size())
			{
				returnList<<recomposedSource.at(index_recomposedSource).join("/");
				index_recomposedSource++;
			}
		}
		else
			returnList << sources.at(index);
		index++;
	}
	return returnList;
}

void scanFileOrFolder::setFilters(QList<Filters_rules> include,QList<Filters_rules> exclude)
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	QMutexLocker lock(&filtersMutex);
	this->include_send=include;
	this->exclude_send=exclude;
	reloadTheNewFilters=true;
	haveFilters=include_send.size()>0 || exclude_send.size()>0;
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,QString("haveFilters: %1, include_send.size(): %2, exclude_send.size(): %3").arg(haveFilters).arg(include_send.size()).arg(exclude_send.size()));
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
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"pattern: "+folder_isolation.pattern());
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"full: "+destinationSuffixPath);
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"prefix: "+prefix);
					ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"suffix: "+suffix);
					//resolv the new name
					QFileInfo destinationInfo;
					int num=1;
					do
					{
						if(num==1)
						{
							if(firstRenamingRule=="")
								destinationSuffixPath=tr("%1 - copy").arg(suffix);
							else
							{
								destinationSuffixPath=firstRenamingRule;
								destinationSuffixPath.replace("%name%",suffix);
							}
						}
						else
						{
							if(otherRenamingRule=="")
								destinationSuffixPath=tr("%1 - copy (%2)").arg(suffix).arg(num);
							else
							{
								destinationSuffixPath=otherRenamingRule;
								destinationSuffixPath.replace("%name%",suffix);
								destinationSuffixPath.replace("%number%",QString::number(num));
							}
						}
						num++;
						destinationInfo.setFile(prefix+destinationSuffixPath);
					}
					while(destinationInfo.exists());
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
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"pattern: "+folder_isolation.pattern());
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"full: "+destinationSuffixPath);
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"prefix: "+prefix);
						ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"suffix: "+suffix);
						//resolv the new name
						QFileInfo destinationInfo;
						int num=1;
						do
						{
							if(num==1)
							{
								if(firstRenamingRule=="")
									destinationSuffixPath=tr("%1 - copy").arg(suffix);
								else
								{
									destinationSuffixPath=firstRenamingRule;
									destinationSuffixPath.replace("%name%",suffix);
								}
							}
							else
							{
								if(otherRenamingRule=="")
									destinationSuffixPath=tr("%1 - copy (%2)").arg(suffix).arg(num);
								else
								{
									destinationSuffixPath=otherRenamingRule;
									destinationSuffixPath.replace("%name%",suffix);
									destinationSuffixPath.replace("%number%",QString::number(num));
								}
							}
							destinationInfo.setFile(prefix+destinationSuffixPath);
							num++;
						}
						while(destinationInfo.exists());
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
	QFileInfoList entryList=finalSource.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);//possible wait time here
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
				this->include=this->include_send;
				this->exclude=this->exclude_send;
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
					filters_index++;
				}
				if(excluded)
				{}
				else
				{
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
						filters_index++;
					}
					if(!included)
					{}
					else
						listFolder(source,destination,sourceSuffixPath+fileInfo.fileName()+"/",destinationSuffixPath+fileName+"/");
				}
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
					filters_index++;
				}
				if(excluded)
				{}
				else
				{
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
						filters_index++;
					}
					if(!included)
					{}
					else
						emit fileTransfer(fileInfo.absoluteFilePath(),finalDest+fileName,mode);
				}
			}
		}
		else
		{
			if(fileInfo.isDir())//possible wait time here
				//listFolder(source,destination,suffixPath+fileInfo.fileName()+QDir::separator());
				listFolder(source,destination,sourceSuffixPath+fileInfo.fileName()+"/",destinationSuffixPath+fileInfo.fileName()+"/");//put unix separator because it's transformed into that's under windows too
			else
				emit fileTransfer(fileInfo.absoluteFilePath(),finalDest+fileInfo.fileName(),mode);
		}
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

void scanFileOrFolder::setRenamingRules(QString firstRenamingRule,QString otherRenamingRule)
{
	this->firstRenamingRule=firstRenamingRule;
	this->otherRenamingRule=otherRenamingRule;
}
