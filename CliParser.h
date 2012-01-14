#ifndef CLIPARSER_H
#define CLIPARSER_H

#include <QObject>
#include <QMessageBox>

class CliParser : public QObject
{
    Q_OBJECT
public:
    explicit CliParser(QObject *parent = 0);

signals:

public slots:
	void cli(QStringList ultracopierArguments,bool external);
};

#endif // CLIPARSER_H
