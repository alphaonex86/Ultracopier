/** \file SystrayIcon.h
\brief Define the class of the systray icon
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef SYSTRAY_ICON_H
#define SYSTRAY_ICON_H

#include <QSystemTrayIcon>
#include <QObject>
#include <QAction>
#include <QMenu>
#include <QTimer>
#include <QDropEvent>
#include <QList>
#include <QUrl>

#include "Environment.h"

/** \brief The systray icon

This class provide a systray icon and its functions */
class SystrayIcon : public QSystemTrayIcon
{
    Q_OBJECT
    public:
        /// \brief Initiate and show the icon in the systray
        SystrayIcon(QObject * parent = 0);
        /// \brief Hide and destroy the icon in the systray
        ~SystrayIcon();
    public slots:
        /// \brief For show a message linked to the systray icon
        void showSystrayMessage(const QString& text);
        /** \brief Send that's caught state have changed for CatchedState::Uncatched or CatchedState::Semicatched or CatchedState::Catched
        \see CatchState
        \see tryCatchCopy()
        \see tryUncatchCopy()
        \param state is the new state */
        void listenerReady(const Ultracopier::ListeningState &state,const bool &havePlugin,const bool &someAreInWaitOfReply);
        void pluginLoaderReady(const Ultracopier::CatchState &state,const bool &havePlugin,const bool &someAreInWaitOfReply);
        void addCopyEngine(const QString &name,const bool &canDoOnlyCopy);
        void removeCopyEngine(const QString &name);
        #ifdef ULTRACOPIER_INTERNET_SUPPORT
        void newUpdate(const QString &version);
        #endif
    private:
        #ifdef ULTRACOPIER_INTERNET_SUPPORT
        QString lastVersion;
        #endif
        QMenu* systrayMenu;			///< Pointer on the menu
        QMenu* copyMenu;			///< Pointer on the copy menu (move or copy)
        QAction* actionMenuQuit;		///< Pointer on the Quit action
        QAction* actionMenuAbout;		///< Pointer on the About action
        QAction* actionOptions;			///< Pointer on the Options action
        QIcon IconQuit;			///< Pointer on the icon for quit
        QIcon IconInfo;			///< Pointer on the icon for info
        QIcon IconAdd;				///< Pointer on the icon for add
        QIcon IconOptions;			///< Pointer on the options
        /// \brief To update the systray icon
        void updateSystrayIcon();
        void showTryCatchMessageWithNoListener();
        struct EngineEntry
        {
            bool canDoOnlyCopy;
            QString name;
        };
        QList<EngineEntry> engineEntryList;
        // To store the current catch state
        Ultracopier::ListeningState stateListener;
        Ultracopier::CatchState statePluginLoader;
        bool haveListenerInfo,havePluginLoaderInfo;
        bool haveListener,havePluginLoader;
        QTimer timerCheckSetTooltip;
        /** \brief drag event processing (impossible with Qt on systray)

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
    private slots:
        /// \brief To update the current themes
        void updateCurrentTheme();
        /** \brief To catch an action on the systray icon
        \param reason Why it activated */
        void CatchAction(QSystemTrayIcon::ActivationReason reason);
        /// \brief To catch copy menu action
        void CatchCopyQuery();
        /// \brief To catch move menu action
        void CatchMoveQuery();
        /// \brief To catch transfer menu action
        void CatchTransferQuery();
        /// \brief to retranslate the ui
        void retranslateTheUI();
        void reloadEngineList();
        void checkSetTooltip();
    signals:
        /// \brief Quit ultracopier
        void quit();
        /// \brief Try catch the copy/move with plugin compatible
        void tryCatchCopy();
        /// \brief Try uncatch the copy/move with plugin compatible
        void tryUncatchCopy();
        /// \brief Show the help dialog
        void showHelp();
        /// \brief Show the help option
        void showOptions();
        /** \brief Add window copy or window move
        \param mode Can be CopyMode::Copy or CopyMode::Move
        \return The core object of the new window created */
        void addWindowCopyMove(Ultracopier::CopyMode mode,QString name);
        void addWindowTransfer(QString name);
        void urlDropped(QList<QUrl> urls);
};

#endif // SYSTRAY_ICON_H

