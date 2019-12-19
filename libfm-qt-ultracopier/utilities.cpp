/*
 * Copyright (C) 2013 - 2015  Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "utilities.h"
#include <QClipboard>

namespace Fm {

const std::string pathSocket()
{
#ifdef Q_OS_UNIX
    return "advanced-copier-"+std::to_string(getuid());
#else
    QString userName;
    char uname[1024];
    DWORD len=1023;
    if(GetUserNameA(uname, &len)!=FALSE)
        userName=QString::fromLatin1(toHex(uname));
    return "advanced-copier-"+userName.toStdString();
#endif
}

char * toHex(const char *str)
{
    char *p, *sz;
    size_t len;
    if (str==NULL)
        return NULL;
    len= strlen(str);
    p = sz = (char *) malloc((len+1)*4);
    for (size_t i=0; i<len; i++)
    {
        sprintf(p, "%.2x00", str[i]);
        p+=4;
    }
    *p=0;
    return sz;
}

void sendRawOrderList(const QStringList & order, QLocalSocket &socket)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << int(0);
    out << 99;//idNextOrder
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
    int (*new_puts)(const char *message);
int result;
new_puts = dlsym(RTLD_NEXT, "puts");
    
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* data = clipboard->mimeData();
    Fm::FilePathList paths;
    bool isCut = false;

    std::tie(paths, isCut) = parseClipboardData(*data);

    if(!paths.empty()) {
        QLocalSocket socket;
        socket.connectToServer(QString::fromStdString(ExtraSocketCatchcopy::pathSocket()));
        socket.waitForConnected();
        if(socket.state()==QLocalSocket::ConnectedState)
        {
            sendRawOrderList(QStringList() << "protocol" << CATCHCOPY_PROTOCOL_VERSION);
            if(isCut) {
                sendRawOrderList(QStringList() << "mv" << paths << destPath);
                clipboard->clear(QClipboard::Clipboard);
            }
            else {
                sendRawOrderList(QStringList() << "cp" << paths << destPath);
            }
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
