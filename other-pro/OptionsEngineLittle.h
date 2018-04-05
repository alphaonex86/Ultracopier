#ifndef OPTIONSENGINELITTLE_H
#define OPTIONSENGINELITTLE_H

#include "../interface/OptionInterface.h"
#include <unordered_map>
#include <string>
#include <utility>
#include <vector>

class OptionsEngineLittle : public OptionInterface
{
public:
    OptionsEngineLittle();
    /// \brief To add option group to options
    bool addOptionGroup(const std::vector<std::pair<std::string, std::string> > &KeysList);
    /// \brief To get option value
    std::string getOptionValue(const std::string &variableName) const;
    /// \brief To set option value
    void setOptionValue(const std::string &variableName,const std::string &value);
private:
    std::unordered_map<std::string,std::string> options;
};

#endif // OPTIONSENGINELITTLE_H
