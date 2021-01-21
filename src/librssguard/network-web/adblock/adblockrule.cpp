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

#include "definitions/definitions.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/adblock/adblocksubscription.h"

#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QWebEnginePage>

static QString toSecondLevelDomain(const QUrl& url) {
  const QString topLevelDomain = url.topLevelDomain();
  const QString urlHost = url.host();

  if (topLevelDomain.isEmpty() || urlHost.isEmpty()) {
    return QString();
  }

  QString domain = urlHost.left(urlHost.size() - topLevelDomain.size());

  if (domain.count(QL1C('.')) == 0) {
    return urlHost;
  }

  while (domain.count(QL1C('.')) != 0) {
    domain = domain.mid(domain.indexOf(QL1C('.')) + 1);
  }

  return domain + topLevelDomain;
}

AdBlockRule::AdBlockRule(const QString& filter, AdBlockSubscription* subscription)
  : m_subscription(subscription), m_type(StringContainsMatchRule), m_caseSensitivity(Qt::CaseInsensitive),
  m_isEnabled(true), m_isException(false), m_isInternalDisabled(false) {
  setFilter(filter);
}

AdBlockRule* AdBlockRule::copy() const {
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
  rule->matchers = matchers;

  return rule;
}

AdBlockSubscription* AdBlockRule::subscription() const {
  return m_subscription;
}

void AdBlockRule::setSubscription(AdBlockSubscription* subscription) {
  m_subscription = subscription;
}

QString AdBlockRule::filter() const {
  return m_filter;
}

void AdBlockRule::setFilter(const QString& filter) {
  m_filter = filter;
  parseFilter();
}

bool AdBlockRule::isCssRule() const {
  return m_type == CssRule;
}

QString AdBlockRule::cssSelector() const {
  return m_matchString;
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
  return !m_regexPattern.isEmpty();
}

bool AdBlockRule::isInternalDisabled() const {
  return m_isInternalDisabled;
}

bool AdBlockRule::urlMatch(const QUrl& url) const {
  if (!hasOption(DocumentOption) && !hasOption(ElementHideOption)) {
    return false;
  }
  else {
    const QString encodedUrl = url.toEncoded();
    const QString domain = url.host();

    return stringMatch(domain, encodedUrl);
  }
}

bool AdBlockRule::networkMatch(const AdblockRequestInfo& request,
                               const QString& domain,
                               const QString& encoded_url) const {
  if (m_type == CssRule || !m_isEnabled || m_isInternalDisabled) {
    return false;
  }

  bool matched = stringMatch(domain, encoded_url);

  if (matched) {
    // Check domain restrictions.
    if (hasOption(DomainRestrictedOption) && !matchDomain(request.firstPartyUrl().host())) {
      return false;
    }

    // Check third-party restriction.
    if (hasOption(ThirdPartyOption) && !matchThirdParty(request)) {
      return false;
    }

    // Check object restrictions.
    if (hasOption(ObjectOption) && !matchObject(request)) {
      return false;
    }

    // Check subdocument restriction.
    if (hasOption(SubdocumentOption) && !matchSubdocument(request)) {
      return false;
    }

    // Check xmlhttprequest restriction.
    if (hasOption(XMLHttpRequestOption) && !matchXmlHttpRequest(request)) {
      return false;
    }

    // Check image restriction.
    if (hasOption(ImageOption) && !matchImage(request)) {
      return false;
    }

    // Check script restriction.
    if (hasOption(ScriptOption) && !matchScript(request)) {
      return false;
    }

    // Check stylesheet restriction.
    if (hasOption(StyleSheetOption) && !matchStyleSheet(request)) {
      return false;
    }

    // Check object-subrequest restriction.
    if (hasOption(ObjectSubrequestOption) && !matchObjectSubrequest(request)) {
      return false;
    }
  }

  return matched;
}

