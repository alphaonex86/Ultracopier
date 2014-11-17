/** \file pluginLoader.cpp
\brief Define the session plugin loader test
\author alpha_one_x86 */

#include "pluginLoader.h"
#include "PlatformMacro.h"

#include <QFile>
#include <QDir>

KeyBindPlugin::KeyBindPlugin()
{
    connect(&optionsWidget,&OptionsWidget::sendKeyBind,this,&KeyBindPlugin::setKeyBind);
}

KeyBindPlugin::~KeyBindPlugin()
{
}

void KeyBindPlugin::setResources(OptionInterface * options,const QString &writePath,const QString &pluginPath,const bool &portableVersion)
{
    Q_UNUSED(options);
    Q_UNUSED(writePath);
    Q_UNUSED(pluginPath);
    Q_UNUSED(portableVersion);
    this->optionsEngine=options;
    if(optionsEngine!=NULL)
    {
        QList<QPair<QString, QVariant> > KeysList;
        KeysList.append(qMakePair(QStringLiteral("keySequence"),QString()));
        optionsEngine->addOptionGroup(KeysList);
        optionsWidget.setKeyBind(QKeySequence::fromString(optionsEngine->getOptionValue("keySequence").toString()));
    }
}

/// \brief to get the options widget, NULL if not have
QWidget * KeyBindPlugin::options()
{
    return &optionsWidget;
}

void KeyBindPlugin::newLanguageLoaded()
{
    optionsWidget.retranslate();
}

/// \brief try enable/disable the catching
void KeyBindPlugin::setEnabled(const bool &needBeRegistred)
{
    Q_UNUSED(needBeRegistred);
}

void KeyBindPlugin::setKeyBind(const QKeySequence &keySequence)
{
    optionsEngine->setOptionValue("keySequence",keySequence);
}

