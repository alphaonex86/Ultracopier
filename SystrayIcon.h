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

#ifdef Q_OS_WIN32
#define TREEMENU
#endif

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
        void showSystrayMessage(const std::string& text);
        /** \brief Send that's caught state have changed for CatchedState::Uncatched or CatchedState::Semicatched or CatchedState::Catched
        \see CatchState
        \see tryCatchCopy()
        \see tryUncatchCopy()
        \param state is the new state */
        void listenerReady(const Ultracopier::ListeningState &state,const bool &havePlugin,const bool &someAreInWaitOfReply);
        void pluginLoaderReady(const Ultracopier::CatchState &state,const bool &havePlugin,const bool &someAreInWaitOfReply);
        void addCopyEngine(const std::string &name,const bool &canDoOnlyCopy);
        void removeCopyEngine(const std::string &name);
        #ifdef ULTRACOPIER_INTERNET_SUPPORT
        void newUpdate(const std::string &version);
        #endif
        void changeToUltimate();
    private:
        #ifdef ULTRACOPIER_INTERNET_SUPPORT
        std::string lastVersion;
        #endif
        QMenu* systrayMenu;			///< Pointer on the menu
        #ifdef TREEMENU
        QMenu* copyMenu;			///< Pointer on the copy menu (move or copy)
        #else
        std::vector<QAction*> actions;
        #endif
        QAction* actionMenuQuit;		///< Pointer on the Quit action
        #ifdef ULTRACOPIER_DEBUG
        QAction* actionSaveBugReport;
        #endif
        QAction* actionMenuAbout;		///< Pointer on the About action
        QAction* actionOptions;			///< Pointer on the Options action
        QAction* actionProductKey;			///< Pointer on the Options action
        QIcon IconQuit;			///< Pointer on the icon for quit
        #ifdef ULTRACOPIER_DEBUG
        QIcon IconSaveBugReport;
        #endif
        QIcon IconInfo;			///< Pointer on the icon for info
        QIcon IconAdd;				///< Pointer on the icon for add
        QIcon IconOptions;			///< Pointer on the options
        /// \brief To update the systray icon
        void updateSystrayIcon();
        void showTryCatchMessageWithNoListener();
        struct EngineEntry
        {
            bool canDoOnlyCopy;
            std::string name;
        };
        std::vector<EngineEntry> engineEntryList;
        // To store the current catch state
        Ultracopier::ListeningState stateListener;
        Ultracopier::CatchState statePluginLoader;
        bool haveListenerInfo,havePluginLoaderInfo;
        bool haveListener,havePluginLoader;
        QTimer timerCheckSetTooltip;

        void addEngineAction(const QString &name, const QIcon &icon, const QString &label, QMenu *menu, void (SystrayIcon::*query)());

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
        #ifdef ULTRACOPIER_INTERNET_SUPPORT
        void messageClicked();
        #endif
    signals:
        /// \brief Quit ultracopier
        void quit() const;
        /// \brief Try catch the copy/move with plugin compatible
        void tryCatchCopy() const;
        /// \brief Try uncatch the copy/move with plugin compatible
        void tryUncatchCopy() const;
        /// \brief Show the help dialog
        void showHelp() const;
        /// \brief Show the help option
        void showOptions() const;
        /// \brief Show the help option
        void showProductKey() const;
        /** \brief Add window copy or window move
        \param mode Can be CopyMode::Copy or CopyMode::Move
        \return The core object of the new window created */
        void addWindowCopyMove(Ultracopier::CopyMode mode,std::string name) const;
        void addWindowTransfer(std::string name) const;
        void urlDropped(std::vector<std::string> urls) const;
        void saveBugReport() const;
};

#endif // SYSTRAY_ICON_H