bool AdBlockRule::matchDomain(const QString& domain) const {
  if (!m_isEnabled) {
    return false;
  }

  if (!hasOption(DomainRestrictedOption)) {
    return true;
  }

  if (m_blockedDomains.isEmpty()) {
    for (const QString& d : m_allowedDomains) {
      if (isMatchingDomain(domain, d)) {
        return true;
      }
    }
  }
  else if (m_allowedDomains.isEmpty()) {
    for (const QString& d : m_blockedDomains) {
      if (isMatchingDomain(domain, d)) {
        return false;
      }
    }

    return true;
  }
  else {
    for (const QString& d : m_blockedDomains) {
      if (isMatchingDomain(domain, d)) {
        return false;
      }
    }

    for (const QString& d : m_allowedDomains) {
      if (isMatchingDomain(domain, d)) {
        return true;
      }
    }
  }

  return false;
}

bool AdBlockRule::matchThirdParty(const AdblockRequestInfo& request) const {
  // Third-party matching should be performed on second-level domains.
  const QString firstPartyHost = toSecondLevelDomain(request.firstPartyUrl());
  const QString host = toSecondLevelDomain(request.requestUrl());
  bool match = firstPartyHost != host;

  return hasException(ThirdPartyOption) ? !match : match;
}

bool AdBlockRule::matchObject(const AdblockRequestInfo& request) const {
  bool match = request.resourceType() == QWebEngineUrlRequestInfo::ResourceType::ResourceTypeObject;

  return hasException(ObjectOption) ? !match : match;
}

bool AdBlockRule::matchSubdocument(const AdblockRequestInfo& request) const {
  bool match = request.resourceType() == QWebEngineUrlRequestInfo::ResourceType::ResourceTypeSubFrame;

  return hasException(SubdocumentOption) ? !match : match;
}

bool AdBlockRule::matchXmlHttpRequest(const AdblockRequestInfo& request) const {
  bool match = request.resourceType() == QWebEngineUrlRequestInfo::ResourceType::ResourceTypeXhr;

  return hasException(XMLHttpRequestOption) ? !match : match;
}

bool AdBlockRule::matchImage(const AdblockRequestInfo& request) const {
  bool match = request.resourceType() == QWebEngineUrlRequestInfo::ResourceType::ResourceTypeImage;

  return hasException(ImageOption) ? !match : match;
}

bool AdBlockRule::matchScript(const AdblockRequestInfo& request) const {
  bool match = request.resourceType() == QWebEngineUrlRequestInfo::ResourceType::ResourceTypeScript;

  return hasException(ScriptOption) ? !match : match;
}

bool AdBlockRule::matchStyleSheet(const AdblockRequestInfo& request) const {
  bool match = request.resourceType() == QWebEngineUrlRequestInfo::ResourceType::ResourceTypeStylesheet;

  return hasException(StyleSheetOption) ? !match : match;
}

bool AdBlockRule::matchObjectSubrequest(const AdblockRequestInfo& request) const {
  bool match = request.resourceType() == QWebEngineUrlRequestInfo::ResourceType::ResourceTypeSubResource;

  return hasException(ObjectSubrequestOption) ? !match : match;
}

