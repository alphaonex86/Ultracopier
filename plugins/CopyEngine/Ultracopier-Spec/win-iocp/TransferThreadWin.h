/** \file TransferThreadWin.h
\brief Windows IOCP backend for the pipelined file transfer (Windows only)
\author alpha_one_x86
\licence GPL3, see the file COPYING

Implements only the I/O hooks of TransferThreadPipelined with a Windows I/O completion
port and overlapped ReadFile/WriteFile; all transfer logic lives in the shared base
class. Compiled only when ULTRACOPIER_PLUGIN_WINIOCP is defined; otherwise the translation unit is
empty and cannot affect the existing backends. */

#ifndef TRANSFERTHREADWIN_H
#define TRANSFERTHREADWIN_H

#ifdef ULTRACOPIER_PLUGIN_WINIOCP

// The IOCP backend uses GetQueuedCompletionStatusEx()/CancelIoEx() — Windows Vista+ APIs
// absent on Windows XP. ULTRACOPIER_PLUGIN_WINIOCP is defined ONLY for Qt6 (Windows 10+)
// builds by ultracopier.pro / CopyEngine.pro (win32:greaterThan(QT_MAJOR_VERSION,5)); on
// Qt5 / Windows XP / pre-Vista the define is absent, so the .pro selects the async
// (thread-based) backend instead and this whole file compiles to nothing. No #error: the
// build gracefully falls back rather than failing (and stays mingw 4.9.x compatible).
#include "../pipeline/TransferThreadPipelined.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

/// \brief Windows IOCP implementation of the pipelined transfer I/O hooks
class TransferThreadWin : public TransferThreadPipelined
{
    // Q_OBJECT is required even though the subclass adds no new signals/slots:
    // ListThread does qobject_cast<TransferThreadImpl*>(sender()), which needs the
    // concrete type to carry its own meta-object.
    Q_OBJECT
public:
    explicit TransferThreadWin();
    ~TransferThreadWin();
protected:
    void run() override;
private:
    // IOCP overlapped pipeline
    static constexpr int NUM_BUFFERS = 8;

    struct PipelineBuffer {
        char *data;
        unsigned int allocSize;
        int bytesUsed;      // bytes currently held in this buffer (valid data length)
        int64_t fileOffset; // source/destination offset this buffer's data belongs to
        unsigned int chunkSize; // intended size of the chunk assigned to this buffer
        OVERLAPPED ov;      // per-buffer overlapped struct; Offset holds the file offset
        enum State { Free, Reading, WriteReady, Writing } state;
    };
    PipelineBuffer pipelineBuffers[NUM_BUFFERS];

    HANDLE iocp;            // I/O completion port, all handles associated to it
    bool iocpInitialized;

    HANDLE sourceHandle;
    HANDLE destHandle;
    uint64_t sourceFileSize;
    int64_t readOffset;
    int64_t writeOffset;

    // Completion-key constants for CreateIoCompletionPort(): source handle posts with
    // KEY_SOURCE, destination with KEY_DEST. The specific buffer is recovered from the
    // returned OVERLAPPED* (each buffer owns exactly one outstanding op at a time).
    static constexpr ULONG_PTR KEY_SOURCE = 1;
    static constexpr ULONG_PTR KEY_DEST   = 2;
    /// \brief map a completed OVERLAPPED* back to its pipeline buffer index, or -1
    int bufferIndexForOverlapped(OVERLAPPED *ov);

    // TransferThreadPipelined I/O hooks
    int openSourceFile() override;
    int openDestFile(uint64_t startSize) override;
    void closeFiles() override;
    void doTransferPipeline() override;
    bool remainSourceOpen() const override;
    bool remainDestinationOpen() const override;
    /// \brief SetFileTime() on the still-open destHandle using the cached source times
    /// (ftCreate/ftAccess/ftWrite), avoiding the per-file reopen in doFilePostOperation.
    bool applyDateTimeOnOpenDestination() override;
    /// \brief recreate a Windows symlink/junction at the destination (don't follow it),
    /// matching the async backend. Returns false for a non-reparse source (regular copy).
    bool trySymlinkCopy() override;
    /// \brief create an NTFS junction (reparse point) at szJunction pointing to szPath
    static bool mkJunction(const wchar_t *szJunction, const wchar_t *szPath);

    void initPipelineBuffers();
    void freePipelineBuffers();
};

#endif // ULTRACOPIER_PLUGIN_WINIOCP

#endif // TRANSFERTHREADWIN_H
