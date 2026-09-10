/* Pinned Dolphin/KIO API surface, so the REAL patched DolphinView::pasteToUrl() can be compiled
 * and RUN here without a Dolphin checkout.
 *
 * VERSION (fixed on purpose): this mirrors the API of **Dolphin 26.04.x** (the release the
 * `file-manager/dolphin-0002-*.patch` targets and the one installed on this box) plus KF6 KIO's
 * paste API. It is deliberately a PINNED copy: if a future Dolphin renames one of these members,
 * the case stops compiling -- which is the signal that the patch itself needs revisiting, exactly
 * what a silent "still applies" diff would hide.
 *
 * Everything the patched function calls out to goes through PasteEnvironment, a pure-virtual seam
 * the test OVERRIDES (the project's virtual/override test-seam idiom, see FileErrorDialog::
 * overrideFactory). That is what lets the case answer the two questions that matter:
 *   * when Ultracopier REFUSES the paste, does the file manager's own copier really take over,
 *     with the right sources and destination?
 *   * when Ultracopier ACCEPTS it, is the internal copier correctly NOT used?
 *
 * The patched code itself is never edited or re-typed: cases/dolphin_paste_patch.py extracts it
 * straight out of the .patch file, so this stub is the only thing that could drift, and it can
 * only drift loudly (a compile error). */
#ifndef DOLPHIN_API_STUB_H
#define DOLPHIN_API_STUB_H

#include <QObject>
#include <QWidget>
#include <QUrl>
#include <QList>
#include <QMimeData>
#include <QString>
#include <QStringList>

class KJob : public QObject
{
    Q_OBJECT
public:
    explicit KJob(QObject *parent = nullptr) : QObject(parent) {}
Q_SIGNALS:
    void result(KJob *job);
};

namespace DolphinTest {
/** The seam. The test subclasses this and observes what the patched paste really did. */
class PasteEnvironment
{
public:
    virtual ~PasteEnvironment() {}
    /** The file manager's OWN copier (KIO::paste in the real thing): called only on the fallback
     * path. The test override records the arguments and performs the copy it can do locally, so
     * "Ultracopier refused -> the internal copy still happens correctly" is really verified. */
    virtual void kioPaste(const QList<QUrl> &sources, const QUrl &destination) = 0;
};
/** Set by the driver before calling pasteToUrl(); never null while a paste runs. */
extern PasteEnvironment *environment;
/** Sources of the last kioPaste() call, for the driver's report. */
extern QList<QUrl> lastPasteSources;
extern QUrl lastPasteDestination;
extern int pasteCallCount;
}

namespace KIO {
class PasteJob : public KJob
{
    Q_OBJECT
public:
    explicit PasteJob(QObject *parent = nullptr) : KJob(parent) {}
Q_SIGNALS:
    void itemCreated(const QUrl &url);
};

/** KIO::paste(mimeData, destination) -- the file manager's internal copier. */
PasteJob *paste(const QMimeData *mimeData, const QUrl &destination);
}

namespace KJobWidgets {
inline void setWindow(KJob *, QWidget *) {}
}

namespace KUrlMimeData {
/** The urls of a clipboard paste (text/uri-list), as Dolphin reads them. */
QList<QUrl> urlsFromMimeData(const QMimeData *mimeData);
}

/** The pieces of DolphinView that the patched pasteToUrl() touches, with the same names as in
 * Dolphin 26.04.x. pasteToUrl is virtual so a test can specialise the view, and the two slots are
 * virtual for the same reason. */
class DolphinView : public QWidget
{
    Q_OBJECT
public:
    explicit DolphinView(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual void pasteToUrl(const QUrl &url);          // defined by the code taken from the patch

    bool m_clearSelectionBeforeSelectingNewItems = false;
    bool m_markFirstNewlySelectedItemAsCurrent = false;
public Q_SLOTS:
    virtual void slotItemCreated(const QUrl &url) { Q_UNUSED(url) }
    virtual void slotPasteJobResult(KJob *job) { Q_UNUSED(job) }
};

#endif // DOLPHIN_API_STUB_H
