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

const AdBlockRule *AdBlockMatcher::match(const QNetworkRequest &request, const QString &url_domain,
                                         const QString &url_string) const {
  // Exception rules.
  if (m_networkExceptionTree.find(request, url_domain, url_string)) {
    return NULL;
  }

  for (int i = 0, count = m_networkExceptionRules.size(); i < count; i++) {
    const AdBlockRule *rule = m_networkExceptionRules.at(i);

    if (rule->networkMatch(request, url_domain, url_string)) {
      return NULL;
    }
  }

  // Block rules.
  if (const AdBlockRule* rule = m_networkBlockTree.find(request, url_domain, url_string)) {
    return rule;
  }

  for (int i = 0, count = m_networkBlockRules.size(); i < count; i++) {
    const AdBlockRule *rule = m_networkBlockRules.at(i);

    if (rule->networkMatch(request, url_domain, url_string)) {
      return rule;
    }
  }

  return NULL;
}

bool AdBlockMatcher::adBlockDisabledForUrl(const QUrl &url) const {
  for (int i = 0, count = m_documentRules.size(); i < count; i++) {
    if (m_documentRules.at(i)->urlMatch(url)) {
      return true;
    }
  }

  return false;
}

bool AdBlockMatcher::elemHideDisabledForUrl(const QUrl &url) const {
  if (adBlockDisabledForUrl(url)) {
    return true;
  }

  for (int i = 0, count = m_elemhideRules.size(); i < count; i++) {
    if (m_elemhideRules.at(i)->urlMatch(url)) {
      return true;
    }
  }

  return false;
}

QString AdBlockMatcher::elementHidingRules() const {
  return m_elementHidingRules;
}

QString AdBlockMatcher::elementHidingRulesForDomain(const QString &domain) const {
  QString rules;
  int added_rules_count = 0;

  for (int i = 0, count = m_domainRestrictedCssRules.size(); i < count; i++) {
    const AdBlockRule *rule = m_domainRestrictedCssRules.at(i);

    if (!rule->matchDomain(domain)) {
      continue;
    }

    if (added_rules_count == 1000) {
      rules.append(rule->cssSelector());
      rules.append(QL1S("{display:none !important;}\n"));
      added_rules_count = 0;
    }
    else {
      rules.append(rule->cssSelector() + QL1C(','));
      added_rules_count++;
    }
  }

  if (added_rules_count != 0) {
    rules = rules.left(rules.size() - 1);
    rules.append(QLatin1String("{display:none !important;}\n"));
  }

  return rules;
}

void AdBlockMatcher::update() {
  clear();

  QHash<QString, const AdBlockRule*> css_rules_hash;
  QVector<const AdBlockRule*> exception_css_rules;

  foreach (AdBlockSubscription *subscription, m_manager->subscriptions()) {
    foreach (const AdBlockRule *rule, subscription->allRules()) {
      // Don't add internally disabled rules to cache
      if (rule->isInternalDisabled()) {
        continue;
      }

      if (rule->isCssRule()) {
        // We will add only enabled css rules to cache, because there is no enabled/disabled
        // check on match. They are directly embedded to pages.
        if (!rule->isEnabled()) {
          continue;
        }

        if (rule->isException()) {
          exception_css_rules.append(rule);
        }
        else {
          css_rules_hash.insert(rule->cssSelector(), rule);
        }
      }
      else if (rule->isDocument()) {
        m_documentRules.append(rule);
      }
      else if (rule->isElemhide()) {
        m_elemhideRules.append(rule);
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

  foreach (const AdBlockRule *rule, exception_css_rules) {
    const AdBlockRule *original_rule = css_rules_hash.value(rule->cssSelector());

    // If we don't have this selector, the exception does nothing.
    if (original_rule == NULL) {
      continue;
    }

    AdBlockRule *copied_rule = original_rule->copy();

    copied_rule->m_options |= AdBlockRule::DomainRestrictedOption;
    copied_rule->m_blockedDomains.append(rule->m_allowedDomains);
    css_rules_hash[rule->cssSelector()] = copied_rule;
    m_createdRules.append(copied_rule);
  }

  // Apparently, excessive amount of selectors for one CSS rule is not what WebKit likes.
  // (In my testings, 4931 is the number that makes it crash)
  // So let's split it by 1000 selectors.
  int hiding_rules_count = 0;

  QHashIterator<QString,const AdBlockRule*> it(css_rules_hash);

  while (it.hasNext()) {
    it.next();
    const AdBlockRule *rule = it.value();

    if (rule->isDomainRestricted()) {
      m_domainRestrictedCssRules.append(rule);
    }
    else if (hiding_rules_count == 1000) {
      m_elementHidingRules.append(rule->cssSelector());
      m_elementHidingRules.append(QL1S("{display:none !important;} "));
      hiding_rules_count = 0;
    }
    else {
      m_elementHidingRules.append(rule->cssSelector() + QL1C(','));
      hiding_rules_count++;
    }
  }

  if (hiding_rules_count != 0) {
    m_elementHidingRules = m_elementHidingRules.left(m_elementHidingRules.size() - 1);
    m_elementHidingRules.append(QLatin1String("{display:none !important;} "));
  }
}

void AdBlockMatcher::clear() {
  m_networkExceptionTree.clear();
  m_networkExceptionRules.clear();

  m_networkBlockTree.clear();
  m_networkBlockRules.clear();

  m_domainRestrictedCssRules.clear();
  m_elementHidingRules.clear();
  m_documentRules.clear();
  m_elemhideRules.clear();

  qDeleteAll(m_createdRules);
  m_createdRules.clear();
}

void AdBlockMatcher::enabledChanged(bool enabled) {
  if (enabled) {
    update();
  }
  else {
    clear();
  }
}
