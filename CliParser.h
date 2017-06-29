/** \file CliParser.h
\brief To group into one class, the CLI parsing
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef CLIPARSER_H
#define CLIPARSER_H

#include <QObject>
#include <QMessageBox>
#include <QCoreApplication>
#include <QFile>

#include "Environment.h"

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
    void cli(const std::vector<std::string> &ultracopierArguments,const bool &external,const bool &onlyCheck);
signals:
    /** new copy without destination have been pased by the CLI */
    void newCopyWithoutDestination(std::vector<std::string> sources) const;
    /** new copy with destination have been pased by the CLI */
    void newCopy(std::vector<std::string> sources,std::string destination) const;
    /** new move without destination have been pased by the CLI */
    void newMoveWithoutDestination(std::vector<std::string> sources) const;
    /** new move with destination have been pased by the CLI */
    void newMove(std::vector<std::string> sources,std::string destination) const;
    /** new transfer list pased by the CLI */
    void newTransferList(std::string engine,std::string mode,std::string file) const;

    void tryLoadPlugin(const std::string &file) const;
    /// \brief Show the help option
    void showOptions() const;
private:
    /** \brief show the help
     *\param incorrectArguments if the help is call because the arguments are wrong */
    void showHelp(const bool &incorrectArguments=true);
};

#endif // CLIPARSER_H
