#include "cpp11addition.h"
#include <sstream>
#include <cassert>
#include <stdlib.h>
#include <limits.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>

/// \todo, check number validity by http://en.cppreference.com/w/c/string/byte/strtol
/* to check: bool my_strtol(const std::string &str, long &v) {
char *end = nullptr;
v = strtol(str.c_str(), &end, 10);
return end != nullptr;
}*/

static const std::regex ishexa("^([0-9a-fA-F][0-9a-fA-F])+$",std::regex::optimize);
#if defined(_WIN32) || defined(CATCHCHALLENGER_EXTRA_CHECK)
static const std::regex regexseparators("[/\\\\]+",std::regex::optimize);
#endif

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

static const char* const lut = "0123456789ABCDEF";

std::size_t pairhash::operator()(const std::pair<uint8_t, uint8_t> &x) const
{
    return (x.first << 8) + x.second;
}

std::size_t pairhash::operator()(const std::pair<uint16_t, uint16_t> &x) const
{
    return (x.first << 16) + x.second;
}

std::string ltrim(const std::string& str)
{
  return std::regex_replace(str, std::regex("^ +"), "");
}
std::string rtrim(const std::string& str)
{
  return std::regex_replace(str, std::regex(" +$"), "");
}
std::string trim(const std::string& str)
{
  return std::regex_replace(str, std::regex("^ +| +$"), "");
}

bool stringreplaceOne(std::string& str, const std::string& from, const std::string& to)
{
    const size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

uint8_t stringreplaceAll(std::string& str, const std::string& from, const std::string& to)
{
    if(from.empty())
        return 0;
    size_t start_pos = 0;
    uint8_t count=0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        count++;
    }
    return count;
}

std::vector<std::string> stringregexsplit(const std::string& input, const std::regex& regex)
{
    // passing -1 as the submatch index parameter performs splitting
    std::sregex_token_iterator
        first{input.begin(), input.end(), regex, -1},
        last;
    return {first, last};
}

std::vector<std::string> stringsplit(const std::string &s, char delim)
{
    std::vector<std::string> elems;

    std::string::size_type i = 0;
    std::string::size_type j = s.find(delim);

    if(j == std::string::npos)
    {
        if(!s.empty())
            elems.push_back(s);
        return elems;
    }
    else
    {
        while (j != std::string::npos) {
           elems.push_back(s.substr(i, j-i));
           i = ++j;
           j = s.find(delim, j);

           if (j == std::string::npos)
              elems.push_back(s.substr(i, s.length()));
        }
        return elems;
    }
}