void AdBlockRule::parseFilter() {
  QString parsedLine = m_filter;

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
  if (parsedLine.contains(QL1S("##")) || parsedLine.contains(QL1S("#@#"))) {
    m_type = CssRule;
    int pos = parsedLine.indexOf(QL1C('#'));

    // Domain restricted rule.
    if (!parsedLine.startsWith(QL1S("##"))) {
      QString domains = parsedLine.left(pos);

      parseDomains(domains, QL1C(','));
    }

    m_isException = parsedLine.at(pos + 1) == QL1C('@');
    m_matchString = parsedLine.mid(m_isException ? pos + 3 : pos + 2);

    // CSS rule cannot have more options -> stop parsing.
    return;
  }

  // Exception always starts with @@.
  if (parsedLine.startsWith(QL1S("@@"))) {
    m_isException = true;
    parsedLine = parsedLine.mid(2);
  }

  // Parse all options following $ char
  int optionsIndex = parsedLine.indexOf(QL1C('$'));

  if (optionsIndex >= 0) {
    const QStringList options = parsedLine.mid(optionsIndex + 1).split(QL1C(','),
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                                                       Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                                                       QString::SkipEmptyParts);
#endif
    int handledOptions = 0;

    for (const QString& option : options) {
      if (option.startsWith(QL1S("domain="))) {
        parseDomains(option.mid(7), QL1C('|'));
        ++handledOptions;
      }
      else if (option == QL1S("match-case")) {
        m_caseSensitivity = Qt::CaseSensitivity::CaseSensitive;
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("third-party"))) {
        setOption(RuleOption::ThirdPartyOption);
        setException(RuleOption::ThirdPartyOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("object"))) {
        setOption(RuleOption::ObjectOption);
        setException(RuleOption::ObjectOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("subdocument"))) {
        setOption(RuleOption::SubdocumentOption);
        setException(RuleOption::SubdocumentOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("xmlhttprequest"))) {
        setOption(RuleOption::XMLHttpRequestOption);
        setException(RuleOption::XMLHttpRequestOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("image"))) {
        setOption(RuleOption::ImageOption);
        setException(RuleOption::ImageOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("script"))) {
        setOption(RuleOption::ScriptOption);
        setException(RuleOption::ScriptOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("stylesheet"))) {
        setOption(RuleOption::StyleSheetOption);
        setException(RuleOption::StyleSheetOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option.endsWith(QL1S("object-subrequest"))) {
        setOption(RuleOption::ObjectSubrequestOption);
        setException(RuleOption::ObjectSubrequestOption, option.startsWith(QL1C('~')));
        ++handledOptions;
      }
      else if (option == QL1S("document") && m_isException) {
        setOption(RuleOption::DocumentOption);
        ++handledOptions;
      }
      else if (option == QL1S("elemhide") && m_isException) {
        setOption(RuleOption::ElementHideOption);
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
      m_type = RuleType::Invalid;
      return;
    }

    parsedLine = parsedLine.left(optionsIndex);
  }

  // Rule is classic regexp.
  if (parsedLine.startsWith(QL1C('/')) && parsedLine.endsWith(QL1C('/'))) {
    parsedLine = parsedLine.mid(1);
    parsedLine = parsedLine.left(parsedLine.size() - 1);
    m_type = RuleType::RegExpMatchRule;
    m_regexPattern = parsedLine;
    matchers = createStringMatchers(parseRegExpFilter(parsedLine));
    return;
  }

  // Remove starting and ending wildcards (*).
  if (parsedLine.startsWith(QL1C('*'))) {
    parsedLine = parsedLine.mid(1);
  }

  if (parsedLine.endsWith(QL1C('*'))) {
    parsedLine = parsedLine.left(parsedLine.size() - 1);
  }

  // We can use fast string matching for domain here.
  if (filterIsOnlyDomain(parsedLine)) {
    parsedLine = parsedLine.mid(2);
    parsedLine = parsedLine.left(parsedLine.size() - 1);
    m_type = RuleType::DomainMatchRule;
    m_matchString = parsedLine;
    return;
  }

  // If rule contains only | at end, we can also use string matching.
  if (filterIsOnlyEndsMatch(parsedLine)) {
    parsedLine = parsedLine.left(parsedLine.size() - 1);
    m_type = RuleType::StringEndsMatchRule;
    m_matchString = parsedLine;
    return;
  }

  // If we still find a wildcard (*) or separator (^) or (|)
  // we must modify parsedLine to comply with SimpleRegExp.
  if (parsedLine.contains(QL1C('*')) || parsedLine.contains(QL1C('^')) || parsedLine.contains(QL1C('|'))) {
    m_type = RuleType::RegExpMatchRule;
    m_regexPattern = createRegExpFromFilter(parsedLine);
    matchers = createStringMatchers(parseRegExpFilter(parsedLine));
    return;
  }

  // We haven't found anything that needs use of regexp, yay!
  m_type = RuleType::StringContainsMatchRule;
  m_matchString = parsedLine;
}

void AdBlockRule::parseDomains(const QString& domains, const QChar& separator) {
  QStringList domainsList = domains.split(separator,
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                          Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                          QString::SplitBehavior::SkipEmptyParts);
#endif

  for (const QString& domain : domainsList) {
    if (domain.isEmpty()) {
      continue;
    }

    if (domain.startsWith(QL1C('~'))) {
      m_blockedDomains.append(domain.mid(1));
    }
    else {
      m_allowedDomains.append(domain);
    }
  }

  if (!m_blockedDomains.isEmpty() || !m_allowedDomains.isEmpty()) {
    setOption(RuleOption::DomainRestrictedOption);
  }
}

bool AdBlockRule::filterIsOnlyDomain(const QString& filter) const {
  if (!filter.endsWith(QL1C('^')) || !filter.startsWith(QL1S("||"))) {
    return false;
  }

  for (auto i : filter) {
    switch (i.toLatin1()) {
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

bool AdBlockRule::filterIsOnlyEndsMatch(const QString& filter) const {
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

static bool wordCharacter(const QChar& c) {
  return c.isLetterOrNumber() || c.isMark() || c == QL1C('_');
}

QString AdBlockRule::createRegExpFromFilter(const QString& filter) const {
  QString parsed;

  parsed.reserve(filter.size());
  bool hadWildcard = false; // Filter multiple wildcards.

  for (int i = 0; i < filter.size(); ++i) {
    const QChar c = filter.at(i);

    switch (c.toLatin1()) {
      case '^':
        parsed.append(QL1S("(?:[^\\w\\d\\-.%]|$)"));
        break;

      case '*':
        if (!hadWildcard) {
          parsed.append(QL1S(".*"));
        }

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

        [[fallthrough]];

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

QList<QStringMatcher> AdBlockRule::createStringMatchers(const QStringList& filters) const {
  QList<QStringMatcher> mtchrs;

  mtchrs.reserve(filters.size());

  for (const QString& filter : filters) {
    mtchrs.append(QStringMatcher(filter, m_caseSensitivity));
  }

  return mtchrs;
}

int AdBlockRule::regexMatched(const QString& str, int offset) const {
  QRegularExpression exp(m_regexPattern);

  if (m_caseSensitivity == Qt::CaseSensitivity::CaseInsensitive) {
    exp.setPatternOptions(exp.patternOptions() | QRegularExpression::PatternOption::CaseInsensitiveOption);
  }

  QRegularExpressionMatch m = exp.match(str, offset);

  if (!m.hasMatch()) {
    return -1;
  }
  else {
    return m.capturedStart();
  }
}

bool AdBlockRule::stringMatch(const QString& domain, const QString& encodedUrl) const {
  if (m_type == RuleType::StringContainsMatchRule) {
    return encodedUrl.contains(m_matchString, m_caseSensitivity);
  }
  else if (m_type == RuleType::DomainMatchRule) {
    return isMatchingDomain(domain, m_matchString);
  }
  else if (m_type == RuleType::StringEndsMatchRule) {
    return encodedUrl.endsWith(m_matchString, m_caseSensitivity);
  }
  else if (m_type == RuleType::RegExpMatchRule) {
    if (!isMatchingRegExpStrings(encodedUrl)) {
      return false;
    }
    else {
      return (regexMatched(encodedUrl) != -1);
    }
  }

  return false;
}

bool AdBlockRule::matchDomain(const QString& pattern, const QString& domain) const {
  if (pattern == domain) {
    return true;
  }

  if (!domain.endsWith(pattern)) {
    return false;
  }

  int index = domain.indexOf(pattern);

  return index > 0 && domain[index - 1] == QLatin1Char('.');
}

bool AdBlockRule::isMatchingDomain(const QString& domain, const QString& filter) const {
  return matchDomain(filter, domain);
}

bool AdBlockRule::isMatchingRegExpStrings(const QString& url) const {
  for (const QStringMatcher& matcher : matchers) {
    if (matcher.indexIn(url) == -1) {
      return false;
    }
  }

  return true;
}

// Split regexp filter into strings that can be used with QString::contains
// Don't use parts that contains only 1 char and duplicated parts.
QStringList AdBlockRule::parseRegExpFilter(const QString& filter) const {
  QStringList list;
  int startPos = -1;

  for (int i = 0; i < filter.size(); ++i) {
    const QChar c = filter.at(i);

    // Meta characters in AdBlock rules are | * ^
    if (c == QL1C('|') || c == QL1C('*') || c == QL1C('^')) {
      const QString sub = filter.mid(startPos, i - startPos);

      if (sub.size() > 1) {
        list.append(sub);
      }

      startPos = i + 1;
    }
  }

  const QString sub = filter.mid(startPos);

  if (sub.size() > 1) {
    list.append(sub);
  }

  list.removeDuplicates();
  return list;
}
