/** \file DebugEngine.h
\brief Define the class for the debug
\author alpha_one_x86
\note This class don't need be thread safe because ultracopier is done with one thread, but I have implement some basic thread protection
\licence GPL3, see the file COPYING */

#ifndef DEBUG_ENGINE_H
#define DEBUG_ENGINE_H

#include <QObject>
#include <QString>
#include <QFile>
#include <QMutex>
#include <QTime>
#include <QTimer>
#include <QList>
#include <QCoreApplication>
#include <QAbstractTableModel>

#include "Variable.h"
#include "PlatformMacro.h"
#include "StructEnumDefinition.h"
#include "StructEnumDefinition_UltracopierSpecific.h"
#include "Singleton.h"

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
        QString function;
        QString text;
        QString file;
        QString location;
    };

    static DebugModel debugModel;
    DebugModel();
    ~DebugModel();

    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex&, const QVariant&, int = Qt::EditRole);

    void addDebugInformation(const int &time, const DebugLevel_custom &level, const QString& function, const QString& text, const QString &file="", const int& ligne=-1, const QString& location="Core");
    void setupTheTimer();
    QTimer *updateDisplayTimer;
    bool displayed;
    bool inWaitOfDisplay;
private:
    QList<DebugItem> list;
private slots:
    void updateDisplay();
};

/** \brief Define the class for the debug

This class provide all needed for the debug mode of ultracopier */
class DebugEngine : public QObject, public Singleton<DebugEngine>
{
    Q_OBJECT
    friend class Singleton<DebugEngine>;
    public:
        /** \brief Get the html text info for re-show it
        \note This function is thread safe */
        QString getTheDebugHtml();
        /// \brief Enumeration of backend
        enum Backend
        {
            Memory,		//Do intensive usage of memory, used only if the file backend is not available
            File		//Store all directly into file, at crash the backtrace is into the file
        };
        /// \brief return the current backend
        Backend getCurrentBackend();
        /// \brief Get the html end
        QString getTheDebugEnd();
        /** \brief For add message info, this function
        \note This function is reentrant */
        static void addDebugInformationStatic(const Ultracopier::DebugLevel &level,const QString& function,const QString& text,const QString& file="",const int& ligne=-1,const QString& location="Core");
        static void addDebugNote(const QString& text);
    public slots:
        /** \brief ask to the user where save the bug report
        \warning This function can be only call by the graphical thread */
        void saveBugReport();
        void addDebugInformation(const DebugLevel_custom &level,const QString& fonction,const QString& text,QString file="",const int& ligne=-1,const QString& location="Core");
    private:
        /// \brief Initiate the ultracopier event dispatcher and check if no other session is running
        DebugEngine();
        /** \brief Destroy the ultracopier event dispatcher
        \note This function is thread safe */
        ~DebugEngine();
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
        QString endOfLogFile;
        /// \brief Drop the html entities
        QString htmlEntities(const QString &text);
        /// \brief To store the debug informations
        QString debugHtmlContent;
        /// \brief The current backend
        Backend currentBackend;
        /// try connect to send to the current running instance the arguements
        bool tryConnect();
        //temp variable
        /* can't because multiple thread can access at this variable
        QString addDebugInformation_lignestring;
        QString addDebugInformation_fileString;
        QString addDebugInformation_time;
        QString addDebugInformation_htmlFormat;
        QString addDebugInformation_textFormat;*/
        quint32 addDebugInformationCallNumber;
};

#endif // ULTRACOPIER_DEBUG

#endif // DEBUG_ENGINE_H
