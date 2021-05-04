// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/adblock/adblockmanager.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "network-web/adblock/adblockdialog.h"
#include "network-web/adblock/adblockicon.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/adblock/adblockurlinterceptor.h"
#include "network-web/networkfactory.h"
#include "network-web/networkurlinterceptor.h"
#include "network-web/webfactory.h"

#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QTimer>
#include <QUrlQuery>
#include <QWebEngineProfile>

AdBlockManager::AdBlockManager(QObject* parent)
  : QObject(parent), m_loaded(false), m_enabled(false), m_interceptor(new AdBlockUrlInterceptor(this)) {
  m_adblockIcon = new AdBlockIcon(this);
  m_adblockIcon->setObjectName(QSL("m_adblockIconAction"));

  m_unifiedFiltersFile = qApp->userDataFolder() + QDir::separator() + QSL("adblock-unified-filters.txt");
}

bool AdBlockManager::block(const AdblockRequestInfo& request) const {
  if (!isEnabled()) {
    return false;
  }

  const QString url_string = request.requestUrl().toEncoded().toLower();
  const QString url_scheme = request.requestUrl().scheme().toLower();

  if (!canRunOnScheme(url_scheme)) {
    return false;
  }
  else {
    // TODO: start server if needed, call it.
    return false;
  }
}

void AdBlockManager::load(bool initial_load) {
  auto new_enabled = qApp->settings()->value(GROUP(AdBlock), SETTING(AdBlock::AdBlockEnabled)).toBool();

  if (!initial_load) {
    new_enabled = !new_enabled;
  }

  if (new_enabled != m_enabled) {
    emit enabledChanged(new_enabled);

    qApp->settings()->setValue(GROUP(AdBlock), AdBlock::AdBlockEnabled, new_enabled);
  }
  else if (!initial_load) {
    return;
  }

  m_enabled = new_enabled;

  if (!m_loaded) {
    qApp->web()->urlIinterceptor()->installUrlInterceptor(m_interceptor);
    m_loaded = true;
  }

  if (m_enabled) {
    updateUnifiedFiltersFile();
  }
}

bool AdBlockManager::isEnabled() const {
  return m_enabled;
}

bool AdBlockManager::canRunOnScheme(const QString& scheme) const {
  return !(scheme == QSL("file") || scheme == QSL("qrc") || scheme == QSL("data") || scheme == QSL("abp"));
}

QString AdBlockManager::elementHidingRulesForDomain(const QUrl& url) const {
  // TODO: call service for cosmetic rules.
  return {};
}

QStringList AdBlockManager::filterLists() const {
  return qApp->settings()->value(GROUP(AdBlock), SETTING(AdBlock::FilterLists)).toStringList();
}

void AdBlockManager::setFilterLists(const QStringList& filter_lists) {
  qApp->settings()->setValue(GROUP(AdBlock), AdBlock::FilterLists, filter_lists);
}

QStringList AdBlockManager::customFilters() const {
  return qApp->settings()->value(GROUP(AdBlock), SETTING(AdBlock::CustomFilters)).toStringList();
}

void AdBlockManager::setCustomFilters(const QStringList& custom_filters) {
  qApp->settings()->setValue(GROUP(AdBlock), AdBlock::CustomFilters, custom_filters);
}

QString AdBlockManager::generateJsForElementHiding(const QString& css) {
  QString source = QL1S("(function() {"
                        "var head = document.getElementsByTagName('head')[0];"
                        "if (!head) return;"
                        "var css = document.createElement('style');"
                        "css.setAttribute('type', 'text/css');"
                        "css.appendChild(document.createTextNode('%1'));"
                        "head.appendChild(css);"
                        "})()");
  QString style = css;

  style.replace(QL1S("'"), QL1S("\\'"));
  style.replace(QL1S("\n"), QL1S("\\n"));

  return source.arg(style);
}

void AdBlockManager::showDialog() {
  AdBlockDialog(qApp->mainFormWidget()).exec();
}

void AdBlockManager::restartServer() {
  // TODO:
}

void AdBlockManager::updateUnifiedFiltersFile() {
  if (QFile::exists(m_unifiedFiltersFile)) {
    QFile::remove(m_unifiedFiltersFile);
  }

  // TODO: generate file
  QByteArray unified_contents;
  auto filter_lists = filterLists();

  // Download filters one by one and append.
  for (const QString& filter_list_url : qAsConst(filter_lists)) {
    QByteArray out;
    auto res = NetworkFactory::performNetworkOperation(filter_list_url,
                                                       2000,
                                                       {},
                                                       out, QNetworkAccessManager::Operation::GetOperation);

    if (res.first == QNetworkReply::NetworkError::NoError) {
      unified_contents += out;
    }
    else {
      qWarningNN << LOGSEC_ADBLOCK
                 << "Failed to download list of filters"
                 << QUOTE_W_SPACE(filter_list_url)
                 << "with error"
                 << QUOTE_W_SPACE_DOT(res.first);
    }
  }

  unified_contents += customFilters().join(QSL("\n")).toUtf8();

  // Save.
  m_unifiedFiltersFile = IOFactory::getSystemFolder(QStandardPaths::StandardLocation::TempLocation) +
                         QDir::separator() +
                         QSL("adblock.filters");

  try {
    IOFactory::writeFile(m_unifiedFiltersFile, unified_contents);

    if (m_enabled) {
      // TODO: re-start nodejs adblock server.
      restartServer();
    }
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_ADBLOCK
                << "Failed to write unified filters to file, error:"
                << QUOTE_W_SPACE_DOT(ex.message());
  }
}
