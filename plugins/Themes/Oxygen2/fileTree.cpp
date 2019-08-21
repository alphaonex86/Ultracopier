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

FacilityInterface *File::facilityEngine=NULL;

#include <QDir>
#include <QUrl>

QString File::displayName() const {
    const QString decodedName = QString::fromStdString(m_name);
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
        const QString &name=QString::fromStdString(d->name());
        if(!name.isEmpty() && !path.isEmpty())
            path.prepend(QDir::separator());
        path.prepend(name);
    }

    return path;
}

void Folder::append(File *p)
{
    if(p->isFolder())
        folders[p->name()]=static_cast<Folder *>(p);
    else
        onlyFiles.push_back(p);
    Folder *d = this;
    while(d != nullptr) {
        d->m_children++;
        d->m_size += p->size();
        d=d->m_parent;
    }
}

///appends a Folder
void Folder::append(Folder *d, const std::string &name)
{
    if (!name.empty())
        m_name=name;

    m_children += d->children(); //doesn't include the dir itself
    d->m_parent = this;
    append((File*)d); //will add 1 to filecount for the dir itself
}

void Folder::append(Folder *d)
{
    m_children += d->children(); //doesn't include the dir itself
    d->m_parent = this;
    append((File*)d); //will add 1 to filecount for the dir itself
}

///appends a File
void Folder::append(const std::string &name, uint64_t size)
{
    append(new File(name, size, this));
}

/// removes a file
void Folder::remove(const File *f) {
    bool found=false;
    uint64_t sizeToRemove=0;
    for(const auto& n : folders)
    {
        Folder * folder=n.second;
        if(f==folder)
        {
            sizeToRemove+=f->size();
            found=true;
            break;
        }
    }
    if(!found)
    {
        for (unsigned int i = 0; i < onlyFiles.size();)
        {
            if(onlyFiles.at(i)==f)
            {
                delete f;
                sizeToRemove+=f->size();
                break;
            }
            else
                i++;
        }
    }
    Folder *d = this;
    while(d != nullptr) {
        d->m_size -= sizeToRemove;
        d->m_children--;
        d=d->m_parent;
    }
}