bool stringEndsWith(std::string const &fullString, std::string const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool stringEndsWith(std::string const &fullString, char const &ending)
{
    if (fullString.length()>0) {
        return fullString[fullString.size()-1]==ending;
    } else {
        return false;
    }
}

bool stringStartWith(std::string const &fullString, std::string const &starting)
{
    if (fullString.length() >= starting.length()) {
        return (fullString.substr(0,starting.length())==starting);
    } else {
        return false;
    }
}

bool stringStartWith(std::string const &fullString, char const &starting)
{
    if (fullString.length()>0) {
        return fullString[0]==starting;
    } else {
        return false;
    }
}

std::string& stringimplode(const std::vector<std::string>& elems, char delim, std::string& s)
{
    for (std::vector<std::string>::const_iterator ii = elems.begin(); ii != elems.cend(); ++ii)
    {
        s += (*ii);
        if ( ii + 1 != elems.end() ) {
            s += delim;
        }
    }

    return s;
}

std::string stringimplode(const std::vector<std::string>& elems, char delim)
{
    std::string s;
    return stringimplode(elems, delim, s);
}

std::string stringimplode(const std::queue<std::string>& elems, char delim)
{
    std::string newString;
    std::queue<std::string> copy=elems;
    unsigned int count=0;
    while(!copy.empty())
    {
        if(count>0)
            newString+=delim;
        newString+=copy.front();
        copy.pop();
        ++count;
    }

    return newString;
}

std::string stringimplode(const std::vector<std::string>& elems, const std::string &delim)
{
    std::string newString;
    for (std::vector<std::string>::const_iterator ii = elems.begin(); ii != elems.cend(); ++ii)
    {
        newString += (*ii);
        if ( ii + 1 != elems.end() ) {
            newString += delim;
        }
    }
    return newString;
}

std::string binarytoHexa(const std::vector<char> &data, bool *ok)
{
    if(ok!=NULL)
       *ok=true;
    std::string output;
    output.reserve(2*data.size());
    for(size_t i=0;i<data.size();++i)
    {
        const unsigned char c = data[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

std::string binarytoHexa(const void * const data, const uint32_t &size, bool *ok)
{
    return binarytoHexa(reinterpret_cast<const char *>(data),size,ok);
}

std::string binarytoHexa(const char * const data, const uint32_t &size, bool *ok)
{
    if(size==0)
        return std::string();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(__builtin_expect((size>100000000),0))
    {
        std::cerr << "cpp11addition binarytoHexa() size>100000000, seam be a bug, dropped to empty string" << std::endl;
        return std::string();
    }
    #endif
    if(ok!=NULL)
       *ok=true;
    std::string output;
    output.reserve(2*size);
    for(size_t i=0;i<size;++i)
    {
        const unsigned char c = data[i];
        output.push_back(lut[c >> 4]);
        output.push_back(lut[c & 15]);
    }
    return output;
}

uint8_t hexToDecUnit(const std::string& data, bool *ok)
{
     auto fromHex = [](char c, bool *ok)
     {
         if(ok!=NULL)
             *ok=true;
        if (isdigit(c)) return c - '0';
        switch(c)
        {
            case '0':
                return 0;
            case '1':
                return 1;
            case '2':
                return 2;
            case '3':
                return 3;
            case '4':
                return 4;
            case '5':
                return 5;
            case '6':
                return 6;
            case '7':
                return 7;
            case '8':
                return 8;
            case '9':
                return 9;

            case 'a':
            case 'A':
                return 10;
            case 'b':
            case 'B':
                return 11;
            case 'c':
            case 'C':
                return 12;
            case 'd':
            case 'D':
                return 13;
            case 'e':
            case 'E':
                return 14;
            case 'f':
            case 'F':
                return 15;
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    };
    return fromHex(data[0],ok) << 4 | fromHex(data[1],ok);
}

std::vector<char> hexatoBinary(const std::string &data,bool *ok)
{
    if(data.size()%2!=0)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
           *ok=false;
        return std::vector<char>();
    }
    if(Q_LIKELY(std::regex_match(data,ishexa)))
    {
        bool ok2;
        std::vector<char> out;
        out.reserve(data.length()/2);
        for(size_t i=0;i<data.length();i+=2)
        {
            const std::string &partpfchain=data.substr(i,2);
            const uint8_t &x=hexToDecUnit(partpfchain,&ok2);
            if(!ok2)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
                #endif
                if(ok!=NULL)
                    *ok=false;
                return std::vector<char>();
            }
            out.push_back(x);
        }
        if(ok!=NULL)
            *ok=true;
        return out;
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return std::vector<char>();
    }
}

void binaryAppend(std::vector<char> &data,const std::vector<char> &add)
{
    if(add.empty())
        return;
    if(data.empty())
    {
        data=add;
        return;
    }
    const int oldsize=data.size();
    data.resize(oldsize+add.size());
    memcpy(data.data()+oldsize,add.data(),add.size());
}

void binaryAppend(std::vector<char> &data,const char * const add,const uint32_t &addSize)
{
    if(addSize==0)
        return;
    if(data.empty())
    {
        data.resize(addSize);
        memcpy(data.data(),add,addSize);
        return;
    }
    const int oldsize=data.size();
    data.resize(oldsize+addSize);
    memcpy(data.data()+oldsize,add,addSize);
}

std::vector<char> base64toBinary(const std::string &string)
{
    int index=0;
    int sub_index=0;
    int encoded_string_remaining=string.size();
    int encoded_string_pos=0;
    unsigned char char_array_4[4], char_array_3[3];
    std::vector<char> ret;

    while(encoded_string_remaining-- && (string[encoded_string_pos]!='=') && is_base64(string[encoded_string_pos]))
    {
        char_array_4[index++]=string[encoded_string_pos];
        encoded_string_pos++;
        if(index==4)
        {
            for(index=0;index<4;index++)
                char_array_4[index]=base64_chars.find(char_array_4[index]);

            char_array_3[0]=(char_array_4[0]<<2) + ((char_array_4[1]&0x30)>>4);
            char_array_3[1]=((char_array_4[1]&0xf)<<4) + ((char_array_4[2]&0x3c)>>2);
            char_array_3[2]=((char_array_4[2]&0x3)<<6) + char_array_4[3];

            for(index=0;(index<3);index++)
                ret.push_back(char_array_3[index]);

            index=0;
        }
    }

    if(index)
    {
        for(sub_index=index;sub_index<4;sub_index++)
            char_array_4[sub_index]=0;

        for(sub_index=0;sub_index<4;sub_index++)
            char_array_4[sub_index]=base64_chars.find(char_array_4[sub_index]);

        char_array_3[0]=(char_array_4[0]<<2) + ((char_array_4[1]&0x30)>>4);
        char_array_3[1]=((char_array_4[1]&0xf)<<4) + ((char_array_4[2]&0x3c)>>2);
        char_array_3[2]=((char_array_4[2]&0x3)<<6) + char_array_4[3];

        for (sub_index=0;(sub_index<index-1);sub_index++)
            ret.push_back(char_array_3[sub_index]);
    }

    return ret;
}


std::string FSabsoluteFilePath(const std::string &string)
{
    std::string newstring=string;
    stringreplaceAll(newstring,"//","/");
    #ifdef _WIN32
    stringreplaceAll(newstring,"\\\\","\\");
    std::vector<std::string> parts=stringregexsplit(newstring,regexseparators);
    #else
    std::vector<std::string> parts=stringsplit(newstring,'/');
    #endif

    #ifndef _WIN32
    unsigned int index=1;
    #else
    unsigned int index=2;
    #endif
    while(index<parts.size())
    {
        if(parts.at(index)=="..")
        {
            parts.erase(parts.begin()+index);
            #ifndef _WIN32
            if(index>0 && (index>1 || !parts.at(index-1).empty()))
            #else
            if(index>1)
            #endif
            {
                parts.erase(parts.begin()+index-1);
                index--;
            }
        }
        else
            index++;
    }

    #ifndef _WIN32
    if(parts.empty() || (parts.size()==1 && parts.at(0).empty()))
        return "/";
    #endif
    return stringimplode(parts,'/');
}

std::string FSabsolutePath(const std::string &string)
{
    const std::string &tempFile=FSabsoluteFilePath(string);
    const std::size_t &found=tempFile.find_last_of("/\\");
    if(found!=std::string::npos)
        return tempFile.substr(0,found)+'/';
    else
        return tempFile;
}

std::wstring FSabsoluteFilePath(const std::wstring &string)
{
    std::wstring newstring=string;
    #ifdef _WIN32
    stringreplaceAll(newstring,L"\\",L"/");
    #endif
    stringreplaceAll(newstring,L"//",L"/");
    std::vector<std::wstring> parts=stringsplit(newstring,'/');

    #ifndef _WIN32
    unsigned int index=1;
    #else
    unsigned int index=2;
    #endif
    while(index<parts.size())
    {
        if(parts.at(index)==L"..")
        {
            parts.erase(parts.begin()+index);
            #ifndef _WIN32
            if(index>0 && (index>1 || !parts.at(index-1).empty()))
            #else
            if(index>1)
            #endif
            {
                parts.erase(parts.begin()+index-1);
                index--;
            }
        }
        else
            index++;
    }

    #ifndef _WIN32
    if(parts.empty() || (parts.size()==1 && parts.at(0).empty()))
        return L"/";
    #endif

    std::wstring newString;
    std::vector<std::wstring> copy=parts;
    unsigned int count=0;
    while(!copy.empty())
    {
        if(count>0)
            newString+=L'/';
        newString+=copy.front();
        copy.erase(copy.begin());
        ++count;
    }

    return newString;
}

std::wstring FSabsolutePath(const std::wstring &string)
{
    const std::wstring &tempFile=FSabsoluteFilePath(string);
    const std::size_t &found=tempFile.find_last_of(L"/\\");
    if(found!=std::wstring::npos)
        return tempFile.substr(0,found)+L'/';
    else
        return tempFile;
}

uint64_t msFrom1970()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

uint64_t sFrom1970()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()/1000;
}

std::string booltostring(const bool &value)
{
    if(value)
        return "true";
    else
        return "false";
}

std::vector<std::string> stringtostringlist(const std::string &string)
{
    if(string.empty())
        return std::vector<std::string>();
    std::vector<std::string> returnedVar;
    size_t start_pos = 0;
    size_t firstChar = 0;
    do
    {
        size_t s=start_pos = string.find(',', start_pos);
        if(s == std::string::npos)
            start_pos=string.size();
        if(start_pos>=string.size() || string.at(start_pos+1)!=',')
        {
            std::string tempString=string.substr(firstChar,start_pos-firstChar);
            stringreplaceAll(tempString,",,",",");
            returnedVar.push_back(tempString);
            start_pos++;
            firstChar=start_pos;
        }
        else
            start_pos+=2;
    } while(firstChar<start_pos || start_pos<string.size());
    return returnedVar;
}

std::string stringlisttostring(const std::vector<std::string> &stringlist)
{
    std::string returnedString;
    unsigned int index=0;
    while(index<stringlist.size())
    {
        if(!returnedString.empty())
            returnedString+=',';
        std::string tempString=stringlist.at(index);
        stringreplaceAll(tempString,",",",,");
        returnedString+=tempString;
        index++;
    }
    return returnedString;
}

bool stringreplaceOne(std::wstring& str, const std::wstring& from, const std::wstring& to)
{
    const size_t start_pos = str.find(from);
    if(start_pos == std::wstring::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

uint8_t stringreplaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to)
{
    if(from.empty())
        return 0;
    size_t start_pos = 0;
    uint8_t count=0;
    while((start_pos = str.find(from, start_pos)) != std::wstring::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        count++;
    }
    return count;
}

bool stringEndsWith(std::wstring const &fullString, std::wstring const &ending)
{
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
    } else {
        return false;
    }
}

bool stringEndsWith(std::wstring const &fullString, char const &ending)
{
    if (fullString.length()>0) {
        return fullString[fullString.size()-1]==ending;
    } else {
        return false;
    }
}

bool stringStartWith(std::wstring const &fullString, std::wstring const &starting)
{
    if (fullString.length() >= starting.length()) {
        return (fullString.substr(0,starting.length())==starting);
    } else {
        return false;
    }
}

bool stringStartWith(std::wstring const &fullString, char const &starting)
{
    if (fullString.length()>0) {
        return fullString[0]==starting;
    } else {
        return false;
    }
}

std::vector<std::wstring> stringsplit(const std::wstring &s, wchar_t delim)
{
    std::vector<std::wstring> elems;

    std::wstring::size_type i = 0;
    std::wstring::size_type j = s.find(delim);

    if(j == std::wstring::npos)
    {
        if(!s.empty())
            elems.push_back(s);
        return elems;
    }
    else
    {
        while (j != std::wstring::npos) {
           elems.push_back(s.substr(i, j-i));
           i = ++j;
           j = s.find(delim, j);

           if (j == std::wstring::npos)
              elems.push_back(s.substr(i, s.length()));
        }
        return elems;
    }
}
