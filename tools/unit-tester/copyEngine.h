/** \file copyEngine.h
\brief Define the copy engine
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include <QObject>
#include <QList>
#include <QStringList>
#include <QFileInfo>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>

#include "../../interface/PluginInterface_CopyEngine.h"
#include "FileErrorDialog.h"
#include "FileExistsDialog.h"
#include "FolderExistsDialog.h"
#include "FileIsSameDialog.h"
#include "ui_options.h"
#include "Environment.h"
#include "ListThread.h"
#include "Filters.h"
#include "RenamingRules.h"

#ifndef COPY_ENGINE_H
#define COPY_ENGINE_H

namespace Ui {
    class options;
}

/// \brief the implementation of copy engine plugin, manage directly few stuff, else pass to ListThread class.
class copyEngine : public PluginInterface_CopyEngine
{
        Q_OBJECT
public:
    enum SupportedTest{Test_Copy};

    copyEngine(const QString &path,const QList<SupportedTest> &tests);
    ~copyEngine();
private:
    ListThread *listThread;
    bool				dialogIsOpen;
    QList<SupportedTest> tests;
private slots:
    void fileAlreadyExistsSlot(QFileInfo source,QFileInfo destination,bool isSame,TransferThread * thread);
    void errorOnFileSlot(QFileInfo fileInfo,QString errorString,TransferThread * thread);
    void folderAlreadyExistsSlot(QFileInfo source,QFileInfo destination,bool isSame,scanFileOrFolder * thread);
    void errorOnFolderSlot(QFileInfo fileInfo,QString errorString,scanFileOrFolder * thread);
    void mkPathErrorOnFolderSlot(QFileInfo,QString);
    void rmPathErrorOnFolderSlot(QFileInfo,QString);

    //dialog message
    /// \note Can be call without queue because all call will be serialized
    void fileAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,TransferThread * thread,bool isCalledByShowOneNewDialog=false);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFile(QFileInfo fileInfo,QString errorString,TransferThread * thread,bool isCalledByShowOneNewDialog=false);
    /// \note Can be call without queue because all call will be serialized
    void folderAlreadyExists(QFileInfo source,QFileInfo destination,bool isSame,scanFileOrFolder * thread,bool isCalledByShowOneNewDialog=false);
    /// \note Can be call without queue because all call will be serialized
    void errorOnFolder(QFileInfo fileInfo,QString errorString,scanFileOrFolder * thread,bool isCalledByShowOneNewDialog=false);
    //mkpath event
    void mkPathErrorOnFolder(QFileInfo,QString,bool isCalledByShowOneNewDialog=false);
    //rmpath event
    void rmPathErrorOnFolder(QFileInfo,QString,bool isCalledByShowOneNewDialog=false);

    //show one new dialog if needed
    void showOneNewDialog();
    void sendNewFilters();

    void doChecksum_toggled(bool);
    void checksumOnlyOnError_toggled(bool);
    void checksumIgnoreIfImpossible_toggled(bool);
    void osBuffer_toggled(bool);
    void osBufferLimited_toggled(bool);
    void osBufferLimit_editingFinished();
    void showFilterDialog();
    void sendNewRenamingRules(QString firstRenamingRule,QString otherRenamingRule);
    void showRenamingRules();
    void get_realBytesTransfered(quint64 realBytesTransfered);
    void newActionInProgess(EngineActionInProgress);
    void updateBufferCheckbox();
public:
    /** \brief to send the options panel
     * \return return false if have not the options
     * \param tempWidget the widget to generate on it the options */
    bool getOptionsEngine(QWidget * tempWidget);
    /** \brief to have interface widget to do modal dialog
     * \param interface to have the widget of the interface, useful for modal dialog */
    void setInterfacePointer(QWidget * interface);
    //return empty if multiple
    /** \brief compare the current sources of the copy, with the passed arguments
     * \param sources the sources list to compares with the current sources list
     * \return true if have same sources, else false (or empty) */
    bool haveSameSource(const QStringList &sources);
    /** \brief compare the current destination of the copy, with the passed arguments
     * \param destination the destination to compares with the current destination
     * \return true if have same destination, else false (or empty) */
    bool haveSameDestination(const QString &destination);
    //external soft like file browser have send copy/move list to do
    /** \brief send copy without destination, ask the destination
     * \param sources the sources list to copy
     * \return true if the copy have been accepted */
    bool newCopy(const QStringList &sources);
    /** \brief send copy with destination
     * \param sources the sources list to copy
     * \param destination the destination to copy
     * \return true if the copy have been accepted */
    bool newCopy(const QStringList &sources,const QString &destination);
    /** \brief send move without destination, ask the destination
     * \param sources the sources list to move
     * \return true if the move have been accepted */
    bool newMove(const QStringList &sources);
    /** \brief send move without destination, ask the destination
     * \param sources the sources list to move
     * \param destination the destination to move
     * \return true if the move have been accepted */
    bool newMove(const QStringList &sources,const QString &destination);
    /** \brief send the new transfer list
     * \param file the transfer list */
    void newTransferList(const QString &file);

    /** \brief to get byte read, use by Ultracopier for the speed calculation
     * real size transfered to right speed calculation */
    quint64 realByteTransfered();
    //speed limitation
    /** \brief get the speed limitation
     * < -1 if not able, 0 if disabled */
    qint64 getSpeedLimitation();
    //get collision action
    /** \brief get the collision action list */
    QList<QPair<QString,QString> > getCollisionAction();
    /** \brief get the collision error list */
    QList<QPair<QString,QString> > getErrorAction();

    /** \brief to set drives detected
     * specific to this copy engine */
    void setDrive(const QStringList &drives);

    /** \brief to sync the transfer list
     * Used when the interface is changed, useful to minimize the memory size */
    void syncTransferList();

    void set_doChecksum(bool doChecksum);
    void set_checksumIgnoreIfImpossible(bool checksumIgnoreIfImpossible);
    void set_checksumOnlyOnError(bool checksumOnlyOnError);
    void set_osBuffer(bool osBuffer);
    void set_osBufferLimited(bool osBufferLimited);
    void set_osBufferLimit(unsigned int osBufferLimit);
    void set_setFilters(QStringList includeStrings,QStringList includeOptions,QStringList excludeStrings,QStringList excludeOptions);
    void setRenamingRules(QString firstRenamingRule,QString otherRenamingRule);
