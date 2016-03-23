// This file is part of RSS Guard.
//
// Copyright (C) 2014-2015 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2014 by David Rosca <nowrep@gmail.com>
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

#include "network-web/adblock/adblockmatcher.h"

#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblockrule.h"
#include "network-web/adblock/adblocksubscription.h"
#include "definitions/definitions.h"


AdBlockMatcher::AdBlockMatcher(AdBlockManager *manager) : QObject(manager), m_manager(manager) {
  connect(manager, SIGNAL(enabledChanged(bool)), this, SLOT(enabledChanged(bool)));
}

AdBlockMatcher::~AdBlockMatcher() {
  clear();
}

const AdBlockRule *AdBlockMatcher::match(const QUrl &url, const QString &url_domain,
                                         const QString &url_string, const QString &referer,
                                         QWebEngineUrlRequestInfo::ResourceType resource_type) const {
  // Exception rules.
  if (m_networkExceptionTree.find(url, url_domain, url_string, referer, resource_type)) {
    return NULL;
  }

  for (int i = 0, count = m_networkExceptionRules.size(); i < count; i++) {
    const AdBlockRule *rule = m_networkExceptionRules.at(i);

    if (rule->networkMatch(url, url_domain, url_string, referer, resource_type)) {
      return NULL;
    }
  }

  // Block rules.
  if (const AdBlockRule* rule = m_networkBlockTree.find(url, url_domain, url_string, referer, resource_type)) {
    return rule;
  }

  for (int i = 0, count = m_networkBlockRules.size(); i < count; i++) {
    const AdBlockRule *rule = m_networkBlockRules.at(i);

    if (rule->networkMatch(url, url_domain, url_string, referer, resource_type)) {
      return rule;
    }
  }

  return NULL;
}

bool AdBlockMatcher::adBlockDisabledForUrl(const QUrl &url, const QString &referer,
                                           QWebEngineUrlRequestInfo::ResourceType resource_type) const {
  for (int i = 0, count = m_documentRules.size(); i < count; i++) {
    if (m_documentRules.at(i)->urlMatch(url, referer, resource_type)) {
      return true;
    }
  }

  return false;
}

void AdBlockMatcher::update() {
  clear();

  foreach (const AdBlockSubscription *subscription, m_manager->subscriptions()) {
    foreach (const AdBlockRule *rule, subscription->allRules()) {
      // Don't add internally disabled rules to cache
      if (rule->isInternalDisabled()) {
        continue;
      }

      if (rule->isDocument()) {
        m_documentRules.append(rule);
      }
      else if (rule->isException()) {
        if (!m_networkExceptionTree.add(rule)) {
          m_networkExceptionRules.append(rule);
        }
      }
      else {
        if (!m_networkBlockTree.add(rule)) {
          m_networkBlockRules.append(rule);
        }
      }
    }
  }
}

void AdBlockMatcher::clear() {
  m_networkExceptionTree.clear();
  m_networkExceptionRules.clear();

  m_networkBlockTree.clear();
  m_networkBlockRules.clear();

  m_domainRestrictedCssRules.clear();
  m_documentRules.clear();
}

void AdBlockMatcher::enabledChanged(bool enabled) {
  if (enabled) {
    update();
  }
  else {
    clear();
  }
}
