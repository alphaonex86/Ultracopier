/** \file interface.h
\brief Define the interface
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef INTERFACE_TEST_H
#define INTERFACE_TEST_H

#include <QObject>
#include <QWidget>
#include <QMenu>
#include <QCloseEvent>

#include "../../../interface/PluginInterface_Themes.h"

namespace Ui {
    class interface;
}

/// \brief Define the interface
class InterfacePlugin : public PluginInterface_Themes
{
    Q_OBJECT
public slots:
    //send information about the copy
    /// \brief to set the action in progress
    void actionInProgess(const Ultracopier::EngineActionInProgress&);
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
    /** \brief show the remaining time
     * time in seconds */
    void remainingTime(const int &remainingSeconds);
    /// \brief set the current collision action
    void newCollisionAction(const QString &action);
    /// \brief set the current error action
    void newErrorAction(const QString &action);
    /// \brief set one error is detected
    void errorDetected();
    /// \brief new error
    void errorToRetry(const QString &source,const QString &destination,const QString &error);
    //speed limitation
    /** \brief the max speed used
     * in byte per seconds, -1 if not able, 0 if disabled */
    bool setSpeedLimitation(const qint64 &speedLimitation);
    //set the translate
    void newLanguageLoaded();
    void synchronizeItems(const QList<Ultracopier::ReturnActionOnCopyList>& returnActions);
    /** \brief support speed limitation */
    void setSupportSpeedLimitation(const bool &supportSpeedLimitationBool);
public:
    /// \brief the transfer item with progression
    struct ItemOfCopyListWithMoreInformations
    {
        quint64 currentProgression;
        Ultracopier::ItemOfCopyList generalData;
        Ultracopier::ActionTypeCopyList actionType;
        bool custom_with_progression;
    };
    /// \brief returned first transfer item
    struct currentTransfertItem
    {
        quint64 id;
        bool haveItem;
        QString from;
        QString to;
        QString current_file;
        int progressBar_file;
    };
    /// \brief get the widget for the copy engine
    QWidget * getOptionsEngineWidget();
    /// \brief to set if the copy engine is found
    void getOptionsEngineEnabled(const bool &isEnabled);
    /// \brief get action on the transfer list (add/move/remove)
    void getActionOnList(const QList<Ultracopier::ReturnActionOnCopyList> &returnActions);
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
    void setCopyType(const Ultracopier::CopyType&);
    /// \brief set the copyMove -> copy or move, to force in copy or move, else support both
    void forceCopyMode(const Ultracopier::CopyMode &);
    /// \brief set if transfer list is exportable/importable
    void setTransferListOperation(const Ultracopier::TransferListOperation &transferListOperation);
    /** \brief set if the order is external (like file manager copy)
     * to notify the interface, which can hide add folder/filer button */
    void haveExternalOrder();
    /// \brief set if is in pause
    void isInPause(const bool &);
signals:
    #ifdef ULTRACOPIER_PLUGIN_DEBUG
    /// \brief To debug source
    void debugInformation(const DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne);
    #endif
public:
    //constructor and destructor
    InterfacePlugin(FacilityInterface * facilityEngine);
    ~InterfacePlugin();
private:
    Ui::interface *ui;
    quint64 currentFile;
    quint64 totalFile;
    quint64 currentSize;
    quint64 totalSize;
    void updateTitle();
    QMenu *menu;
    Ultracopier::EngineActionInProgress action;
    void closeEvent(QCloseEvent *event);
    void updateModeAndType();
    bool modeIsForced;
    Ultracopier::CopyType type;
    Ultracopier::CopyMode mode;
    bool haveStarted;
    QList<ItemOfCopyListWithMoreInformations> InternalRunningOperation;
    int loop_size,index_for_loop;
    int sub_loop_size,sub_index_for_loop;
    currentTransfertItem getCurrentTransfertItem();
    FacilityInterface * facilityEngine;
private slots:
    void forcedModeAddFile();
    void forcedModeAddFolder();
    void forcedModeAddFileToCopy();
    void forcedModeAddFolderToCopy();
    void forcedModeAddFileToMove();
    void forcedModeAddFolderToMove();
};

#endif // INTERFACE_TEST_H
