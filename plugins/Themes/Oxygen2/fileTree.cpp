/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <martin.sandsmark@kde.org>
* Copyright 2017  Harald Sitter <sitter@kde.org>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
* by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/

#include "fileTree.h"

#include <QDir>
#include <QUrl>

QString File::displayName() const {
    const QString decodedName = QFile::decodeName(m_name);
    return url().isLocalFile() ? QDir::toNativeSeparators(decodedName) : decodedName;
}

QString File::displayPath(const Folder *root) const
{
    // Use QUrl to sanitize the path for display and then run it through
    // QDir to make sure we use native path separators.
    const QUrl url = this->url(root);
    const QString cleanPath = url.toDisplayString(QUrl::PreferLocalFile | QUrl::NormalizePathSegments);
    return url.isLocalFile() ? QDir::toNativeSeparators(cleanPath) : cleanPath;
}

QUrl File::url(const Folder *root) const
{
    QString path;

    if (root == this)
        root = nullptr; //prevent returning empty string when there is something we could return

    for (const Folder *d = (Folder*)this; d != root && d; d = d->parent()) {
        path.prepend(QFile::decodeName(d->name8Bit()));
    }

    return QUrl::fromUserInput(path, QString(), QUrl::AssumeLocalFile);
}
