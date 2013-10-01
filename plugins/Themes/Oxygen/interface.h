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
#include <QTime>
#include <QSystemTrayIcon>
#include <QPixmap>

#include "../../../interface/PluginInterface_Themes.h"

#include "ui_interface.h"
#include "ui_themesOptions.h"
#include "Environment.h"
#include "TransferModel.h"

// for windows progress bar
#ifndef __GNUC__
#	include <shobjidl.h>
#endif

namespace Ui {
    class interfaceCopy;
    class themesOptions;
}

/// \brief Define the interface
class Themes : public PluginInterface_Themes
{
    Q_OBJECT
public:
    Themes(const bool &alwaysOnTop,
           const bool &showProgressionInTheTitle,
           const QColor &progressColorWrite,
           const QColor &progressColorRead,
           const QColor &progressColorRemaining,
           const bool &showDualProgression,
           const quint8 &comboBox_copyEnd,
           const bool &speedWithProgressBar,
           const qint32 &currentSpeed,
           const bool &checkBoxShowSpeed,
           FacilityInterface * facilityEngine,
           const bool &moreButtonPushed,
           const bool &minimizeToSystray);
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
    /// \brief new error
    void errorToRetry(const QString &source,const QString &destination,const QString &error);
    /** \brief support speed limitation */
    void setSupportSpeedLimitation(const bool &supportSpeedLimitationBool);
    //get information about the copy
    /// \brief show the general progression
    void setGeneralProgression(const quint64 &current,const quint64 &total);
    /// \brief show the file progression
    void setFileProgression(const QList<Ultracopier::ProgressionItem> &progressionList);
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
    void speedWithProgressBar_toggled(bool checked);
    void showDualProgression_toggled(bool checked);
    void checkBoxShowSpeed_toggled(bool checked);
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
    void on_searchButton_toggled(bool checked);
    void on_exportTransferList_clicked();
    void on_importTransferList_clicked();
    void progressColorWrite_clicked();
    void progressColorRead_clicked();
    void progressColorRemaining_clicked();
    void alwaysOnTop_clicked(bool reshow);
    void alwaysOnTop_clickedSlot();
    void updateProgressionColorBar();
    void updateTitle();
    void catchAction(QSystemTrayIcon::ActivationReason reason);
    void on_exportErrorToTransferList_clicked();
private:
    QTime duration;
    bool durationStarted;
    QPixmap pixmapTop,pixmapBottom;
    QColor progressColorWrite,progressColorRead,progressColorRemaining;
    Ui::interfaceCopy *ui;
    Ui::themesOptions *uiOptions;
    quint64 currentFile;
    quint64 totalFile;
    quint64 currentSize;
    quint64 totalSize;
    quint8 getOldProgression;
    QSystemTrayIcon *sysTrayIcon;
    void updateOverallInformation();
    void updateCurrentFileInformation();
    QMenu *menu;
    Ultracopier::EngineActionInProgress action;
    void closeEvent(QCloseEvent *event);
    qint32 currentSpeed;///< in KB/s, assume as 0KB/s as default like every where
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
    QIcon dynaIcon(int percent,QString text="") const;
    void updateSysTrayIcon();
signals:
    /// \brief To debug source
    void debugInformation(const Ultracopier::DebugLevel &level,const QString &fonction,const QString &text,const QString &file,const int &ligne) const;
};

#endif // INTERFACE_H
