#ifndef CLIPARSER_H
#define CLIPARSER_H

#include <QObject>
#include <QMessageBox>

/** \brief class to parse all command line options */
class CliParser : public QObject
{
	Q_OBJECT
public:
	explicit CliParser(QObject *parent = 0);
public slots:
	/** \brief method to parse the ultracopier arguments
	  \param ultracopierArguments the argument list
	  \param external true if the arguments come from other instance of ultracopier
	*/
	void cli(QStringList ultracopierArguments,bool external);
};

#endif // CLIPARSER_H
