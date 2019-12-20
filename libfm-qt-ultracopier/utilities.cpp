#include "utilities.h"
#include <QClipboard>
#include <QApplication>
#include <libfm-qt/foldermodel.h>
#include <libfm-qt/fileoperation.h>
#include <dlfcn.h>

namespace Fm {

void sendRawOrderList(const QStringList & order, QLocalSocket &socket, int idNextOrder)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << int(0);
    out << idNextOrder;
    out << order;
    out.device()->seek(0);
    out << block.size();
    do //cut string list and send it as block of 32KB
    {
        QByteArray blockToSend;
        int byteWriten;
        blockToSend=block.left(32*1024);//32KB
        block.remove(0,blockToSend.size());
        byteWriten = socket.write(blockToSend);
    }
    while(block.size());
}

void pasteFilesFromClipboard(const Fm::FilePath& destPath, QWidget* parent) {
    //https://gist.github.com/mooware/1174572
    typedef std::pair<Fm::FilePathList, bool> (*methodType)(const QMimeData& data);

    static methodType origMethod = 0;

    // use the mangled method name here. RTLD_NEXT means something like
    // "search this symbol in any libraries loaded after the current one".
    void *tmpPtr = dlsym(RTLD_NEXT, "pasteFilesFromClipboard");

    // not even reinterpret_cast can convert between void* and a method ptr,
    // so i'm doing the worst hack i've ever seen.
    memcpy(&origMethod, &tmpPtr, sizeof(tmpPtr));

    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* data = clipboard->mimeData();
    Fm::FilePathList paths;
    bool isCut = false;

    std::tie(paths, isCut) = (*origMethod)(*data);

    if(!paths.empty()) {
        QLocalSocket socket;
        socket.connectToServer(QString::fromStdString("advanced-copier-"+std::to_string(getuid())));
        socket.waitForConnected();
        if(socket.state()==QLocalSocket::ConnectedState)
        {
            sendRawOrderList(QStringList() << "protocol" << "0002", socket, 1);
            socket.waitForReadyRead();
            socket.readAll();
            QStringList l;
            if(isCut) {
                l << "mv";
                clipboard->clear(QClipboard::Clipboard);
            }
            else {
                l << "cp";
            }
            for(const FilePath &n : paths)
                l << n.toString().get();
            l << destPath.toString().get();
            sendRawOrderList(l, socket, 2);
            socket.waitForBytesWritten();
            socket.close();
        }
        else
        {
            if(isCut) {
                FileOperation::moveFiles(paths, destPath, parent);
                clipboard->clear(QClipboard::Clipboard);
            }
            else {
                FileOperation::copyFiles(paths, destPath, parent);
            }
        }
    }
}

} // namespace Fm
