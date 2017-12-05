#include "cpp11addition.h"
#include <cstdio>
#include <stdexcept>

/// \todo, check number validity by http://en.cppreference.com/w/c/string/byte/strtol
/* to check: bool my_strtol(const std::string &str, long &v) {
char *end = nullptr;
v = strtol(str.c_str(), &end, 10);
return end != nullptr;
}*/

static const std::regex isaunsignednumber("^[0-9]+$",std::regex::optimize);
static const std::regex isasignednumber("^-?[0-9]+$",std::regex::optimize);
static const std::regex isadouble("^-?[0-9]+(\\.[0-9]+)?$",std::regex::optimize);

uint8_t stringtouint8(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    unsigned int tempValue;
    try {
      tempValue = std::stoul(string);
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(tempValue<=0xFF))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return tempValue;
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #else
    if(Q_LIKELY(std::regex_match(string,isaunsignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoul(string);
        if(Q_LIKELY(tempValue<=0xFF))
            return tempValue;
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
            #endif
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

uint16_t stringtouint16(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    unsigned int tempValue;
    try {
      tempValue = std::stoul(string);
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(tempValue<=0xFFFF))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return tempValue;
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #else
    if(Q_LIKELY(std::regex_match(string,isaunsignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoul(string);
        if(Q_LIKELY(tempValue<=0xFFFF))
            return tempValue;
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
            #endif
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

uint32_t stringtouint32(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    uint32_t tempValue;
    try {
      tempValue = std::stoul(string);
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(ok!=NULL))
        *ok=true;
    return tempValue;
    #else
    if(Q_LIKELY(std::regex_match(string,isaunsignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoull(string);
        if(Q_LIKELY(tempValue<=0xFFFFFFFF))
            return tempValue;
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
            #endif
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

bool stringtobool(const std::string &string,bool *ok)
{
    if(string=="1")
    {
        if(ok!=NULL)
            *ok=true;
        return true;
    }
    if(string=="0")
    {
        if(ok!=NULL)
            *ok=true;
        return false;
    }
    if(string=="true" || string=="t" /*postgresql*/ || string=="TRUE")
    {
        if(ok!=NULL)
            *ok=true;
        return true;
    }
    if(string=="false" || string=="f" /*postgresql*/ || string=="FALSE")
    {
        if(ok!=NULL)
            *ok=true;
        return false;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
    #endif
    if(ok!=NULL)
        *ok=false;
    return false;
}

uint64_t stringtouint64(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    uint64_t tempValue;
    try {
      tempValue = std::stoull(string);
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(ok!=NULL))
        *ok=true;
    return tempValue;
    #else
    if(Q_LIKELY(std::regex_match(string,isaunsignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stoull(string);
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

int8_t stringtoint8(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    int tempValue;
    try {
      tempValue = std::stol(string);
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(tempValue<=0x7F && tempValue>=-0x7F))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return tempValue;
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #else
    if(Q_LIKELY(std::regex_match(string,isasignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoi(string);
        if(Q_LIKELY(tempValue<=0x7F))
            return tempValue;
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
            #endif
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

int16_t stringtoint16(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    int tempValue;
    try {
      tempValue = std::stol(string);
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(tempValue<=0x7FFF && tempValue>=-0x7FFF))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return tempValue;
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #else
    if(Q_LIKELY(std::regex_match(string,isasignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        const unsigned int &tempValue=std::stoi(string);
        if(Q_LIKELY(tempValue<=0x7FFF))
            return tempValue;
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
            #endif
            if(ok!=NULL)
                *ok=false;
            return 0;
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

int32_t stringtoint32(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    int32_t tempValue;
    try {
      tempValue = std::stol(string);
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(ok!=NULL))
        *ok=true;
    return tempValue;
    #else
    if(Q_LIKELY(std::regex_match(string,isasignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stoi(string);
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

int64_t stringtoint64(const std::string &string,bool *ok)
{
    #ifdef __EXCEPTIONS
    int64_t tempValue;
    try {
      tempValue = std::stoll(string);
    }
    catch(...) {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(Q_LIKELY(ok!=NULL))
            *ok=false;
        return 0;
    }
    if(Q_LIKELY(ok!=NULL))
        *ok=true;
    return tempValue;
    #else
    if(Q_LIKELY(std::regex_match(string,isasignednumber)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stoll(string);
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
    #endif
}

float stringtofloat(const std::string &string,bool *ok)
{
    if(Q_LIKELY(std::regex_match(string,isadouble)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stof(string);
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}

double stringtodouble(const std::string &string,bool *ok)
{
    if(Q_LIKELY(std::regex_match(string,isadouble)))
    {
        if(Q_LIKELY(ok!=NULL))
            *ok=true;
        return std::stod(string);
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        std::cerr << "Convertion failed and repported at " << __FILE__ << ":" << __LINE__ << std::endl;
        #endif
        if(ok!=NULL)
            *ok=false;
        return 0;
    }
}
