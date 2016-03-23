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

/**
 * Copyright (c) 2009, Zsombor Gegesy <gzsombor@gmail.com>
 * Copyright (c) 2009, Benjamin C. Meyer <ben@meyerhome.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Benjamin Meyer nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "network-web/adblock/adblockrule.h"

#include "network-web/adblock/adblocksubscription.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/webfactory.h"
#include "definitions/definitions.h"

#include <QUrl>
#include <QString>
#include <QStringList>
#include <QNetworkRequest>
#include <QWebEnginePage>


AdBlockRule::AdBlockRule(const QString &filter, AdBlockSubscription* subscription)
  : m_subscription(subscription), m_type(StringContainsMatchRule), m_caseSensitivity(Qt::CaseInsensitive), m_isEnabled(true),
    m_isException(false), m_isInternalDisabled(false), m_regExp(NULL) {
  setFilter(filter);
}

AdBlockRule::~AdBlockRule() {
  delete m_regExp;
}

AdBlockRule *AdBlockRule::copy() const {
  AdBlockRule* rule = new AdBlockRule();

  rule->m_subscription = m_subscription;
  rule->m_type = m_type;
  rule->m_options = m_options;
  rule->m_exceptions = m_exceptions;
  rule->m_filter = m_filter;
  rule->m_matchString = m_matchString;
  rule->m_caseSensitivity = m_caseSensitivity;
  rule->m_isEnabled = m_isEnabled;
  rule->m_isException = m_isException;
  rule->m_isInternalDisabled = m_isInternalDisabled;
  rule->m_allowedDomains = m_allowedDomains;
  rule->m_blockedDomains = m_blockedDomains;

  if (m_regExp != NULL) {
    rule->m_regExp = new RegExp();
    rule->m_regExp->regExp = m_regExp->regExp;
    rule->m_regExp->matchers = m_regExp->matchers;
  }

  return rule;
}

AdBlockSubscription *AdBlockRule::subscription() const {
  return m_subscription;
}

void AdBlockRule::setSubscription(AdBlockSubscription *subscription) {
  m_subscription = subscription;
}

QString AdBlockRule::filter() const {
  return m_filter;
}

void AdBlockRule::setFilter(const QString &filter) {
  m_filter = filter;
  parseFilter();
}

bool AdBlockRule::isDocument() const {
  return hasOption(DocumentOption);
}

bool AdBlockRule::isElemhide() const {
  return hasOption(ElementHideOption);
}

bool AdBlockRule::isDomainRestricted() const {
  return hasOption(DomainRestrictedOption);
}

bool AdBlockRule::isException() const {
  return m_isException;
}

bool AdBlockRule::isComment() const {
  return m_filter.startsWith(QL1C('!'));
}

bool AdBlockRule::isEnabled() const {
  return m_isEnabled;
}

void AdBlockRule::setEnabled(bool enabled) {
  m_isEnabled = enabled;
}

bool AdBlockRule::isSlow() const {
  return m_regExp != NULL;
}

bool AdBlockRule::isInternalDisabled() const {
  return m_isInternalDisabled;
}

bool AdBlockRule::urlMatch(const QUrl &url, const QString &referer, QWebEngineUrlRequestInfo::ResourceType resource_type) const {
  if (!hasOption(DocumentOption) && !hasOption(ElementHideOption)) {
    return false;
  }
  else {
    return networkMatch(url, url.host(), url.toEncoded(), referer, resource_type);
  }
}

bool AdBlockRule::networkMatch(const QUrl &url, const QString &domain,
                               const QString &encoded_url, const QString &referer,
                               QWebEngineUrlRequestInfo::ResourceType resource_type) const {
  if (!m_isEnabled || m_isInternalDisabled) {
    return false;
  }

  bool matched = false;

  if (m_type == StringContainsMatchRule) {
    matched = encoded_url.contains(m_matchString, m_caseSensitivity);
  }
  else if (m_type == DomainMatchRule) {
    matched = isMatchingDomain(domain, m_matchString);
  }
  else if (m_type == StringEndsMatchRule) {
    matched = encoded_url.endsWith(m_matchString, m_caseSensitivity);
  }
  else if (m_type == RegExpMatchRule) {
    if (!isMatchingRegExpStrings(encoded_url)) {
      return false;
    }

    matched = (m_regExp->regExp.indexIn(encoded_url) != -1);
  }

  if (matched) {
    // Check domain restrictions.
    if (hasOption(DomainRestrictedOption) && !matchDomain(domain)) {
      return false;
    }

    // Check third-party restriction.
    if (hasOption(ThirdPartyOption) && !matchThirdParty(referer, url)) {
      return false;
    }

    // Check object restrictions.
    if (hasOption(ObjectOption) && !matchObject(resource_type)) {
      return false;
    }

    // Check subdocument restriction.
    if (hasOption(SubdocumentOption)) {
      return false;
    }

    // Check xmlhttprequest restriction
    if (hasOption(XMLHttpRequestOption)) {
      return false;
    }

    // Check image restriction
    if (hasOption(ImageOption) && !matchImage(encoded_url)) {
      return false;
    }
  }

  return matched;
}

bool AdBlockRule::matchDomain(const QString &domain) const {
  if (!m_isEnabled) {
    return false;
  }

  if (!hasOption(DomainRestrictedOption)) {
    return true;
  }

  if (m_blockedDomains.isEmpty()) {
    foreach (const QString &d, m_allowedDomains) {
      if (isMatchingDomain(domain, d)) {
        return true;
      }
    }
  }
  else if (m_allowedDomains.isEmpty()) {
    foreach (const QString &d, m_blockedDomains) {
      if (isMatchingDomain(domain, d)) {
        return false;
      }
    }

    return true;
  }
  else {
    foreach (const QString &d, m_blockedDomains) {
      if (isMatchingDomain(domain, d)) {
        return false;
      }
    }

    foreach (const QString &d, m_allowedDomains) {
      if (isMatchingDomain(domain, d)) {
        return true;
      }
    }
  }

  return false;
}

bool AdBlockRule::matchThirdParty(const QString &referer, const QUrl &url) const {
  if (referer.isEmpty()) {
    return false;
  }

  // Third-party matching should be performed on second-level domains.
  const QString refererHost = WebFactory::instance()->toSecondLevelDomain(QUrl(referer));
  const QString host = WebFactory::instance()->toSecondLevelDomain(url);

  bool match = refererHost != host;

  return hasException(ThirdPartyOption) ? !match : match;
}

bool AdBlockRule::matchObject(QWebEngineUrlRequestInfo::ResourceType type) const {
  bool match = type == QWebEngineUrlRequestInfo::ResourceTypeObject;

  return hasException(ObjectOption) ? !match : match;
}

bool AdBlockRule::matchImage(const QString &encoded_url) const {
  bool match = encoded_url.endsWith(QL1S(".png")) ||
               encoded_url.endsWith(QL1S(".jpg")) ||
               encoded_url.endsWith(QL1S(".gif")) ||
               encoded_url.endsWith(QL1S(".jpeg"));

  return hasException(ImageOption) ? !match : match;
}

void AdBlockRule::parseFilter() {
  QString parsed_line = m_filter;

  // Empty rule or just comment.
  if (m_filter.trimmed().isEmpty() || m_filter.startsWith(QL1C('!'))) {
    // We want to differentiate rule disabled by user and rule disabled in subscription file
    // m_isInternalDisabled is also used when rule is disabled due to all options not being supported.
    m_isEnabled = false;
    m_isInternalDisabled = true;
    m_type = Invalid;
    return;
  }

  // CSS Element hiding rule.
  if (parsed_line.contains(QL1S("##")) || parsed_line.contains(QL1S("#@#"))) {
    // Do not parse CSS rules.
    return;
  }

  // Exception always starts with @@
  if (parsed_line.startsWith(QL1S("@@"))) {
    m_isException = true;
    parsed_line = parsed_line.mid(2);
  }

  // Parse all options following $ char.
  int optionsIndex = parsed_line.indexOf(QL1C('$'));
  if (optionsIndex >= 0) {
    const QStringList options = parsed_line.mid(optionsIndex + 1).split(QL1C(','), QString::SkipEmptyParts);

    int handledOptions = 0;
    foreach (const QString &option, options) {
      if (option.startsWith(QL1S("domain="))) {
        parseDomains(option.mid(7), QL1C('|'));
        ++handledOptions;
      }
      else if (option == QL1S("match-case")) {
        m_caseSensitivity = Qt::CaseSensitive;
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("third-party"))) {
        setOption(ThirdPartyOption);
        setException(ThirdPartyOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("object"))) {
        setOption(ObjectOption);
        setException(ObjectOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("subdocument"))) {
        setOption(SubdocumentOption);
        setException(SubdocumentOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("xmlhttprequest"))) {
        setOption(XMLHttpRequestOption);
        setException(XMLHttpRequestOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("image"))) {
        setOption(ImageOption);
        setException(ImageOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option == QL1S("document") && m_isException) {
        setOption(DocumentOption);
        ++handledOptions;
      }
      else if (option == QL1S("elemhide") && m_isException) {
        setOption(ElementHideOption);
        ++handledOptions;
      }
      else if (option == QL1S("collapse")) {
        // Hiding placeholders of blocked elements is enabled by default.
        ++handledOptions;
      }
    }

    // If we don't handle all options, it's safer to just disable this rule.
    if (handledOptions != options.count()) {
      m_isInternalDisabled = true;
      m_type = Invalid;
      return;
    }

    parsed_line = parsed_line.left(optionsIndex);
  }

  // Rule is classic regexp.
  if (parsed_line.startsWith(QL1C('/')) && parsed_line.endsWith(QL1C('/'))) {
    parsed_line = parsed_line.mid(1);
    parsed_line = parsed_line.left(parsed_line.size() - 1);

    m_type = RegExpMatchRule;
    m_regExp = new RegExp();
    m_regExp->regExp = QRegExp(parsed_line, m_caseSensitivity);
    m_regExp->matchers = createStringMatchers(parseRegExpFilter(parsed_line));
    return;
  }

  // Remove starting and ending wildcards (*).
  if (parsed_line.startsWith(QL1C('*'))) {
    parsed_line = parsed_line.mid(1);
  }

  if (parsed_line.endsWith(QL1C('*'))) {
    parsed_line = parsed_line.left(parsed_line.size() - 1);
  }

  // We can use fast string matching for domain here.
  if (filterIsOnlyDomain(parsed_line)) {
    parsed_line = parsed_line.mid(2);
    parsed_line = parsed_line.left(parsed_line.size() - 1);

    m_type = DomainMatchRule;
    m_matchString = parsed_line;
    return;
  }

  // If rule contains only | at end, we can also use string matching.
  if (filterIsOnlyEndsMatch(parsed_line)) {
    parsed_line = parsed_line.left(parsed_line.size() - 1);

    m_type = StringEndsMatchRule;
    m_matchString = parsed_line;
    return;
  }

  // If we still find a wildcard (*) or separator (^) or (|)
  // we must modify parsedLine to comply with QzRegExp.
  if (parsed_line.contains(QL1C('*')) ||
      parsed_line.contains(QL1C('^')) ||
      parsed_line.contains(QL1C('|'))
      ) {
    m_type = RegExpMatchRule;
    m_regExp = new RegExp;
    m_regExp->regExp = QRegExp(createRegExpFromFilter(parsed_line), m_caseSensitivity);
    m_regExp->matchers = createStringMatchers(parseRegExpFilter(parsed_line));
    return;
  }

  // We haven't found anything that needs use of regexp, yay!
  m_type = StringContainsMatchRule;
  m_matchString = parsed_line;
}

void AdBlockRule::parseDomains(const QString &domains, const QChar &separator) {
  QStringList domains_list = domains.split(separator, QString::SkipEmptyParts);

  foreach (const QString domain, domains_list) {
    if (!domain.isEmpty()) {
      if (domain.startsWith(QL1C('~'))) {
        m_blockedDomains.append(domain.mid(1));
      }
      else {
        m_allowedDomains.append(domain);
      }
    }
  }

  if (!m_blockedDomains.isEmpty() || !m_allowedDomains.isEmpty()) {
    setOption(DomainRestrictedOption);
  }
}

bool AdBlockRule::filterIsOnlyDomain(const QString &filter) const {
  if (!filter.endsWith(QL1C('^')) || !filter.startsWith(QL1S("||"))) {
    return false;
  }

  for (int i = 0; i < filter.size(); i++) {
    switch (filter.at(i).toLatin1()) {
      case '/':
      case ':':
      case '?':
      case '=':
      case '&':
      case '*':
        return false;

      default:
        break;
    }
  }

  return true;
}

bool AdBlockRule::filterIsOnlyEndsMatch(const QString &filter) const {
  for (int i = 0; i < filter.size(); ++i) {
    switch (filter.at(i).toLatin1()) {
      case '^':
      case '*':
        return false;

      case '|':
        return i == filter.size() - 1;

      default:
        break;
    }
  }

  return false;
}

static bool wordCharacter(const QChar &c) {
  return c.isLetterOrNumber() || c.isMark() || c == QL1C('_');
}

QString AdBlockRule::createRegExpFromFilter(const QString &filter) const {
  QString parsed;
  parsed.reserve(filter.size());

  bool hadWildcard = false; // Filter multiple wildcards.

  for (int i = 0; i < filter.size(); i++) {
    const QChar c = filter.at(i);

    switch (c.toLatin1()) {
      case '^':
        parsed.append(QL1S("(?:[^\\w\\d\\-.%]|$)"));
        break;

      case '*':
        if (!hadWildcard)
          parsed.append(QL1S(".*"));
        break;

      case '|':
        if (i == 0) {
          if (filter.size() > 1 && filter.at(1) == QL1C('|')) {
            parsed.append(QL1S("^[\\w\\-]+:\\/+(?!\\/)(?:[^\\/]+\\.)?"));
            i++;
          }
          else {
            parsed.append('^');
          }
          break;
        }
        else if (i == filter.size() - 1) {
          parsed.append(QL1C('$'));
          break;
        }
        // Fall through.

      default:
        if (!wordCharacter(c)) {
          parsed.append(QL1C('\\') + c);
        }
        else {
          parsed.append(c);
        }
    }

    hadWildcard = c == QL1C('*');
  }

  return parsed;
}

QList<QStringMatcher> AdBlockRule::createStringMatchers(const QStringList &filters) const {
  QList<QStringMatcher> matchers;
  matchers.reserve(filters.size());

  foreach (const QString &filter, filters) {
    matchers.append(QStringMatcher(filter, m_caseSensitivity));
  }

  return matchers;
}

bool AdBlockRule::isMatchingDomain(const QString &domain, const QString &pattern) const {
  if (pattern == domain) {
    return true;
  }

  if (!domain.endsWith(pattern)) {
    return false;
  }

  int index = domain.indexOf(pattern);

  return index > 0 && domain[index - 1] == QLatin1Char('.');
}

bool AdBlockRule::isMatchingRegExpStrings(const QString &url) const {
  Q_ASSERT(m_regExp);

  foreach (const QStringMatcher &matcher, m_regExp->matchers) {
    if (matcher.indexIn(url) == -1) {
      return false;
    }
  }

  return true;
}

// Split regexp filter into strings that can be used with QString::contains.
// Don't use parts that contains only 1 char and duplicated parts.
QStringList AdBlockRule::parseRegExpFilter(const QString &filter) const {
  QStringList list;
  int start_pos = -1;

  for (int i = 0; i < filter.size(); i++) {
    const QChar c = filter.at(i);

    // Meta characters in AdBlock rules are "| * ^".
    if (c == QL1C('|') || c == QL1C('*') || c == QL1C('^')) {
      const QString sub = filter.mid(start_pos, i - start_pos);

      if (sub.size() > 1) {
        list.append(sub);
      }

      start_pos = i + 1;
    }
  }

  const QString sub = filter.mid(start_pos);

  if (sub.size() > 1) {
    list.append(sub);
  }

  list.removeDuplicates();

  return list;
}

bool AdBlockRule::hasOption(const AdBlockRule::RuleOption &opt) const {
  return (m_options & opt);
}

bool AdBlockRule::hasException(const AdBlockRule::RuleOption &opt) const {
  return (m_exceptions & opt);
}

void AdBlockRule::setOption(const AdBlockRule::RuleOption &opt) {
  m_options |= opt;
}

void AdBlockRule::setException(const AdBlockRule::RuleOption &opt, bool on) {
  if (on) {
    m_exceptions |= opt;
  }
}
