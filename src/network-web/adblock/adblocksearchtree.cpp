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

#include "network-web/adblock/adblocksearchtree.h"
#include "network-web/adblock/adblockrule.h"


AdBlockSearchTree::AdBlockSearchTree() : m_root(new Node()) {
}

AdBlockSearchTree::~AdBlockSearchTree() {
  deleteNode(m_root);
}

void AdBlockSearchTree::clear() {
  deleteNode(m_root);
  m_root = new Node();
}

bool AdBlockSearchTree::add(const AdBlockRule *rule) {
  if (rule->m_type != AdBlockRule::StringContainsMatchRule) {
    return false;
  }

  const QString filter = rule->m_matchString;
  int len = filter.size();

  if (len <= 0) {
    qWarning("Inserting rule with filter len <= 0!");
    return false;
  }

  Node *node = m_root;

  for (int i = 0; i < len; i++) {
    const QChar c = filter.at(i);

    if (!node->children.contains(c)) {
      Node *n = new Node();
      n->c = c;

      node->children[c] = n;
    }

    node = node->children[c];
  }

  node->rule = rule;

  return true;
}

const AdBlockRule *AdBlockSearchTree::find(const QNetworkRequest &request, const QString &domain,
                                           const QString &url_string) const {
  int len = url_string.size();

  if (len <= 0) {
    return NULL;
  }

  const QChar *string = url_string.constData();

  for (int i = 0; i < len; i++) {
    const AdBlockRule *rule = prefixSearch(request, domain, url_string, string++, len - i);

    if (rule != NULL) {
      return rule;
    }
  }

  return NULL;
}

const AdBlockRule *AdBlockSearchTree::prefixSearch(const QNetworkRequest &request, const QString &domain,
                                                   const QString &url_string, const QChar* string, int len) const {
  if (len <= 0) {
    return NULL;
  }

  QChar c = string[0];

  if (!m_root->children.contains(c)) {
    return NULL;
  }

  Node *node = m_root->children[c];

  for (int i = 1; i < len; i++) {
    const QChar c = (++string)[0];

    if (node->rule && node->rule->networkMatch(request, domain, url_string)) {
      return node->rule;
    }

    if (!node->children.contains(c)) {
      return NULL;
    }

    node = node->children[c];
  }

  if (node->rule && node->rule->networkMatch(request, domain, url_string)) {
    return node->rule;
  }

  return NULL;
}

void AdBlockSearchTree::deleteNode(Node *node) {
  if (node == NULL) {
    return;
  }

  QHashIterator<QChar,Node*> i(node->children);

  while (i.hasNext()) {
    i.next();
    deleteNode(i.value());
  }

  delete node;
}
