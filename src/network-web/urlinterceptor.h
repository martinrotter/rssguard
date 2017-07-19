/* ============================================================
* QupZilla - QtWebEngine based browser
* Copyright (C) 2015 David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */

#ifndef URLINTERCEPTOR_H
#define URLINTERCEPTOR_H

#include <QObject>
#include <QWebEngineUrlRequestInfo>

class UrlInterceptor : public QObject
{
public:
    explicit UrlInterceptor(QObject *parent = Q_NULLPTR) : QObject(parent) { }
    virtual void interceptRequest(QWebEngineUrlRequestInfo &info) = 0;
};

#endif // URLINTERCEPTOR_H
