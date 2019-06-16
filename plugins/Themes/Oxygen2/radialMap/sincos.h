/***********************************************************************
* Copyright 2003-2004  Max Howell <max.howell@methylblue.com>
* Copyright 2008-2009  Martin Sandsmark <martin.sandsmark@kde.org>
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

#ifndef SINCOS_H
#define SINCOS_H

#include <math.h>

#if !defined(__GLIBC__) || (__GLIBC__ < 2) ||  (__GLIBC__ == 2 && __GLIBC_MINOR__ < 1)

#include <qmath.h>

void
sincos(double angleRadians, double *Sin, double *Cos);

#ifdef SINCOS_H_IMPLEMENTATION
void
sincos(double angleRadians, double *Sin, double *Cos)
{
    *Sin = qSin(angleRadians);
    *Cos = qCos(angleRadians);
}
#endif

#endif

#endif
