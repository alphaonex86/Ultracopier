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
	void newCopy(const quint32 &id,const QStringList &sources,const QString &destination);
	void newMove(const quint32 &id,const QStringList &sources,const QString &destination);
public slots:
	Q_SCRIPTABLE void copy(const QStringList &sources,const QString &destination);
	Q_SCRIPTABLE void move(const QStringList &sources,const QString &destination);
};

#endif // CATCHCOPY_H
