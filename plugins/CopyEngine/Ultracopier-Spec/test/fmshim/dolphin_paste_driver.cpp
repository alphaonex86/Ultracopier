/* Runs the REAL patched DolphinView::pasteToUrl() (taken verbatim from
 * file-manager/dolphin-0002-*.patch) against a REAL Ultracopier, and reports what it did.
 *
 *   dolphin_paste_driver <destination-dir> cp|cut <source-url> [<source-url> ...]
 *
 * It puts the sources on the clipboard exactly as a file manager does (text/uri-list, plus
 * application/x-kde-cutselection for a cut), calls pasteToUrl(), and prints one line per fact:
 *
 *   INTERNAL_COPY <n>   the file manager's own copier ran (the fallback) with n sources
 *   ULTRACOPIER         Ultracopier took the job (the internal copier did NOT run)
 *   CLIPBOARD cleared|kept
 *
 * The internal copier is the PasteEnvironment override below: it does the copy KIO would do for
 * the sources it can handle (local files), so a refusal is verified all the way to the bytes on
 * disk, not just to "the fallback was reached".
 *
 * The socket name the patch builds ("advanced-copier-<uid>") is NOT parameterised on purpose --
 * that hardcoded name is part of what is under test. The case isolates the run with a private
 * $TMPDIR instead, which is where Qt puts the socket. */
#include "dolphin_api_stub.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <iostream>

namespace DolphinTest {
PasteEnvironment *environment = nullptr;
QList<QUrl> lastPasteSources;
QUrl lastPasteDestination;
int pasteCallCount = 0;
}

namespace KIO {
PasteJob *paste(const QMimeData *mimeData, const QUrl &destination)
{
    QList<QUrl> sources = KUrlMimeData::urlsFromMimeData(mimeData);
    DolphinTest::lastPasteSources = sources;
    DolphinTest::lastPasteDestination = destination;
    ++DolphinTest::pasteCallCount;
    if (DolphinTest::environment != nullptr) {
        DolphinTest::environment->kioPaste(sources, destination);
    }
    return new PasteJob();
}
}

namespace KUrlMimeData {
QList<QUrl> urlsFromMimeData(const QMimeData *mimeData)
{
    QList<QUrl> urls;
    if (mimeData == nullptr) {
        return urls;
    }
    const QByteArray data = mimeData->data(QStringLiteral("text/uri-list"));
    const QList<QByteArray> lines = data.split('\n');
    for (const QByteArray &line : lines) {
        const QByteArray trimmed = line.trimmed();
        if (!trimmed.isEmpty() && !trimmed.startsWith('#')) {
            urls.append(QUrl::fromEncoded(trimmed));
        }
    }
    return urls;
}
}

/* The file manager's own copier: what must take over when Ultracopier refuses the paste. */
class InternalCopier : public DolphinTest::PasteEnvironment
{
public:
    void kioPaste(const QList<QUrl> &sources, const QUrl &destination) override
    {
        const QString destDir = destination.toLocalFile();
        for (const QUrl &u : sources) {
            if (u.isLocalFile()) {
                const QString src = u.toLocalFile();
                const QString dst = QDir(destDir).filePath(QFileInfo(src).fileName());
                QFile::remove(dst);
                if (!QFile::copy(src, dst)) {
                    std::cout << "INTERNAL_COPY_FAILED " << dst.toStdString() << std::endl;
                }
            }
            // a non-local url (sftp://...) is exactly what KIO handles natively and Ultracopier
            // could not: nothing to do here, its presence in `sources` is what the case asserts.
        }
    }
};

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (argc < 4) {
        std::cout << "usage: dolphin_paste_driver <dest-dir> cp|cut <source-url> [...]" << std::endl;
        return 3;
    }
    const QUrl destination = QUrl::fromLocalFile(QString::fromUtf8(argv[1]));
    const bool isCut = (QString::fromUtf8(argv[2]) == QLatin1String("cut"));

    QByteArray uriList;
    for (int i = 3; i < argc; ++i) {
        uriList += QUrl(QString::fromUtf8(argv[i])).toEncoded();
        uriList += "\r\n";
    }
    QMimeData *mime = new QMimeData();
    mime->setData(QStringLiteral("text/uri-list"), uriList);
    if (isCut) {
        mime->setData(QStringLiteral("application/x-kde-cutselection"), QByteArrayLiteral("1"));
    }
    QApplication::clipboard()->setMimeData(mime);

    InternalCopier copier;
    DolphinTest::environment = &copier;

    DolphinView view;
    view.pasteToUrl(destination);

    if (DolphinTest::pasteCallCount > 0) {
        std::cout << "INTERNAL_COPY " << DolphinTest::lastPasteSources.size() << std::endl;
    } else {
        std::cout << "ULTRACOPIER" << std::endl;
    }
    const QMimeData *after = QApplication::clipboard()->mimeData();
    const bool cleared = (after == nullptr)
                         || KUrlMimeData::urlsFromMimeData(after).isEmpty();
    std::cout << "CLIPBOARD " << (cleared ? "cleared" : "kept") << std::endl;
    return 0;
}
