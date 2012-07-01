#include "Catchcopy.h"

Catchcopy::Catchcopy()
{
}

void Catchcopy::copy(QStringList sources,QString destination)
{
	emit newCopy(0,sources,destination);
}

void Catchcopy::move(QStringList sources,QString destination)
{
	emit newMove(0,sources,destination);
}
