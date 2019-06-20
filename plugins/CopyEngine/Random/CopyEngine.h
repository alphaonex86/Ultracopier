/** \file copyEngine.h
\brief Define the copy engine
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#include "../../../interface/PluginInterface_CopyEngine.h"

#ifndef COPY_ENGINE_H
#define COPY_ENGINE_H

#include <QTimer>

namespace Ui {
    class copyEngineOptions;
}

/// \brief the implementation of copy engine plugin, manage directly few stuff, else pass to ListThread class.
class CopyEngine : public PluginInterface_CopyEngine
{
        Q_OBJECT
public:
    CopyEngine();
    ~CopyEngine();
    void exportErrorIntoTransferList() override;
public:
    /** \brief to send the options panel
     * \return return false if have not the options
     * \param tempWidget the widget to generate on it the options */
    bool getOptionsEngine(QWidget * tempWidget) override;
    /** \brief to have interface widget to do modal dialog
     * \param interface to have the widget of the interface, useful for modal dialog */
    void setInterfacePointer(QWidget * uiinterface) override;
    //return empty if multiple
    /** \brief compare the current sources of the copy, with the passed arguments
     * \param sources the sources list to compares with the current sources list
     * \return true if have same sources, else false (or empty) */
    bool haveSameSource(const std::vector<std::string> &sources) override;
    /** \brief compare the current destination of the copy, with the passed arguments
     * \param destination the destination to compares with the current destination
     * \return true if have same destination, else false (or empty) */
    bool haveSameDestination(const std::string &destination) override;
    //external soft like file browser have send copy/move list to do
    /** \brief send copy without destination, ask the destination
     * \param sources the sources list to copy
     * \return true if the copy have been accepted */
    bool newCopy(const std::vector<std::string> &sources) override;
    /** \brief send copy with destination
     * \param sources the sources list to copy
     * \param destination the destination to copy
     * \return true if the copy have been accepted */
    bool newCopy(const std::vector<std::string> &sources,const std::string &destination) override;
    /** \brief send move without destination, ask the destination
     * \param sources the sources list to move
     * \return true if the move have been accepted */
    bool newMove(const std::vector<std::string> &sources) override;
    /** \brief send move without destination, ask the destination
     * \param sources the sources list to move
     * \param destination the destination to move
     * \return true if the move have been accepted */
    bool newMove(const std::vector<std::string> &sources,const std::string &destination) override;
    /** \brief send the new transfer list
     * \param file the transfer list */
    void newTransferList(const std::string &file) override;

    /** \brief to get byte read, use by Ultracopier for the speed calculation
     * real size transfered to right speed calculation */
    uint64_t realByteTransfered() override;
    /** \brief support speed limitation */
    bool supportSpeedLimitation() const override;

    /** \brief to set drives detected
     * specific to this copy engine */

    /** \brief to sync the transfer list
     * Used when the interface is changed, useful to minimize the memory size */
    void syncTransferList() override;
public slots:
    //user ask ask to add folder (add it with interface ask source/destination)
    /** \brief add folder called on the interface
     * Used by manual adding */
    bool userAddFolder(const Ultracopier::CopyMode &mode) override;
    /** \brief add file called on the interface
     * Used by manual adding */
    bool userAddFile(const Ultracopier::CopyMode &mode) override;
    //action on the copy
    /// \brief put the transfer in pause
    void pause() override;
    /// \brief resume the transfer
    void resume() override;
    /** \brief skip one transfer entry
     * \param id id of the file to remove */
    void skip(const uint64_t &id) override;
    /// \brief cancel all the transfer
    void cancel() override;
    //edit the transfer list
    /** \brief remove the selected item
     * \param ids ids is the id list of the selected items */
    void removeItems(const std::vector<uint64_t> &ids) override;
    /** \brief move on top of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnTop(const std::vector<uint64_t> &ids) override;
    /** \brief move up the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsUp(const std::vector<uint64_t> &ids) override;
    /** \brief move down the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsDown(const std::vector<uint64_t> &ids) override;
    /** \brief move on bottom of the list the selected item
     * \param ids ids is the id list of the selected items */
    void moveItemsOnBottom(const std::vector<uint64_t> &ids) override;

    /** \brief give the forced mode, to export/import transfer list */
    void forceMode(const Ultracopier::CopyMode &mode) override;
    /// \brief export the transfer list into a file
    void exportTransferList() override;
    /// \brief import the transfer list into a file
    void importTransferList() override;

    /** \brief to set the speed limitation
     * -1 if not able, 0 if disabled */
    bool setSpeedLimitation(const int64_t &speedLimitation) override;
private:
    bool send;
    QTimer *timer;
    void timerSlot();
};

#endif // COPY_ENGINE_H
