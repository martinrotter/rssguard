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

#ifndef ADBLOCKURLINTERCEPTOR_H
#define ADBLOCKURLINTERCEPTOR_H

#include "urlinterceptor.h"
#include "qzcommon.h"

class AdBlockManager;

class QUPZILLA_EXPORT AdBlockUrlInterceptor : public UrlInterceptor
{
public:
    explicit AdBlockUrlInterceptor(AdBlockManager* manager);

    void interceptRequest(QWebEngineUrlRequestInfo &info);

private:
    AdBlockManager *m_manager;
};

#endif // ADBLOCKURLINTERCEPTOR_H
