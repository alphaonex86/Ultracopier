/** \file DebugEngine.h
\brief Define the class for the debug
\author alpha_one_x86
\note This class don't need be thread safe because ultracopier is done with one thread, but I have implement some basic thread protection
\licence GPL3, see the file COPYING */

#ifndef DEBUG_ENGINE_H
#define DEBUG_ENGINE_H

#include <QObject>
#include <QString>
#include <string>
#include <QFile>
#include <QMutex>
#include <QTime>
#include <QTimer>
#include <QList>
#include <QCoreApplication>
#include <QAbstractTableModel>
#include <QRegularExpression>
#include <regex>

#include "Variable.h"
#include "PlatformMacro.h"
#include "StructEnumDefinition.h"
#include "StructEnumDefinition_UltracopierSpecific.h"

#ifdef ULTRACOPIER_DEBUG

class DebugModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    /// \brief the transfer item displayed
    struct DebugItem
    {
        int time;
        DebugLevel_custom level;
        std::string function;
        std::string text;
        std::string file;
        std::string location;
    };

    static DebugModel *debugModel;
    DebugModel();
    ~DebugModel();

    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex&, const QVariant&, int = Qt::EditRole);

    void addDebugInformation(const int &time, const DebugLevel_custom &level, const std::string& function, const std::string& text, const std::string &file="", const int& ligne=-1, const std::string& location="Core");
    void setupTheTimer();
    QTimer *updateDisplayTimer;
    bool displayed;
    bool inWaitOfDisplay;
private:
    std::vector<DebugItem> list;
private slots:
    void updateDisplay();
};

/** \brief Define the class for the debug

This class provide all needed for the debug mode of ultracopier */
class DebugEngine : public QObject
{
    Q_OBJECT
    public:
        /// \brief Initiate the ultracopier event dispatcher and check if no other session is running
        DebugEngine();
        /** \brief Destroy the ultracopier event dispatcher
        \note This function is thread safe */
        ~DebugEngine();
        /** \brief Get the html text info for re-show it
        \note This function is thread safe */
        std::string getTheDebugHtml();
        /// \brief Enumeration of backend
        enum Backend
        {
            Memory,		//Do intensive usage of memory, used only if the file backend is not available
            File		//Store all directly into file, at crash the backtrace is into the file
        };
        /// \brief return the current backend
        Backend getCurrentBackend();
        /// \brief Get the html end
        std::string getTheDebugEnd();
        /** \brief For add message info, this function
        \note This function is reentrant */
        static void addDebugInformationStatic(const Ultracopier::DebugLevel &level,const std::string& function,const std::string& text,const std::string& file="",const int& ligne=-1,const std::string& location="Core");
        static void addDebugNote(const std::string& text);
        static DebugEngine *debugEngine;
    public slots:
        /** \brief ask to the user where save the bug report
        \warning This function can be only call by the graphical thread */
        void saveBugReport();
        void addDebugInformation(const DebugLevel_custom &level,const std::string& fonction,const std::string& text,std::string file="",const int& ligne=-1,const std::string& location="Core");
    private:
        /// \brief Path for log file
        QFile logFile;
        /// \brief Path for lock file
        QFile lockFile;
        /// \brief Internal function to remove the lock file
        bool removeTheLockFile();
        /** \brief Do thread safe part for the addDebugInformation()
        \see addDebugInformation() */
        QMutex mutex;
        QMutex mutexList;
        /// \brief For record the start time
        QTime startTime;
        /// \brief String for the end of log file
        std::string endOfLogFile;
        /// \brief Drop the html entities
        std::string htmlEntities(const std::string &text);
        /// \brief To store the debug informations
        std::string debugHtmlContent;
        /// \brief The current backend
        Backend currentBackend;
        /// try connect to send to the current running instance the arguements
        bool tryConnect();
        int addDebugInformationCallNumber;
        bool quit;
        std::regex fileNameCleaner;
};

#endif // ULTRACOPIER_DEBUG

#endif // DEBUG_ENGINE_H