public slots:
    //user ask ask to add folder (add it with interface ask source/destination)
    /** \brief add folder called on the interface
     * Used by manual adding */
    bool userAddFolder(const CopyMode &mode);
    /** \brief add file called on the interface
     * Used by manual adding */
    bool userAddFile(const CopyMode &mode);
    //action on the copy
    /// \brief put the transfer in pause
    void pause();
    /// \brief resume the transfer
    void resume();
    /** \brief skip one transfer entry
     * \param id id of the file to remove */
    void skip(const quint64 &id);
    /// \brief cancel all the transfer
    void cancel();
    //edit the transfer list
    /** \brief remove the selected item
     * \param ids ids is the id list of the selected items */
    void removeItems(const QList<int> &ids);
    /** \brief move on top of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnTop(const QList<int> &ids);
    /** \brief move up the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsUp(const QList<int> &ids);
    /** \brief move down the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsDown(const QList<int> &ids);
    /** \brief move on bottom of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnBottom(const QList<int> &ids);

    /** \brief give the forced mode, to export/import transfer list */
    void forceMode(const CopyMode &mode);
    /// \brief export the transfer list into a file
    void exportTransferList();
    /// \brief import the transfer list into a file
    void importTransferList();

    /** \brief to set the speed limitation
     * -1 if not able, 0 if disabled */
    bool setSpeedLimitation(const qint64 &speedLimitation);
    //action
    /// \brief to set the collision action
    void setCollisionAction(const QString &action);
    /// \brief to set the error action
    void setErrorAction(const QString &action);

    // specific to this copy engine

    /// \brief set if the rights shoul be keep
    void setRightTransfer(const bool doRightTransfer);
    /// \brief set keep date
    void setKeepDate(const bool keepDate);
    /// \brief set block size in KB
    void setBlockSize(const int blockSize);
    /// \brief set auto start
    void setAutoStart(const bool autoStart);
    /// \brief set if need check if the destination folder exists
    void setCheckDestinationFolderExists(const bool checkDestinationFolderExists);
    /// \brief reset widget
    void resetTempWidget();
    //autoconnect
    void on_comboBoxFolderColision_currentIndexChanged(int index);
    void on_comboBoxFolderError_currentIndexChanged(int index);
    /// \brief need retranslate the insterface
    void newLanguageLoaded();
private slots:
    void setComboBoxFolderColision(FolderExistsAction action,bool changeComboBox=true);
    void setComboBoxFolderError(FileErrorAction action,bool changeComboBox=true);
    void warningTransferList(const QString &warning);
    void errorTransferList(const QString &error);
signals:
    void error();
    void collision();

    //action on the copy
    void signal_pause();
    void signal_resume();
    void signal_skip(const quint64 &id);

    //edit the transfer list
    void signal_removeItems(const QList<int> &ids);
    void signal_moveItemsOnTop(const QList<int> &ids);
    void signal_moveItemsUp(const QList<int> &ids);
    void signal_moveItemsDown(const QList<int> &ids);
    void signal_moveItemsOnBottom(const QList<int> &ids);

    void signal_forceMode(const CopyMode &mode);
    void signal_exportTransferList(const QString &fileName);
    void signal_importTransferList(const QString &fileName);

    //action
    void signal_setCollisionAction(FileExistsAction alwaysDoThisActionForFileExists);
    void signal_setComboBoxFolderColision(FolderExistsAction action);
    void signal_setFolderColision(FolderExistsAction action);

    //when the cancel is clicked on copy engine dialog
    void cancelAll();

    //other signals
    void queryOneNewDialog();

    void send_osBufferLimit(const unsigned int &osBufferLimit);
    void send_setFilters(const QList<Filters_rules> &include,const QList<Filters_rules> &exclude);
    void send_sendNewRenamingRules(QString firstRenamingRule,QString otherRenamingRule);
};

#endif // COPY_ENGINE_H
