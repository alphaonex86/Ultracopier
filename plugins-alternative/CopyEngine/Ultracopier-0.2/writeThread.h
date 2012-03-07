/***************************************************************************
				  writeThread.h
			      -------------------

     Begin        : Web May 6 2009 19:11 alpha_one_x86
     Project      : Ultracopier
     Email        : ultracopier@first-world.info
     Note         : See README for copyright and developer
     Target       : Define the class of the writeThread

****************************************************************************/

/** \file writeThread.h
\brief Thread changed write to the destination file
\author alpha_one_x86
\version 0.3
\date 2011 */

#include <QObject>
#include <QThread>
#include <QByteArray>
#include <QFile>
#include <QObject>
#include <QList>
#include <QMutex>
#include <QWaitCondition>
#include <QFileInfo>
#include <QSemaphore>
#include <QDateTime>

#include "Environment.h"
#include "AvancedQFile.h"

#define LISTBLOCKSIZE 64

#ifndef WRITETHREAD_H
#define WRITETHREAD_H

/// \brief Thread changed write to the destination file
class writeThread : public QThread
{
	Q_OBJECT
	public:
		writeThread(QObject * parent = 0);
		~writeThread();
		/// \brief set the files
		void setFiles(QString sourcePath,QString destinationPath);
		/// \brief try open the destination
		QString openDestination();
		/// \brief stop all
		void stop();
		/// \brief write a new block
		void addNewBlock(QByteArray newBlock);
		/// \brief keep date
		void setKeepDate(bool keepDate);
		/// \brief preallocate the file
		void setPreallocation(bool preallocation);
		/// \brief skip the current file
		void skipTheCurrentFile();
		/// \brief the current stat
		enum currentStat {
		NotRunning,
		WaitingTask,
		Running,
		Terminated
		};
		/// \brief return the statut
		currentStat getStatus();
		/// \brief set action on error
		void errorAction(FileErrorAction action);
		/// \brief set if is moving mode or not
		void setMovingMode(CopyMode mode);
		/// \brief end of source detected
		void endOfSourceDetected();
		void stopWhenIsFinish(bool stopThreadWhenFinish);
		#ifdef DEBUG_WINDOW
		volatile bool isBlocked;
		#endif // DEBUG_WINDOW
	protected:
		void run();
	private:
		volatile bool			stopIt;			///< For store the stop query
		volatile bool			movingMode;		///< Set if is in moving mode
		volatile FileErrorAction	fileErrorActionSuspended;	///< For action after unlock the mutex
		volatile bool			keepDate;		///< For store if date need be keep
		volatile bool			endOfSource;		///< For store if end os source
		volatile bool			preallocation;		///< For store if preallocation is needed
		volatile bool			stopThreadWhenFinish;	///< For stop thread when the thread is finish
		volatile bool			needSkipTheCurrentFile;	///< For stop thread when the thread is finish
		QMutex				accessList;		///< For use the list
		QSemaphore			freeBlock;
		QSemaphore			usedBlock;
		QSemaphore			waitDestionFile;
		QSemaphore			waitErrorAction;
		QFile				source;			///< For have global transfer progression
		AvancedQFile			destination;		///< For have global transfer progression
		QList<QByteArray>		theBlockList;		///< Store the block list
		int				sizeList;
		quint64				CurentCopiedSize;
		currentStat			status;
		CopyMode			mode;
		/// \brief error on file or folder
		FileErrorAction sendErrorOnFile(QFileInfo fileInfo,QString errorString);
		void resetSemaphore();
	signals:
		void haveStartFileOperation();
		void haveFinishFileOperation();
		/** \brief send the new error and wait return
		 * \see errorAction() to the return error code
		 * The current thread should be blocked into \see errorOnFile() */
		void errorOnFile(QFileInfo,QString);
		/// \brief To debug source
		void debugInformation(DebugLevel level,QString fonction,QString text,QString file,int ligne);
};

#endif // WRITETHREAD_H
