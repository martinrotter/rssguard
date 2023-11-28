// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/adblock/adblockmanager.h"

#include "3rd-party/boolinq/boolinq.h"
#include "exceptions/applicationexception.h"
#include "exceptions/networkexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "network-web/adblock/adblockdialog.h"
#include "network-web/adblock/adblockicon.h"
#include "network-web/adblock/adblockrequestinfo.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"

#if defined(NO_LITE)
#include "network-web/adblock/adblockurlinterceptor.h"
#include "network-web/webengine/networkurlinterceptor.h"
#endif

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QThread>
#include <QTimer>
#include <QUrlQuery>

AdBlockManager::AdBlockManager(QObject* parent)
  : QObject(parent), m_loaded(false), m_enabled(false), m_installing(false),
#if defined(NO_LITE)
    m_interceptor(new AdBlockUrlInterceptor(this)),
#endif
    m_serverProcess(nullptr), m_cacheBlocks({}) {
  m_adblockIcon = new AdBlockIcon(this);
  m_adblockIcon->setObjectName(QSL("m_adblockIconAction"));
  m_unifiedFiltersFile = qApp->userDataFolder() + QDir::separator() + QSL("adblock-unified-filters.txt");

  connect(qApp->nodejs(), &NodeJs::packageInstalledUpdated, this, &AdBlockManager::onPackageReady);
  connect(qApp->nodejs(), &NodeJs::packageError, this, &AdBlockManager::onPackageError);
}

AdBlockManager::~AdBlockManager() {
  killServer();
}

BlockingResult AdBlockManager::block(const AdblockRequestInfo& request) {
  if (!isEnabled()) {
    return {false};
  }

  const QString url_string = request.requestUrl().toEncoded().toLower();
  const QString firstparty_url_string = request.firstPartyUrl().toEncoded().toLower();
  const QString url_scheme = request.requestUrl().scheme().toLower();
  const QPair<QString, QString> url_pair = {firstparty_url_string, url_string};
  const QString url_type = request.resourceType();

  if (!canRunOnScheme(url_scheme)) {
    return {false};
  }
  else {
    if (m_cacheBlocks.contains(url_pair)) {
      qDebugNN << LOGSEC_ADBLOCK << "Found blocking data in cache, URL:" << QUOTE_W_SPACE_DOT(url_pair);

      return m_cacheBlocks.value(url_pair);
    }

    if (m_serverProcess != nullptr && m_serverProcess->state() == QProcess::ProcessState::Running) {
      try {
        auto result = askServerIfBlocked(firstparty_url_string, url_string, url_type);

        m_cacheBlocks.insert(url_pair, result);

        qDebugNN << LOGSEC_ADBLOCK << "Inserted blocking data to cache for:" << QUOTE_W_SPACE_DOT(url_pair);

        return result;
      }
      catch (const ApplicationException& ex) {
        qCriticalNN << LOGSEC_ADBLOCK
                    << "HTTP error when calling server for blocking rules:" << QUOTE_W_SPACE_DOT(ex.message());
        return {false};
      }
    }
    else {
      return {false};
    }
  }
}

void AdBlockManager::setEnabled(bool enabled) {
  if (enabled == m_enabled) {
    return;
  }

  if (!m_loaded) {
#if defined(NO_LITE)
    qApp->web()->urlIinterceptor()->installUrlInterceptor(m_interceptor);
#endif
    m_loaded = true;
  }

  m_enabled = enabled;
  emit enabledChanged(m_enabled);

  if (m_enabled) {
    if (!m_installing) {
      m_installing = true;
      qApp->nodejs()->installUpdatePackages({{QSL(CLIQZ_ADBLOCKED_PACKAGE), QSL(CLIQZ_ADBLOCKED_VERSION)}});
    }
  }
  else {
    killServer();
  }
}

bool AdBlockManager::isEnabled() const {
  return m_enabled;
}

bool AdBlockManager::canRunOnScheme(const QString& scheme) const {
  return !(scheme == QSL("file") || scheme == QSL("qrc") || scheme == QSL("data") || scheme == QSL("abp"));
}

