// For license of this file, see <project-root-folder>/LICENSE.md.

#include <QHash>
#include <QUrl>
#include <QFile>
#include <QStringList>
#include <QStringLiteral>
#include <QRegularExpression>

static QStringList s_tlds = {};

static void loadTlds() {
  QFile fl(QStringLiteral(":/scripts/public_suffix_list.dat"));

  QByteArray data;

  if (fl.open(QIODevice::OpenModeFlag::Text | QIODevice::OpenModeFlag::Unbuffered | QIODevice::OpenModeFlag::ReadOnly)) {
    data = fl.readAll();
    fl.close();
  }

  QString str_data = QString::fromUtf8(data);

  s_tlds << str_data.split(QStringLiteral("\n"), Qt::SplitBehaviorFlags::SkipEmptyParts).filter(QRegularExpression("^[^/].+$"));
}

static bool containsTldEntry(const QString& entry) {
  if (s_tlds.isEmpty()) {
    loadTlds();

    std::sort(s_tlds.begin(), s_tlds.end(), [=](const QString& lhs, const QString& rhs) {
      return lhs.compare(rhs) < 0;
    });
  }

  return std::binary_search(s_tlds.begin(), s_tlds.end(), entry);
}

static bool isEffectiveTld(const QString& domain) {
  // for domain 'foo.bar.com':
  // 1. return if TLD table contains 'foo.bar.com'
  if (containsTldEntry(domain)) {
    return true;
  }

  if (domain.contains(QLatin1Char('.'))) {
    int count = domain.size() - domain.indexOf(QLatin1Char('.'));
    QString wild_card_domain;

    wild_card_domain.reserve(count + 1);
    wild_card_domain.append(QLatin1Char('*'));
    wild_card_domain.append(domain.rightRef(count));

    // 2. if table contains '*.bar.com',
    // test if table contains '!foo.bar.com'
    if (containsTldEntry(wild_card_domain)) {
      QString exception_domain;

      exception_domain.reserve(domain.size() + 1);
      exception_domain.append(QLatin1Char('!'));
      exception_domain.append(domain);

      return !containsTldEntry(exception_domain);
    }
  }

  return false;
}

static QString topLevelDomain(const QUrl& url) {
  auto domain = url.toString(QUrl::ComponentFormattingOption::PrettyDecoded);
  QStringList sections = domain.toLower().split(QLatin1Char('.'), Qt::SplitBehaviorFlags::SkipEmptyParts);

  if (sections.isEmpty()) {
    return QString();
  }

  QString level, tld;

  for (int j = sections.count() - 1; j >= 0; j--) {
    level.prepend(QLatin1Char('.') + sections.at(j));

    if (isEffectiveTld(level.right(level.size() - 1))) {
      tld = level;
    }
  }

  return tld;
}
