/** \file QTarDecode.cpp
\brief To read a tar data block
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "QTarDecode.h"

#include <inttypes.h>
#include <regex>
#include <cstring>
#include <iostream>

static const std::regex isaunsignednumber("^[0-9]+$",std::regex::optimize);
static const std::regex isaunoctalnumber("^[0-7]+$",std::regex::optimize);
static const char* const lut = "0123456789ABCDEF";

QTarDecode::QTarDecode()
{
    error="Unknown error";
}

uint64_t QTarDecode::octaltouint64(const std::string &string,bool *ok)
{
    if(std::regex_match(string,isaunsignednumber))
    {
        if(ok!=NULL)
            *ok=true;
        return std::stoi(string,0,8);
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

uint64_t QTarDecode::stringtouint64(const std::string &string,bool *ok)
{
    if(std::regex_match(string,isaunsignednumber))
    {
        if(ok!=NULL)
            *ok=true;
        return std::stoull(string);
    }
    else
    {
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

std::string QTarDecode::errorString()
{
    return error;
}

void QTarDecode::setErrorString(const std::string &error)
{
    this->error=error;
    fileList.clear();
    dataList.clear();
}

bool QTarDecode::stringStartWith(std::string const &fullString, std::string const &starting)
{
    if (fullString.length() >= starting.length()) {
        return (fullString.substr(0,starting.length())==starting);
    } else {
        return false;
    }
}

bool QTarDecode::decodeData(const std::vector<char> &data)
{
    setErrorString("Unknown error");
    if(data.size()<1024)
        return false;
    unsigned int offset=0;
    while(offset<data.size())
    {
        //load the file name from ascii, from 0+offset with size of 100
        std::string fileName(data.data()+offset,100);
        while(!fileName.empty() && fileName.at(fileName.size()-1)==0x00)
            fileName.resize(fileName.size()-1);
        //load the file type
        std::string fileType(data.data()+156+offset,1);
        while(!fileName.empty() && fileType.at(fileType.size()-1)==0x00)
            fileType.resize(fileType.size()-1);
        //load the ustar string
        std::string ustar(data.data()+257+offset,5);
        while(!fileName.empty() && ustar.at(ustar.size()-1)==0x00)
            ustar.resize(ustar.size()-1);
        //load the ustar version
        std::string ustarVersion(data.data()+263+offset,2);
        while(!fileName.empty() && ustarVersion.at(ustarVersion.size()-1)==0x00)
            ustarVersion.resize(ustarVersion.size()-1);
        bool ok=false;
        //load the file size from ascii, from 124+offset with size of 12
        uint64_t finalSize=0;
        std::string sizeString(data.data()+124+offset,12);
        if(sizeString.at(sizeString.size()-1)==0x00)
        {
            //the size is in octal base
            sizeString.resize(sizeString.size()-1);
            finalSize=octaltouint64(sizeString,&ok);
        }
        else
            finalSize=stringtouint64(sizeString,&ok);
        //if the size conversion to qulonglong have failed, then qui with error
        if(ok==false)
        {
            //it's the end of the archive, no more verification
            break;
        }
        //if check if the ustar not found
        if(ustar!="ustar")
        {
            setErrorString("\"ustar\" string not found at "+std::to_string(257+offset)+", content: \""+ustar+"\"");
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
            std::vector<char> newdata;
            newdata.resize(finalSize);
            memcpy(newdata.data(),data.data()+512+offset,finalSize);
            fileList.push_back(fileName);
            dataList.push_back(newdata);
        }
        //calculate the offset for the next header
        bool retenu=((finalSize%512)!=0);
        //go to the next header
        offset+=512+(finalSize/512+retenu)*512;
    }
    if(fileList.size()>0)
    {
        std::string baseDirToTest=fileList.at(0);
        std::size_t found = baseDirToTest.find_last_of("/");
        if(found!=std::string::npos && found>=baseDirToTest.size())
            baseDirToTest.resize(baseDirToTest.size()-(baseDirToTest.size()-found));
        bool isFoundForAllEntries=true;
        for (unsigned int i = 0; i < fileList.size(); ++i)
            if(!stringStartWith(fileList.at(i),baseDirToTest))
                isFoundForAllEntries=false;
        if(isFoundForAllEntries)
            for (unsigned int i = 0; i < fileList.size(); ++i)
                fileList[i].erase(0,baseDirToTest.size());
    }
    return true;
}

std::vector<std::string> QTarDecode::getFileList()
{
    return fileList;
}

std::vector<std::vector<char> > QTarDecode::getDataList()
{
    return dataList;
}


