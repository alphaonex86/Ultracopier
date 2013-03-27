/** \file interface.h
\brief Define the interface test
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef INTERFACE_H
#define INTERFACE_H

#include <QObject>
#include <QWidget>
#include <QCloseEvent>
#include <QMenu>
#include <QColor>

#include "../../../interface/PluginInterface_Themes.h"

#include "ui_interface.h"
#include "Environment.h"
#include "TransferModel.h"

namespace Ui {
    class interfaceCopy;
}

/// \brief Define the interface
class Themes : public PluginInterface_Themes
{
    Q_OBJECT
private:
    struct ItemOfCopyListWithMoreInformations
    {
        quint64 currentProgression;
        Ultracopier::ItemOfCopyList generalData;
        Ultracopier::ActionTypeCopyList actionType;
        bool custom_with_progression;
    };
    struct currentTransfertItem
    {
        quint64 id;
        bool haveItem;
        QString from;
        QString to;
        QString current_file;
        int progressBar_file;
    };
    Ui::interfaceCopy *ui;
    quint64 currentFile;
    quint64 totalFile;
    quint64 currentSize;
    quint64 totalSize;
    void updateOverallInformation();
    void updateCurrentFileInformation();
    Ultracopier::EngineActionInProgress action;
    void closeEvent(QCloseEvent *event);
    QList<ItemOfCopyListWithMoreInformations> currentProgressList;
    QString speedString;
    bool storeIsInPause;
    bool modeIsForced;
    Ultracopier::CopyType type;
    Ultracopier::CopyMode mode;
    void updateModeAndType();
    bool haveStarted;
    QWidget optionEngineWidget;
    void updateTitle();
    QMenu menu;
    FacilityInterface * facilityEngine;
    int loop_size,loop_sub_size,indexAction,index;
    int index_for_loop,sub_loop_size,sub_index_for_loop;
    currentTransfertItem getCurrentTransfertItem();
    QList<quint64> startId,stopId;///< To show what is started, what is stopped
    QList<ItemOfCopyListWithMoreInformations> InternalRunningOperation;///< to have progression and stat
    /// \brief the custom transfer model
    TransferModel transferModel;
    QColor progressColorWrite;
    QColor progressColorRead;
    QColor progressColorRemaining;
public:
    //send information about the copy
    /// \brief to set the action in progress
    void actionInProgess(const Ultracopier::EngineActionInProgress &);
    /// \brief new transfer have started
    void newTransferStart(const Ultracopier::ItemOfCopyList &item);
    /** \brief one transfer have been stopped
     * is stopped, example: because error have occurred, and try later, don't remove the item! */
    void newTransferStop(const quint64 &id);
    /// \brief the new folder is listing
    void newFolderListing(const QString &path);
    /** \brief show the detected speed
     * in byte per seconds */
    void detectedSpeed(const quint64 &speed);
    /** \brief support speed limitation */
    void setSupportSpeedLimitation(const bool &supportSpeedLimitationBool);
    /** \brief show the remaining time
     * time in seconds */
    void remainingTime(const int &remainingSeconds);
    /// \brief set the current collision action
    void newCollisionAction(const QString &action);
    /// \brief set the current error action
    void newErrorAction(const QString &action);
    /// \brief set one error is detected
    void errorDetected();
    //speed limitation
    /** \brief the max speed used
     * in byte per seconds, -1 if not able, 0 if disabled */
    bool setSpeedLimitation(const qint64 &speedLimitation);
    Themes(FacilityInterface * facilityEngine);
    ~Themes();
    //get information about the copy
    /// \brief show the general progression
    void setGeneralProgression(const quint64 &current,const quint64 &total);
    /// \brief show the file progression
    void setFileProgression(const QList<Ultracopier::ProgressionItem> &progressionList);
    /// \brief set collision action
    void setCollisionAction(const QList<QPair<QString,QString> > &);
    /// \brief set error action
    void setErrorAction(const QList<QPair<QString,QString> > &);
    /// \brief set the copyType -> file or folder
    void setCopyType(const Ultracopier::CopyType &);
    /// \brief set the copyMove -> copy or move, to force in copy or move, else support both
    void forceCopyMode(const Ultracopier::CopyMode &);
    /// \brief set if transfer list is exportable/importable
    void setTransferListOperation(const Ultracopier::TransferListOperation &transferListOperation);
    /** \brief set if the order is external (like file manager copy)
     * to notify the interface, which can hide add folder/filer button */
    void haveExternalOrder();
    /// \brief set if is in pause
    void isInPause(const bool &);
    /// \brief get the widget for the copy engine
    QWidget * getOptionsEngineWidget();
    /// \brief to set if the copy engine is found
    void getOptionsEngineEnabled(const bool &isEnabled);
private slots:
    void on_cancelButton_clicked();
    void on_pauseButton_clicked();
    void on_skipButton_clicked();
    void forcedModeAddFile();
    void forcedModeAddFolder();
    void forcedModeAddFileToCopy();
    void forcedModeAddFolderToCopy();
    void forcedModeAddFileToMove();
    void forcedModeAddFolderToMove();
signals:
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne);
    #endif
    //set the transfer list
    /*void removeItems(QList<int> ids);
    void moveItemsOnTop(QList<int> ids);
    void moveItemsUp(QList<int> ids);
    void moveItemsDown(QList<int> ids);
    void moveItemsOnBottom(QList<int> ids);
    void exportTransferList();
    void importTransferList();
    //user ask ask to add folder (add it with interface ask source/destination)
    void userAddFolder(Ultracopier::CopyMode);
    void userAddFile(Ultracopier::CopyMode);
    void urlDropped(QList<QUrl> urls);
    //action on the copy
    void pause();
    void resume();
    void skip(quint64 id);
    void cancel();
    //edit the action
    void sendCollisionAction(QString action);
    void sendErrorAction(QString action);
    void newSpeedLimitation(qint64);*/
public slots:
    //set the translate
    void newLanguageLoaded();
    void getActionOnList(const QList<Ultracopier::ReturnActionOnCopyList>& returnActions);
};

#endif // INTERFACE_H
