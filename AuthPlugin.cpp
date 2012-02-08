/** \file AuthPlugin.cpp
\brief Define the authentication plugin
\author alpha_one_x86
\version 0.3
\date 2010 */

#include <QDir>

#include "AuthPlugin.h"

AuthPlugin::AuthPlugin()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	//load the overall instance
	stopIt=false;
	//set sem to 1
	sem.release();
	moveToThread(this);
}

AuthPlugin::~AuthPlugin()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	stop();
}

QStringList AuthPlugin::getFileList(QString path)
{
	QStringList list;
	QDir dir(path);
	if(dir.exists())
	{
		foreach(QString dirName, dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
			list<<getFileList(path+dirName+QDir::separator());
		foreach(QString fileName, dir.entryList(QDir::Files|QDir::NoDotAndDotDot))
			list<<path+fileName;
	}
	return list;
}

void AuthPlugin::run()
{
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"start");
	sem.acquire();
	stopIt=false;
	foreach(QString basePath,readPath)
	{
		foreach(QString dirSub,searchPathPlugin)
		{
			QString pluginComposed=basePath+dirSub+QDir::separator();
			QDir dir(pluginComposed);
			if(dir.exists())
			{
				foreach(QString dirName, dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot))
				{
					QString pluginBasePath	= pluginComposed+dirName+QDir::separator();
					if(QFile::exists(pluginBasePath+"sign.key"))
					{
						QCryptographicHash folderHash(QCryptographicHash::Sha1);
						QStringList filesList=getFileList(pluginBasePath);
						if(stopIt)
							return;
						QFile keyDescriptor(pluginBasePath+"sign.key");
						if(keyDescriptor.open(QIODevice::ReadOnly))
						{
							if(stopIt)
								return;
							bool errorFound=false;
							foreach(QString fileToAddToCheckSum,filesList)
								if(fileToAddToCheckSum!=(pluginBasePath+"sign.key"))
								{
									QFile fileDescriptor(fileToAddToCheckSum);
									if(fileDescriptor.open(QIODevice::ReadOnly))
									{
										if(stopIt)
											return;
										folderHash.addData(fileDescriptor.readAll());
										fileDescriptor.close();
										if(stopIt)
											return;
									}
									else
									{
										ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"Error at open this file: "+fileToAddToCheckSum+", error string: "+fileDescriptor.errorString());
										errorFound=true;
										break;
									}
								}
							if(!errorFound)
							{
								ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"folder: \""+pluginBasePath+"\", hash: "+QString(folderHash.result().toHex()));
								QByteArray key=keyDescriptor.readAll();
								if(key==folderHash.result())
									emit authentifiedPath(pluginBasePath);
								else
									ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"This plugin have wrong authentification");
							}
							keyDescriptor.close();
						}
						else
							ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Warning,"file: \""+pluginBasePath+"sign.key\", unable to open the key file: "+keyDescriptor.errorString());
					}
				}
			}
		}
	}
	ULTRACOPIER_DEBUGCONSOLE(DebugLevel_Notice,"stop()");
	sem.release();
}

void AuthPlugin::loadSearchPath(QStringList readPath,QStringList searchPathPlugin)
{
	this->readPath=readPath;
	this->searchPathPlugin=searchPathPlugin;
}

void AuthPlugin::stop()
{
	stopIt=true;
	sem.acquire();
	sem.release();
}