QString AdBlockManager::elementHidingRulesForDomain(const QUrl& url) const {
  if (m_serverProcess != nullptr && m_serverProcess->state() == QProcess::ProcessState::Running) {
    try {
      auto result = askServerForCosmeticRules(url.toString());

      return result;
    }
    catch (const ApplicationException& ex) {
      qCriticalNN << LOGSEC_ADBLOCK
                  << "HTTP error when calling server for cosmetic rules:" << QUOTE_W_SPACE_DOT(ex.message());
      return {};
    }
  }
  else {
    return {};
  }
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
  QString source = QSL("(function() {"
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

void AdBlockManager::onPackageReady(const QList<NodeJs::PackageMetadata>& pkgs, bool already_up_to_date) {
  Q_UNUSED(already_up_to_date)

  bool concerns_adblock = boolinq::from(pkgs).any([](const NodeJs::PackageMetadata& pkg) {
    return pkg.m_name == QSL(CLIQZ_ADBLOCKED_PACKAGE);
  });

  if (concerns_adblock) {
    m_installing = false;

    if (m_enabled) {
      try {
        updateUnifiedFiltersFileAndStartServer();
      }
      catch (const ApplicationException& ex) {
        qCriticalNN << LOGSEC_ADBLOCK << "Failed to setup filters and start server:" << QUOTE_W_SPACE_DOT(ex.message());

        m_enabled = false;
        emit enabledChanged(m_enabled, tr("Failed to setup filters and start server: %1.").arg(ex.message()));
      }
    }
  }
}

void AdBlockManager::onPackageError(const QList<NodeJs::PackageMetadata>& pkgs, const QString& error) {
  bool concerns_adblock = boolinq::from(pkgs).any([](const NodeJs::PackageMetadata& pkg) {
    return pkg.m_name == QSL(CLIQZ_ADBLOCKED_PACKAGE);
  });

  if (concerns_adblock) {
    m_installing = false;
    m_enabled = false;

    qCriticalNN << LOGSEC_ADBLOCK << "Needed Node.js packages were not installed:" << QUOTE_W_SPACE_DOT(error);

    emit processTerminated();
  }
}

void AdBlockManager::onServerProcessFinished(int exit_code, QProcess::ExitStatus exit_status) {
  Q_UNUSED(exit_status)
  killServer();

  qCriticalNN << LOGSEC_ADBLOCK << "Process exited with exit code" << QUOTE_W_SPACE(exit_code)
              << "so check application log for more details.";

  m_enabled = false;
  emit processTerminated();
}

BlockingResult AdBlockManager::askServerIfBlocked(const QString& fp_url,
                                                  const QString& url,
                                                  const QString& url_type) const {
  QJsonObject req_obj;
  QByteArray out;
  QElapsedTimer tmr;

  req_obj[QSL("fp_url")] = fp_url;
  req_obj[QSL("url")] = url;
  req_obj[QSL("url_type")] = url_type, req_obj[QSL("filter")] = true;

  tmr.start();

  auto network_res =
    NetworkFactory::performNetworkOperation(QSL("http://%1:%2")
                                              .arg(QHostAddress(QHostAddress::SpecialAddress::LocalHost).toString(),
                                                   QString::number(ADBLOCK_SERVER_PORT)),
                                            500,
                                            QJsonDocument(req_obj).toJson(),
                                            out,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            {{QSL(HTTP_HEADERS_CONTENT_TYPE).toLocal8Bit(),
                                              QSL("application/json").toLocal8Bit()}});

  if (network_res.m_networkError == QNetworkReply::NetworkError::NoError) {
    qDebugNN << LOGSEC_ADBLOCK << "Query for blocking info to server took " << tmr.elapsed() << " ms.";

    QJsonObject out_obj = QJsonDocument::fromJson(out).object();
    bool blocking = out_obj[QSL("filter")].toObject()[QSL("match")].toBool();

    return {blocking,
            blocking ? out_obj[QSL("filter")].toObject()[QSL("filter")].toObject()[QSL("filter")].toString()
                     : QString()};
  }
  else {
    throw NetworkException(network_res.m_networkError);
  }
}

QString AdBlockManager::askServerForCosmeticRules(const QString& url) const {
  QJsonObject req_obj;
  QByteArray out;
  QElapsedTimer tmr;

  req_obj[QSL("url")] = url;
  req_obj[QSL("cosmetic")] = true;

  tmr.start();

  auto network_res =
    NetworkFactory::performNetworkOperation(QSL("http://%1:%2")
                                              .arg(QHostAddress(QHostAddress::SpecialAddress::LocalHost).toString(),
                                                   QString::number(ADBLOCK_SERVER_PORT)),
                                            500,
                                            QJsonDocument(req_obj).toJson(),
                                            out,
                                            QNetworkAccessManager::Operation::PostOperation,
                                            {{QSL(HTTP_HEADERS_CONTENT_TYPE).toLocal8Bit(),
                                              QSL("application/json").toLocal8Bit()}});

  if (network_res.m_networkError == QNetworkReply::NetworkError::NoError) {
    qDebugNN << LOGSEC_ADBLOCK << "Query for cosmetic rules to server took " << tmr.elapsed() << " ms.";

    QJsonObject out_obj = QJsonDocument::fromJson(out).object();

    return out_obj[QSL("cosmetic")].toObject()[QSL("styles")].toString();
  }
  else {
    throw NetworkException(network_res.m_networkError);
  }
}

QProcess* AdBlockManager::startServer(int port) {
  QString temp_server =
    QDir::toNativeSeparators(IOFactory::getSystemFolder(QStandardPaths::StandardLocation::TempLocation)) +
    QDir::separator() + QSL(ADBLOCK_SERVER_FILE);

  if (!IOFactory::copyFile(QSL(":/scripts/adblock/") + QSL(ADBLOCK_SERVER_FILE), temp_server)) {
    qWarningNN << LOGSEC_ADBLOCK << "Failed to copy server file to TEMP.";
  }

  QProcess* proc = new QProcess(this);

  proc->setProcessChannelMode(QProcess::ProcessChannelMode::ForwardedErrorChannel);

  connect(proc,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this,
          &AdBlockManager::onServerProcessFinished);

  qApp->nodejs()->runScript(proc,
                            QDir::toNativeSeparators(temp_server),
                            {QString::number(port), QDir::toNativeSeparators(m_unifiedFiltersFile)});

  qDebugNN << LOGSEC_ADBLOCK << "Attempting to start AdBlock server.";
  return proc;
}

void AdBlockManager::killServer() {
  if (m_serverProcess != nullptr) {
    disconnect(m_serverProcess,
               QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
               this,
               &AdBlockManager::onServerProcessFinished);

    if (m_serverProcess->state() == QProcess::ProcessState::Running) {
      m_serverProcess->kill();
    }

    m_serverProcess->deleteLater();
    m_serverProcess = nullptr;
  }
}

void AdBlockManager::updateUnifiedFiltersFileAndStartServer() {
  m_cacheBlocks.clear();
  killServer();

  if (QFile::exists(m_unifiedFiltersFile)) {
    QFile::remove(m_unifiedFiltersFile);
  }

  QString unified_contents;
  auto filter_lists = filterLists();

  // Download filters one by one and append.
  for (const QString& filter_list_url : std::as_const(filter_lists)) {
    if (filter_list_url.simplified().isEmpty()) {
      continue;
    }

    QByteArray out;
    auto res = NetworkFactory::performNetworkOperation(filter_list_url,
                                                       2000,
                                                       {},
                                                       out,
                                                       QNetworkAccessManager::Operation::GetOperation);

    if (res.m_networkError == QNetworkReply::NetworkError::NoError) {
      unified_contents = unified_contents.append(QString::fromUtf8(out));
      unified_contents = unified_contents.append('\n');

      qDebugNN << LOGSEC_ADBLOCK << "Downloaded filter list from" << QUOTE_W_SPACE_DOT(filter_list_url);
    }
    else {
      throw NetworkException(res.m_networkError, tr("failed to download filter list '%1'").arg(filter_list_url));
    }
  }

  unified_contents = unified_contents.append(customFilters().join(QSL("\n")));

  // Save.
  m_unifiedFiltersFile = IOFactory::getSystemFolder(QStandardPaths::StandardLocation::TempLocation) +
                         QDir::separator() + QSL("adblock.filters");

  IOFactory::writeFile(m_unifiedFiltersFile, unified_contents.toUtf8());

  if (m_enabled) {
    auto custom_port = qApp->customAdblockPort();

    m_serverProcess = startServer(custom_port > 0 ? custom_port : ADBLOCK_SERVER_PORT);
  }
}
