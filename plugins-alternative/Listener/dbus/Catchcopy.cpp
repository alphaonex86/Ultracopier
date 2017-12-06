#include "Catchcopy.h"

#include <QCoreApplication>

Catchcopy::Catchcopy()
{
}

void Catchcopy::copy(const QStringList &sources,const QString &destination)
{
	emit newCopy(0,sources,destination);
}

void Catchcopy::move(const QStringList &sources,const QString &destination)
{
	emit newMove(0,sources,destination);
}
