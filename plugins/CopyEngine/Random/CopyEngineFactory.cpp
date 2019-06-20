/** \file factory.cpp
\brief Define the factory to create new instance
\author alpha_one_x86 */

#include "../../../cpp11addition.h"
#include "CopyEngineFactory.h"

CopyEngineFactory::CopyEngineFactory()
{
}

CopyEngineFactory::~CopyEngineFactory()
{
}

PluginInterface_CopyEngine * CopyEngineFactory::getInstance()
{
    CopyEngine *realObject=new CopyEngine();
    return realObject;
}

void CopyEngineFactory::setResources(OptionInterface * options,const std::string &writePath,const std::string &pluginPath,
                                     FacilityInterface * facilityInterface,const bool &portableVersion)
{
    (void)options;
    (void)writePath;
    (void)pluginPath;
    (void)facilityInterface;
    (void)portableVersion;
}

std::vector<std::string> CopyEngineFactory::supportedProtocolsForTheSource() const
{
    std::vector<std::string> l;
    l.push_back("file");
    return l;
}

std::vector<std::string> CopyEngineFactory::supportedProtocolsForTheDestination() const
{
    std::vector<std::string> l;
    l.push_back("file");
    return l;
}

Ultracopier::CopyType CopyEngineFactory::getCopyType()
{
    return Ultracopier::FileAndFolder;
}

Ultracopier::TransferListOperation CopyEngineFactory::getTransferListOperation()
{
    return Ultracopier::TransferListOperation_ImportExport;
}

bool CopyEngineFactory::canDoOnlyCopy() const
{
    return false;
}

void CopyEngineFactory::resetOptions()
{
}

QWidget * CopyEngineFactory::options()
{
    return nullptr;
}

void CopyEngineFactory::newLanguageLoaded()
{
}
