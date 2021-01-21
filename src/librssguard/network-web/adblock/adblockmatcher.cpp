// For license of this file, see <project-root-folder>/LICENSE.md.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "network-web/adblock/adblockmatcher.h"

#include "definitions/definitions.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/adblock/adblockrule.h"
#include "network-web/adblock/adblocksubscription.h"

AdBlockMatcher::AdBlockMatcher(AdBlockManager* manager)
  : QObject(manager), m_manager(manager) {}

AdBlockMatcher::~AdBlockMatcher() {
  clear();
}

const AdBlockRule* AdBlockMatcher::match(const AdblockRequestInfo& request, const QString& urlDomain,
                                         const QString& urlString) const {
  // Exception rules.
  if (m_networkExceptionTree.find(request, urlDomain, urlString) != nullptr) {
    return nullptr;
  }

  int count = m_networkExceptionRules.count();

  for (int i = 0; i < count; ++i) {
    const AdBlockRule* rule = m_networkExceptionRules.at(i);

    if (rule->networkMatch(request, urlDomain, urlString)) {
      return nullptr;
    }
  }

  // Block rules.
  if (const AdBlockRule* rule = m_networkBlockTree.find(request, urlDomain, urlString)) {
    return rule;
  }

  count = m_networkBlockRules.count();

  for (int i = 0; i < count; ++i) {
    const AdBlockRule* rule = m_networkBlockRules.at(i);

    if (rule->networkMatch(request, urlDomain, urlString)) {
      return rule;
    }
  }

  return nullptr;
}

bool AdBlockMatcher::adBlockDisabledForUrl(const QUrl& url) const {
  int count = m_documentRules.count();

  for (int i = 0; i < count; ++i) {
    if (m_documentRules.at(i)->urlMatch(url)) {
      return true;
    }
  }

  return false;
}

bool AdBlockMatcher::elemHideDisabledForUrl(const QUrl& url) const {
  if (adBlockDisabledForUrl(url)) {
    return true;
  }

  int count = m_elemhideRules.count();

  for (int i = 0; i < count; ++i) {
    if (m_elemhideRules.at(i)->urlMatch(url)) {
      return true;
    }
  }

  return false;
}

QString AdBlockMatcher::elementHidingRules() const {
  return m_elementHidingRules;
}

QString AdBlockMatcher::elementHidingRulesForDomain(const QString& domain) const {
  QString rules;
  int addedRulesCount = 0;
  int count = m_domainRestrictedCssRules.count();

  for (int i = 0; i < count; ++i) {
    const AdBlockRule* rule = m_domainRestrictedCssRules.at(i);

    if (!rule->matchDomain(domain)) {
      continue;
    }

    if (Q_UNLIKELY(addedRulesCount == 1000)) {
      rules.append(rule->cssSelector());
      rules.append(QSL("{display:none !important;}\n"));
      addedRulesCount = 0;
    }
    else {
      rules.append(rule->cssSelector() + QLatin1Char(','));
      addedRulesCount++;
    }
  }

  if (addedRulesCount != 0) {
    rules = rules.left(rules.size() - 1);
    rules.append(QSL("{display:none !important;}\n"));
  }

  return rules;
}

void AdBlockMatcher::update() {
  clear();
  QHash<QString, const AdBlockRule*> cssRulesHash;
  QVector<const AdBlockRule*> exceptionCssRules;

  for (AdBlockSubscription* subscription : m_manager->subscriptions()) {
    for (const AdBlockRule* rule : subscription->allRules()) {
      // Don't add internally disabled rules to cache.
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
          exceptionCssRules.append(rule);
        }
        else {
          cssRulesHash.insert(rule->cssSelector(), rule);
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

  for (const AdBlockRule* rule : exceptionCssRules) {
    const AdBlockRule* originalRule = cssRulesHash.value(rule->cssSelector());

    // If we don't have this selector, the exception does nothing.
    if (originalRule == nullptr) {
      continue;
    }

    AdBlockRule* copiedRule = originalRule->copy();

    copiedRule->m_options |= AdBlockRule::RuleOption::DomainRestrictedOption;
    copiedRule->m_blockedDomains.append(rule->m_allowedDomains);
    cssRulesHash[rule->cssSelector()] = copiedRule;
    m_createdRules.append(copiedRule);
  }

  // Apparently, excessive amount of selectors for one CSS rule is not what WebKit likes.
  // (In my testings, 4931 is the number that makes it crash).
  // So let's split it by 1000 selectors.
  int hidingRulesCount = 0;
  QHashIterator<QString, const AdBlockRule*> it(cssRulesHash);

  while (it.hasNext()) {
    it.next();
    const AdBlockRule* rule = it.value();

    if (rule->isDomainRestricted()) {
      m_domainRestrictedCssRules.append(rule);
    }
    else if (Q_UNLIKELY(hidingRulesCount == 1000)) {
      m_elementHidingRules.append(rule->cssSelector());
      m_elementHidingRules.append(QL1S("{display:none !important;} "));
      hidingRulesCount = 0;
    }
    else {
      m_elementHidingRules.append(rule->cssSelector() + QLatin1Char(','));
      hidingRulesCount++;
    }
  }

  if (hidingRulesCount != 0) {
    m_elementHidingRules = m_elementHidingRules.left(m_elementHidingRules.size() - 1);
    m_elementHidingRules.append(QL1S("{display:none !important;} "));
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
