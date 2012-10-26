/** \file interface.h
\brief Define the interface
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef INTERFACE_H
#define INTERFACE_H

#include <QObject>
#include <QWidget>
#include <QMenu>
#include <QCloseEvent>
#include <QShortcut>
#include <QItemSelectionModel>
#include <QTimer>

#include "../../../interface/PluginInterface_Themes.h"

#include "ui_interface.h"
#include "ui_options.h"
#include "Environment.h"
#include "TransferModel.h"

// for windows progress bar
#ifndef __GNUC__
#include <shobjidl.h>
#endif

namespace Ui {
    class interfaceCopy;
}

/// \brief Define the interface
class Themes : public PluginInterface_Themes
{
    Q_OBJECT
public:
    Themes(bool checkBoxShowSpeed,FacilityInterface * facilityEngine,bool moreButtonPushed);
    ~Themes();
    //send information about the copy
    /// \brief to set the action in progress
    void actionInProgess(const Ultracopier::EngineActionInProgress &);
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
    //speed limitation
    /** \brief the max speed used
     * in byte per seconds, -1 if not able, 0 if disabled */
    bool setSpeedLimitation(const qint64 &speedLimitation);
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
    //edit the transfer list
    /// \brief get action on the transfer list (add/move/remove)
    void getActionOnList(const QList<Ultracopier::ReturnActionOnCopyList> &returnActions);
    /** \brief set if the order is external (like file manager copy)
     * to notify the interface, which can hide add folder/filer button */
    void haveExternalOrder();
    /// \brief set if is in pause
    void isInPause(const bool &);
    /// \brief get the widget for the copy engine
    QWidget * getOptionsEngineWidget();
    /// \brief to set if the copy engine is found
    void getOptionsEngineEnabled(const bool &isEnabled);
    enum status{status_never_started,status_started,status_stopped};
    status stat;
public slots:
    /// \brief set the translate
    void newLanguageLoaded();
private slots:
    void on_putOnTop_clicked();
    void on_pushUp_clicked();
    void on_pushDown_clicked();
    void on_putOnBottom_clicked();
    void on_del_clicked();
    void on_cancelButton_clicked();
    void on_checkBoxShowSpeed_toggled(bool checked);
    void on_SliderSpeed_valueChanged(int value);
    void on_pauseButton_clicked();
    void on_skipButton_clicked();
    void forcedModeAddFile();
    void forcedModeAddFolder();
    void forcedModeAddFileToCopy();
    void forcedModeAddFolderToCopy();
    void forcedModeAddFileToMove();
    void forcedModeAddFolderToMove();
    void uiUpdateSpeed();
    void on_pushButtonCloseSearch_clicked();
    //close the search box
    void closeTheSearchBox();
    //search box shortcut
    void searchBoxShortcut();
    //hilight the search
    void hilightTheSearch(bool searchNext=false);
    void hilightTheSearchSlot();
    //auto connect
    void on_pushButtonSearchPrev_clicked();
    void on_pushButtonSearchNext_clicked();
    void on_lineEditSearch_returnPressed();
    void on_lineEditSearch_textChanged(QString text);
    void on_moreButton_toggled(bool checked);
    void on_comboBox_copyErrors_currentIndexChanged(int index);
    void on_comboBox_fileCollisions_currentIndexChanged(int index);
    void on_searchButton_toggled(bool checked);
    void on_exportTransferList_clicked();
    void on_importTransferList_clicked();
private:
    Ui::interfaceCopy *ui;
    quint64 currentFile;
    quint64 totalFile;
    quint64 currentSize;
    quint64 totalSize;
    void updateOverallInformation();
    void updateCurrentFileInformation();
    QMenu *menu;
    Ultracopier::EngineActionInProgress action;
    void closeEvent(QCloseEvent *event);
    qint64 currentSpeed;///< in KB/s, assume as 0KB/s as default like every where
    void updateSpeed();
    bool storeIsInPause;
    bool modeIsForced;
    Ultracopier::CopyType type;
    Ultracopier::CopyMode mode;
    void updateModeAndType();
    bool haveStarted;
    bool haveError;
    QWidget optionEngineWidget;
    QShortcut *searchShortcut;
    QShortcut *searchShortcut2;
    QShortcut *searchShortcut3;
    QTimer *TimerForSearch;
    int currentIndexSearch;		///< Current index search in starting at the end
    FacilityInterface * facilityEngine;
    QIcon player_play,player_pause;
    QItemSelectionModel *selectionModel;
    QModelIndexList selectedItems;
    //temp variables
    int loop_size,loop_sub_size,index,indexAction;
    QList<int> ids;
    quint64 baseRow,addRow,removeRow;
    /// \brief the custom transfer model
    TransferModel transferModel;
    /** \brief drag event processing

    need setAcceptDrops(true); into the constructor
    need implementation to accept the drop:
    void dragEnterEvent(QDragEnterEvent* event);
    void dragMoveEvent(QDragMoveEvent* event);
    void dragLeaveEvent(QDragLeaveEvent* event);
    */
    void dropEvent(QDropEvent *event);
    /** \brief accept all event to allow the drag and drop
      \see dropEvent() */
    void dragEnterEvent(QDragEnterEvent* event);
    /** \brief accept all event to allow the drag and drop
      \see dropEvent() */
    void dragMoveEvent(QDragMoveEvent* event);
    /** \brief accept all event to allow the drag and drop
      \see dropEvent() */
    void dragLeaveEvent(QDragLeaveEvent* event);
    //have functionality
    bool shutdown;
    void updatePause();
signals:
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne);
};

#endif // INTERFACE_H
