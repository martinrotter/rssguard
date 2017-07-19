/* ============================================================
* QupZilla - Qt web browser
* Copyright (C) 2013-2017 David Rosca <nowrep@gmail.com>
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
#include "adblocksearchtree.h"
#include "adblockrule.h"

#include <QWebEngineUrlRequestInfo>

AdBlockSearchTree::AdBlockSearchTree()
    : m_root(new Node)
{
}

AdBlockSearchTree::~AdBlockSearchTree()
{
    deleteNode(m_root);
}

void AdBlockSearchTree::clear()
{
    deleteNode(m_root);
    m_root = new Node;
}

bool AdBlockSearchTree::add(const AdBlockRule* rule)
{
    if (rule->m_type != AdBlockRule::StringContainsMatchRule) {
        return false;
    }

    const QString filter = rule->m_matchString;
    int len = filter.size();

    if (len <= 0) {
        qDebug() << "AdBlockSearchTree: Inserting rule with filter len <= 0!";
        return false;
    }

    Node* node = m_root;

    for (int i = 0; i < len; ++i) {
        const QChar c = filter.at(i);
        Node *next = node->children.value(c);
        if (!next) {
            next = new Node;
            next->c = c;
            node->children[c] = next;
        }
        node = next;
    }

    node->rule = rule;

    return true;
}

const AdBlockRule* AdBlockSearchTree::find(const QWebEngineUrlRequestInfo &request, const QString &domain, const QString &urlString) const
{
    int len = urlString.size();

    if (len <= 0) {
        return 0;
    }

    const QChar* string = urlString.constData();

    for (int i = 0; i < len; ++i) {
        const AdBlockRule* rule = prefixSearch(request, domain, urlString, string++, len - i);
        if (rule) {
            return rule;
        }
    }

    return 0;
}

const AdBlockRule* AdBlockSearchTree::prefixSearch(const QWebEngineUrlRequestInfo &request, const QString &domain, const QString &urlString, const QChar* string, int len) const
{
    if (len <= 0) {
        return 0;
    }

    QChar c = string[0];

    Node* node = m_root->children.value(c);
    if (!node) {
        return nullptr;
    }

    for (int i = 1; i < len; ++i) {
        const QChar c = (++string)[0];

        if (node->rule && node->rule->networkMatch(request, domain, urlString)) {
            return node->rule;
        }

        node = node->children.value(c);
        if (!node) {
            return nullptr;
        }
    }

    if (node->rule && node->rule->networkMatch(request, domain, urlString)) {
        return node->rule;
    }

    return nullptr;
}

void AdBlockSearchTree::deleteNode(AdBlockSearchTree::Node* node)
{
    if (!node) {
        return;
    }

    QHashIterator<QChar, Node*> i(node->children);
    while (i.hasNext()) {
        i.next();
        deleteNode(i.value());
    }

    delete node;
}
