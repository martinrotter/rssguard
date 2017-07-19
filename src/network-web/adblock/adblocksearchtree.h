/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2013-2014  David Rosca <nowrep@gmail.com>
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
#ifndef ADBLOCKSEARCHTREE_H
#define ADBLOCKSEARCHTREE_H

#include <QChar>
#include <QHash>

#include "qzcommon.h"

class QWebEngineUrlRequestInfo;

class AdBlockRule;

class QUPZILLA_EXPORT AdBlockSearchTree
{
public:
    explicit AdBlockSearchTree();
    ~AdBlockSearchTree();

    void clear();

    bool add(const AdBlockRule* rule);
    const AdBlockRule* find(const QWebEngineUrlRequestInfo &request, const QString &domain, const QString &urlString) const;

private:
    struct Node {
        QChar c;
        const AdBlockRule* rule;
        QHash<QChar, Node*> children;

        Node() : c(0) , rule(0) { }
    };

    const AdBlockRule* prefixSearch(const QWebEngineUrlRequestInfo &request, const QString &domain,
                                    const QString &urlString, const QChar* string, int len) const;

    void deleteNode(Node* node);

    Node* m_root;
};

#endif // ADBLOCKSEARCHTREE_H
