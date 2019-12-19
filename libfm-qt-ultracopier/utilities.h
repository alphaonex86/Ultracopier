#include <QWidget>
#include <QStringList>
#include <QLocalSocket>

#include <libfm-qt/utilities.h>
#include <libfm-qt/core/filepath.h>

#ifndef FM_UTILITIESUC_H
#define FM_UTILITIESUC_H

namespace Fm {

__attribute__((visibility("default"))) void pasteFilesFromClipboard(const Fm::FilePath& destPath, QWidget* parent);

std::string pathSocket();
char * toHex(const char *str);
void sendRawOrderList(const QStringList & order, QLocalSocket &socket);

}

#endif // FM_UTILITIESUC_H
