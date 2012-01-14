#include "QTarDecode.h"
#include <QDebug>

QTarDecode::QTarDecode()
{
	error="Unknow error";
}

QString QTarDecode::errorString()
{
	return error;
}

void QTarDecode::setErrorString(QString error)
{
	this->error=error;
	fileList.clear();
	dataList.clear();
}

bool QTarDecode::decodeData(QByteArray data)
{
	setErrorString("Unknow error");
	if(data.size()<1024)
		return false;
	int offset=0;
	while(offset<data.size())
	{
		//load the file name from ascii, from 0+offset with size of 100
		QString fileName=data.mid(0+offset,100);
		//load the file type
		QString fileType=data.mid(156+offset,1);
		//load the ustar string
		QString ustar=data.mid(257+offset,5);
		//load the ustar version
		QString ustarVersion=data.mid(263+offset,2);
		bool ok;
		//load the file size from ascii, from 124+offset with size of 12
		int finalSize=QString(data.mid(124+offset,12)).toUInt(&ok,8);
		//if the size conversion to qulonglong have failed, then qui with error
		if(ok==false)
		{
			//if((offset+1024)==data.size())
			//it's the end of the archive, no more verification
			break;
			/*else
			{
				setErrorString("Unable to convert ascii size at "+QString::number(124+offset)+" to usigned long long: \""+QString(data.mid(124+offset,12))+"\"");
				return false;
			}*/
		}
		//if check if the ustar not found
		if(ustar!="ustar")
		{
			setErrorString("\"ustar\" string not found at "+QString::number(257+offset)+", content: \""+ustar+"\"");
			return false;
		}
		if(ustarVersion!="00")
		{
			setErrorString("ustar version string is wrong, content:\""+ustarVersion+"\"");
			return false;
		}
		//check if the file have the good size for load the data
		if((offset+512+finalSize)>data.size())
		{
			setErrorString("The tar file seem be too short");
			return false;
		}
		if(fileType=="0") //this code manage only the file, then only the file is returned
		{
			QByteArray fileData=data.mid(512+offset,finalSize);
			fileList << fileName;
			dataList << fileData;
		}
		//calculate the offset for the next header
		bool retenu=((finalSize%512)!=0);
		//go to the next header
		offset+=512+(finalSize/512+retenu)*512;
	}
	if(fileList.size()>0)
	{
		QString baseDirToTest=fileList.at(0);
		baseDirToTest.remove(QRegExp("/.*$"));
		baseDirToTest+='/';
		bool isFoundForAllEntries=true;
		for (int i = 0; i < fileList.size(); ++i)
			if(!fileList.at(i).startsWith(baseDirToTest))
				isFoundForAllEntries=false;
		if(isFoundForAllEntries)
			for (int i = 0; i < fileList.size(); ++i)
				fileList[i].remove(0,baseDirToTest.size());
	}
	return true;
}

QStringList QTarDecode::getFileList()
{
	return fileList;
}

QList<QByteArray> QTarDecode::getDataList()
{
	return dataList;
}


