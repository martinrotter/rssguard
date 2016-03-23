// This file is part of RSS Guard.
//
// Copyright (C) 2014-2015 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2010-2014 by David Rosca <nowrep@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#ifndef ADBLOCKSEARCHTREE_H
#define ADBLOCKSEARCHTREE_H

#include <QChar>
#include <QHash>
#include <QUrl>
#include <QWebEngineUrlRequestInfo>


class QNetworkRequest;
class AdBlockRule;

class AdBlockSearchTree {
  public:
    explicit AdBlockSearchTree();
    virtual ~AdBlockSearchTree();

    void clear();

    bool add(const AdBlockRule *rule);
    const AdBlockRule *find(const QUrl &url, const QString &domain, const QString &url_string,
                            const QString &referer, QWebEngineUrlRequestInfo::ResourceType resource_type) const;

  private:
    struct Node {
      public:
        QChar c;
        const AdBlockRule *rule;
        QHash<QChar,Node*> children;

        Node() : c(0) , rule(0) {
        }
    };

    void deleteNode(Node *node);
    const AdBlockRule *prefixSearch(const QUrl &url, const QString &domain,
                                    const QString &url_string, const QChar* string, const QString &referer,
                                    QWebEngineUrlRequestInfo::ResourceType resource_type, int len) const;

    Node *m_root;
};

#endif // ADBLOCKSEARCHTREE_H
