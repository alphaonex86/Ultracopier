#ifndef CATCHCOPY_H
#define CATCHCOPY_H

#include <QObject>
#include <QStringList>

class Catchcopy : public QObject
{
	Q_OBJECT
public:
	explicit Catchcopy();
signals:
	void newCopy(quint32 id,QStringList sources,QString destination);
	void newMove(quint32 id,QStringList sources,QString destination);
public slots:
	Q_SCRIPTABLE void copy(QStringList sources,QString destination);
	Q_SCRIPTABLE void move(QStringList sources,QString destination);
};

#endif // CATCHCOPY_H
