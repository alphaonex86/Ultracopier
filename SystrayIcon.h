/** \file SystrayIcon.cpp
\brief Define the class of the systray icon
\author alpha_one_x86
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#ifndef SYSTRAY_ICON_H
#define SYSTRAY_ICON_H

#include <QSystemTrayIcon>
#include <QObject>
#include <QAction>
#include <QMenu>

#include "Environment.h"
#include "GlobalClass.h"

/** \brief The systray icon

This class provide a systray icon and its functions */
class SystrayIcon : public QObject, public GlobalClass
{
	Q_OBJECT
	public:
		/// \brief Initiate and show the icon in the systray
		SystrayIcon();
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
		void listenerReady(ListeningState state,bool havePlugin,bool someAreInWaitOfReply);
		void pluginLoaderReady(CatchState state,bool havePlugin,bool someAreInWaitOfReply);
		void addCopyEngine(QString name,bool canDoOnlyCopy);
		void removeCopyEngine(QString name);
	private:
		QSystemTrayIcon* sysTrayIcon;		///< Pointer on the systray icon
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
		ListeningState stateListener;
		CatchState statePluginLoader;
		bool haveListenerInfo,havePluginLoaderInfo;
		bool haveListener,havePluginLoader;
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
		void addWindowCopyMove(CopyMode mode,QString name);
		void addWindowTransfer(QString name);
};

#endif // SYSTRAY_ICON_H

