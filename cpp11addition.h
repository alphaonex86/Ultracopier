#ifndef CATCHCHALLENGER_CPP11ADDITION_H
#define CATCHCHALLENGER_CPP11ADDITION_H

#include <vector>
#include <queue>
#include <string>
#include <regex>
#include <unordered_map>
#include <unordered_set>

#if ! defined(Q_LIKELY)
    #if defined(__GNUC__)
    #  define Q_LIKELY(expr)    __builtin_expect(!!(expr), true)
    #  define Q_UNLIKELY(expr)  __builtin_expect(!!(expr), false)
    #else
    #  define Q_LIKELY(x) (x)
    #endif
#endif

struct pairhash {
public:
  std::size_t operator()(const std::pair<uint8_t, uint8_t> &x) const;
  std::size_t operator()(const std::pair<uint16_t, uint16_t> &x) const;
};

std::string ltrim(const std::string& str);
std::string rtrim(const std::string& str);
std::string trim(const std::string& str);
bool stringreplaceOne(std::string& str, const std::string& from, const std::string& to);
uint8_t stringreplaceAll(std::string& str, const std::string& from, const std::string& to);
std::vector<std::string> stringregexsplit(const std::string& input, const std::regex& regex);
std::vector<std::string> stringsplit(const std::string &s, char delim);
bool stringEndsWith(std::string const &fullString, std::string const &ending);
bool stringEndsWith(std::string const &fullString, char const &ending);
bool stringStartWith(std::string const &fullString, std::string const &starting);
bool stringStartWith(std::string const &fullString, char const &starting);
std::string& stringimplode(const std::vector<std::string>& elems, char delim, std::string& s);
std::string stringimplode(const std::vector<std::string>& elems, char delim);
std::string stringimplode(const std::queue<std::string>& elems, char delim);
std::string stringimplode(const std::vector<std::string>& elems, const std::string &delim);

uint8_t stringtouint8(const std::string &string,bool *ok=NULL);
uint16_t stringtouint16(const std::string &string,bool *ok=NULL);
uint32_t stringtouint32(const std::string &string,bool *ok=NULL);
bool stringtobool(const std::string &string,bool *ok=NULL);
uint64_t stringtouint64(const std::string &string,bool *ok=NULL);
int8_t stringtoint8(const std::string &string,bool *ok=NULL);
int16_t stringtoint16(const std::string &string,bool *ok=NULL);
int32_t stringtoint32(const std::string &string,bool *ok=NULL);
int64_t stringtoint64(const std::string &string,bool *ok=NULL);
float stringtofloat(const std::string &string,bool *ok=NULL);
double stringtodouble(const std::string &string,bool *ok=NULL);
std::string booltostring(const bool &value);

std::vector<std::string> stringtostringlist(const std::string &string);
std::string stringlisttostring(const std::vector<std::string> &stringlist);

std::string binarytoHexa(const std::vector<char> &data,bool *ok=NULL);
std::string binarytoHexa(const char * const data,const uint32_t &size,bool *ok=NULL);
std::string binarytoHexa(const unsigned char * const data,const uint32_t &size,bool *ok=NULL);
uint8_t hexToDecUnit(const std::string& data,bool *ok=NULL);
std::vector<char> hexatoBinary(const std::string &data,bool *ok=NULL);
void binaryAppend(std::vector<char> &data,const std::vector<char> &add);
void binaryAppend(std::vector<char> &data, const char * const add, const uint32_t &addSize);
std::vector<char> base64toBinary(const std::string &string);
std::string FSabsoluteFilePath(const std::string &string);
std::string FSabsolutePath(const std::string &string);
uint64_t msFrom1970();
uint64_t sFrom1970();

template <class T>
int vectorindexOf(const std::vector<T> &list,const T &item)
{
    const auto &r=std::find(list.cbegin(),list.cend(),item);
    if(r==list.cend())
        return -1;
    else
        return std::distance(list.cbegin(),r);
}

template <class T>
bool vectorremoveOne(std::vector<T> &list,const T &item)
{
    const auto &r=std::find(list.cbegin(),list.cend(),item);
    if(r==list.cend())
    {
        return false;
    }
    else
    {
        list.erase(r);
        return true;
    }
}

template <class T>
bool vectorcontainsAtLeastOne(const std::vector<T> &list,const T &item)
{
    const auto &r=std::find(list.cbegin(),list.cend(),item);
    if(r==list.cend())
        return false;
    else
        return true;
}

template <class T>
unsigned int vectorcontainsCount(const std::vector<T> &list,const T &item)
{
    return std::count(list.cbegin(), list.cend(), item);
}

template <class T, class U>
std::vector<T> unordered_map_keys_vector(const std::unordered_map<T,U> &unordered_map_var)
{
    std::vector<T> keyList;
    keyList.reserve(unordered_map_var.size());
    for ( auto it = unordered_map_var.cbegin(); it != unordered_map_var.cend(); ++it )
        keyList.push_back(it->first);
    return keyList;
}

template <class T>
unsigned int vectorRemoveEmpty(std::vector<T> &list)
{
    unsigned int removedEntryNumber=0;
    for(auto it = list.begin();it != list.cend();)
        if((*it).empty())
        {
            list.erase(it);
            ++removedEntryNumber;
        }
        else
            ++it;
    return removedEntryNumber;
}

template <class T>
unsigned int vectorRemoveDuplicatesForSmallList(std::vector<T> &list)
{
    /*unsigned int removedEntryNumber=0;
    for(auto it = list.begin();it < list.cend()-1;)
    {
        const auto indexOf=std::find(it+1,list.cend(),*it);
        if(indexOf!=list.end())
        {
            list.erase(indexOf);
            ++removedEntryNumber;
        }
        else
            ++it;
    }
    return removedEntryNumber;*/

    std::unordered_set<T> s(list.cbegin(),list.cend());
    const unsigned int removedEntryNumber=list.size()-s.size();
    list=std::vector<T>(s.cbegin(),s.cend());
    return removedEntryNumber;
}

template <class T>
unsigned int vectorRemoveDuplicatesForBigList(std::vector<T> &list)
{
    std::unordered_set<T> s(list.cbegin(),list.cend());
    const unsigned int removedEntryNumber=list.size()-s.size();
    list=std::vector<T>(s.cbegin(),s.cend());
    return removedEntryNumber;
}

template <class T>
unsigned int vectorRemoveAll(std::vector<T> &list,const T &item)
{
    unsigned int removedEntryNumber=0;
    unsigned int index=0;
    while(index<list.size())
    {
        if(list.at(index)==item)
        {
            removedEntryNumber++;
            list.erase(list.cbegin()+index);
        }
        else
            index++;
    }
    return removedEntryNumber;
}

template <class T>
bool vectorHaveDuplicatesForSmallList(const std::vector<T> &list)
{
    std::unordered_set<T> s;
    unsigned int index=0;
    while(index<list.size())
    {
        if(s.find(list.at(index))==s.cend())
            s.insert(list.at(index));
        else
            return true;
        index++;
    }
    return false;
}

#endif
