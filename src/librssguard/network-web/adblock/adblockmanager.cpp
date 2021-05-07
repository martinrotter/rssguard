// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/adblock/adblockmanager.h"

#include "exceptions/applicationexception.h"
#include "exceptions/networkexception.h"
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
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QProcess>
#include <QTimer>
#include <QUrlQuery>
#include <QWebEngineProfile>

AdBlockManager::AdBlockManager(QObject* parent)
  : QObject(parent), m_loaded(false), m_enabled(false), m_interceptor(new AdBlockUrlInterceptor(this)) {
  m_adblockIcon = new AdBlockIcon(this);
  m_adblockIcon->setObjectName(QSL("m_adblockIconAction"));
  m_unifiedFiltersFile = qApp->userDataFolder() + QDir::separator() + QSL("adblock-unified-filters.txt");
  m_serverProcess = new QProcess(this);
}

AdBlockManager::~AdBlockManager() {
  if (m_serverProcess->state() == QProcess::ProcessState::Running) {
    m_serverProcess->kill();
  }
}

BlockingResult AdBlockManager::block(const AdblockRequestInfo& request) const {
  if (!isEnabled()) {
    return { false };
  }

  const QString url_string = request.requestUrl().toEncoded().toLower();
  const QString url_scheme = request.requestUrl().scheme().toLower();

  if (!canRunOnScheme(url_scheme)) {
    return { false };
  }
  else {
    if (m_serverProcess->state() == QProcess::ProcessState::Running) {
      try {
        auto result = askServerIfBlocked(url_string);

        return result;
      }
      catch (const ApplicationException& ex) {
        qCriticalNN << LOGSEC_ADBLOCK
                    << "HTTP error when calling server:"
                    << QUOTE_W_SPACE_DOT(ex.message());
      }
    }
    else {
      return { false };
    }
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

BlockingResult AdBlockManager::askServerIfBlocked(const QString& url) const {
  QJsonObject req_obj;
  QByteArray out;
  QElapsedTimer tmr;

  req_obj["url"] = url;
  req_obj["filter"] = true;

  tmr.start();

  auto network_res = NetworkFactory::performNetworkOperation(QSL("http://%1:%2").arg(QHostAddress(QHostAddress::SpecialAddress::LocalHost).toString(),
                                                                                     ADBLOCK_SERVER_PORT),
                                                             500,
                                                             QJsonDocument(req_obj).toJson(),
                                                             out,
                                                             QNetworkAccessManager::Operation::PostOperation,
                                                             { {
                                                               QSL(HTTP_HEADERS_CONTENT_TYPE).toLocal8Bit(),
                                                               QSL("application/json").toLocal8Bit() } });

  if (network_res.first == QNetworkReply::NetworkError::NoError) {
    qDebugNN << LOGSEC_ADBLOCK
             << "Query to server took "
             << tmr.elapsed()
             << " ms.";

    QJsonObject out_obj = QJsonDocument::fromJson(out).object();
    bool blocking = out_obj["filter"].toObject()["match"].toBool();

    return {
      blocking,
      blocking
          ? out_obj["filter"].toObject()["filter"].toObject()["filter"].toString()
          : QString()
    };
  }
  else {
    throw NetworkException(network_res.first);
  }
}

void AdBlockManager::restartServer() {
  if (m_serverProcess->state() == QProcess::ProcessState::Running) {
    m_serverProcess->kill();

    if (!m_serverProcess->waitForFinished(1000)) {
      m_serverProcess->deleteLater();
      m_serverProcess = new QProcess(this);
    }
  }

  QString temp_server = QDir::toNativeSeparators(IOFactory::getSystemFolder(QStandardPaths::StandardLocation::TempLocation)) +
                        QDir::separator() +
                        QSL("adblock-server.js");

  if (!IOFactory::copyFile(QSL(":/scripts/adblock/adblock-server.js"), temp_server)) {
    qCriticalNN << LOGSEC_ADBLOCK << "Failed to copy server file to TEMP.";
  }

#if defined(Q_OS_WIN)
  m_serverProcess->setProgram(QSL("node.exe"));
#else
  m_serverProcess->setProgram(QSL("node"));
#endif

  m_serverProcess->setArguments({
    QDir::toNativeSeparators(temp_server),
    ADBLOCK_SERVER_PORT,
    QDir::toNativeSeparators(m_unifiedFiltersFile)
  });

  m_serverProcess->setProcessEnvironment(QProcessEnvironment::systemEnvironment());

  auto pe = m_serverProcess->processEnvironment();
  QString node_path =
#if defined(Q_OS_WIN)
    pe.value(QSL("APPDATA")) +
#elif defined(Q_OS_LINUX)
    QSL("/usr/local/lib/node_modules") +
#else
    QDir::toNativeSeparators(IOFactory::getSystemFolder(QStandardPaths::StandardLocation::GenericDataLocation)) +
#endif
    QDir::separator() +
    QSL("npm") +
    QDir::separator() +
    QSL("node_modules");

  if (!pe.contains(QSL("NODE_PATH"))) {
    pe.insert(QSL("NODE_PATH"), node_path);
  }

  m_serverProcess->setProcessEnvironment(pe);
  m_serverProcess->setProcessChannelMode(QProcess::ProcessChannelMode::ForwardedErrorChannel);

  if (!m_serverProcess->open()) {
    qWarningNN << LOGSEC_ADBLOCK << "Failed to start server.";
  }
  else {
    qDebugNN << LOGSEC_ADBLOCK << "Started server.";
  }
}

void AdBlockManager::updateUnifiedFiltersFile() {
  if (QFile::exists(m_unifiedFiltersFile)) {
    QFile::remove(m_unifiedFiltersFile);
  }

  QString unified_contents;
  auto filter_lists = filterLists();

  // Download filters one by one and append.
  for (const QString& filter_list_url : qAsConst(filter_lists)) {
    QByteArray out;
    auto res = NetworkFactory::performNetworkOperation(filter_list_url,
                                                       2000,
                                                       {},
                                                       out,
                                                       QNetworkAccessManager::Operation::GetOperation);

    if (res.first == QNetworkReply::NetworkError::NoError) {
      unified_contents = unified_contents.append(QString::fromUtf8(out));
      unified_contents = unified_contents.append('\n');

      qDebugNN << LOGSEC_ADBLOCK
               << "Downloaded filter list from"
               << QUOTE_W_SPACE_DOT(filter_list_url);
    }
    else {
      qWarningNN << LOGSEC_ADBLOCK
                 << "Failed to download list of filters"
                 << QUOTE_W_SPACE(filter_list_url)
                 << "with error"
                 << QUOTE_W_SPACE_DOT(res.first);
    }
  }

  unified_contents = unified_contents.append(customFilters().join(QSL("\n")));

  // Save.
  m_unifiedFiltersFile = IOFactory::getSystemFolder(QStandardPaths::StandardLocation::TempLocation) +
                         QDir::separator() +
                         QSL("adblock.filters");

  try {
    IOFactory::writeFile(m_unifiedFiltersFile, unified_contents.toUtf8());

    if (m_enabled) {
      restartServer();
    }
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_ADBLOCK
                << "Failed to write unified filters to file, error:"
                << QUOTE_W_SPACE_DOT(ex.message());
  }
}
