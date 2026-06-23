/* Test-only override of FileErrorDialog (compiled into the TEST binary only -- see
 * binary_for() in test/lib/harness.py, which appends this file to SOURCES). It lives entirely
 * under test/, exactly like the LD_PRELOAD fault shim: the SHIPPING binary never compiles it, so
 * FileErrorDialog::overrideFactory stays nullptr there and the real GUI dialog runs unchanged.
 *
 * It exercises the otherwise-interactive fileError dialog headlessly by SUBCLASSING it and
 * overriding the two virtuals that matter -- QDialog::exec() (so nothing is ever shown) and
 * getAction() (so a scripted choice is returned). The choice comes from the env var
 * ULTRACOPIER_TEST_FILE_ERROR_ACTION, set by the Python case. This is the clean "virtual/override
 * the .ui class" seam: no test code, #ifdef or env read lives in the engine itself.
 *
 * The Retry path is the important one: media-reconnect RESUME is reachable ONLY through
 * FileError_Retry (the GUI "Retry" button). See test/cases/file_error_dialog.py and
 * test/cases/reconnect_resume_uring.py. */
#include "../../FileErrorDialog.h"

#include <QDialog>
#include <cstdlib>
#include <cstring>

namespace {

/// \brief headless FileErrorDialog: never shows, returns the scripted action.
class FileErrorDialogTest : public FileErrorDialog
{
public:
    FileErrorDialogTest(QWidget *parent, INTERNALTYPEPATH fileInfo, std::string errorString, const ErrorType &errorType, FacilityInterface *facilityEngine)
        : FileErrorDialog(parent,fileInfo,errorString,errorType,facilityEngine) {}

    /// \brief the "trigger event": never pop the modal GUI, accept immediately. The action dispatch
    /// (Skip/PutToEnd/Cancel, and Retry->resume on io_uring) is what we assert, and that is independent
    /// of how long the dialog stays up. NOTE: async's frozen restart-from-0 Retry is NOT asserted
    /// headlessly -- a real modal dialog GATES successive retries by waiting for the human, which a
    /// fast automated accept cannot reproduce; the async Retry button is left to manual GUI testing,
    /// while the Retry path itself is covered deterministically on io_uring (reconnect_resume_uring).
    int exec() override { return QDialog::Accepted; }

    /// \brief the scripted user choice, read from the env the Python case set.
    FileErrorAction getAction() override
    {
        const char *e=getenv("ULTRACOPIER_TEST_FILE_ERROR_ACTION");
        if(e!=NULL)
        {
            if(strcmp(e,"retry")==0)    return FileError_Retry;
            if(strcmp(e,"skip")==0)     return FileError_Skip;
            if(strcmp(e,"puttoend")==0) return FileError_PutToEndOfTheList;
            if(strcmp(e,"cancel")==0)   return FileError_Cancel;
        }
        // The factory is only ever consulted when the policy is Ask; reaching here means a case
        // forgot to set the env. Skip is the safe non-hanging default (the job still completes).
        return FileError_Skip;
    }

    /// \brief never make a choice "always" -- each fault is decided fresh by the env above.
    bool getAlways() override { return false; }
};

FileErrorDialog *makeTestDialog(QWidget *parent, INTERNALTYPEPATH fileInfo, std::string errorString, const ErrorType &errorType, FacilityInterface *facilityEngine)
{
    return new FileErrorDialogTest(parent,fileInfo,errorString,errorType,facilityEngine);
}

/// \brief install the override at static-init time (the .o is linked into the test binary only).
struct FileErrorDialogHookInstaller
{
    FileErrorDialogHookInstaller() { FileErrorDialog::overrideFactory=&makeTestDialog; }
};
static FileErrorDialogHookInstaller s_fileErrorDialogHookInstaller;

} // namespace
