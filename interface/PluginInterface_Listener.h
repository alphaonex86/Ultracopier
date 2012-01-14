/** \file SessionLoader_Listen.h
\brief Define the interface of the plugin of type: listener
\author alpha_one_x86
\version 0.3
\date 2010 */ 

#ifndef PLUGININTERFACE_LISTENER_H
#define PLUGININTERFACE_LISTENER_H

#include <QObject>
#include <QString>
#include <QStringList>

#include "OptionInterface.h"

#include "../StructEnumDefinition.h"

class PluginInterface_Listen : public QObject
{
	Q_OBJECT
	public:
		virtual void listen() = 0;
		virtual void close() = 0;
		virtual const QString errorString() = 0;
		virtual void setResources(OptionInterface * options,QString writePath,QString pluginPath,bool portableVersion) = 0;
	public slots:
		virtual void copyFinished(quint32 orderId,bool withError) = 0;
		virtual void copyCanceled(quint32 orderId) = 0;
	/* signal to implement
	signals:
		void newState(ListeningState state);
		void newCopy(quint32 orderId,QStringList sources);
		void newCopy(quint32 orderId,QStringList sources,QString destination);
		void newMove(quint32 orderId,QStringList sources);
		void newMove(quint32 orderId,QStringList sources,QString destination);*/
};

Q_DECLARE_INTERFACE(PluginInterface_Listen,"first-world.info.ultracopier.PluginInterface.Listener/0.3.0.1");

#endif // PLUGININTERFACE_LISTENER_H
