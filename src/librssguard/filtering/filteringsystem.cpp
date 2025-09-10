// For license of this file, see <project-root-folder>/LICENSE.md.

#include "filtering/filteringsystem.h"

#include "exceptions/filteringexception.h"
#include "miscellaneous/application.h"
#include "services/abstract/labelsnode.h"

FilteringSystem::FilteringSystem(FiteringUseCase mode,
                                 QSqlDatabase& db,
                                 Feed* feed,
                                 ServiceRoot* account,
                                 QObject* parent)
  : QObject(parent), m_mode(mode), m_db(db), m_feed(feed), m_account(account) {
  initializeEngine();

  m_availableLabels =
    (m_account != nullptr && m_account->labelsNode() != nullptr) ? m_account->labelsNode()->labels() : QList<Label*>();

  m_filterFeed.setSystem(this);
  m_filterApp.setSystem(this);
  m_filterMessage.setSystem(this);
  m_filterUtils.setSystem(this);
  m_filterAccount.setSystem(this);
}

void FilteringSystem::setMessage(Message* message) {
  m_filterMessage.setMessage(message);
}

FilterMessage::FilteringAction FilteringSystem::filterMessage(const MessageFilter& filter) {
  QJSValue filter_func = m_engine.evaluate(qApp->replaceUserDataFolderPlaceholder(filter.script()));

  if (filter_func.isError()) {
    QJSValue::ErrorType error = filter_func.errorType();
    QString message = filter_func.toString();

    throw FilteringException(error, message);
  }

  auto filter_output = m_engine.evaluate(QSL("filterMessage()"));

  if (filter_output.isError()) {
    QJSValue::ErrorType error = filter_output.errorType();
    QString message = filter_output.toString();

    throw FilteringException(error, message);
  }

  return FilterMessage::FilteringAction(filter_output.toInt());
}

QJSEngine& FilteringSystem::engine() {
  return m_engine;
}

FilterMessage& FilteringSystem::message() {
  return m_filterMessage;
}

void FilteringSystem::initializeEngine() {
  m_engine.installExtensions(QJSEngine::Extension::ConsoleExtension | QJSEngine::Extension::GarbageCollectionExtension);
  m_engine.setUiLanguage(qApp->localization()->loadedLanguage());

  // msg
  m_engine.globalObject().setProperty(QSL("Msg"), m_engine.newQMetaObject(&FilterMessage::staticMetaObject));
  m_engine.globalObject().setProperty(QSL("msg"), m_engine.newQObject(&m_filterMessage));

  // feed
  m_engine.globalObject().setProperty(QSL("feed"), m_engine.newQObject(&m_filterFeed));

  // acc
  m_engine.globalObject().setProperty(QSL("acc"), m_engine.newQObject(&m_filterAccount));

  // utils
  m_engine.globalObject().setProperty(QSL("utils"), m_engine.newQObject(&m_filterUtils));

  // app
  m_engine.globalObject().setProperty(QSL("App"), m_engine.newQMetaObject(&FilterApp::staticMetaObject));
  m_engine.globalObject().setProperty(QSL("app"), m_engine.newQObject(&m_filterApp));

  // run
  m_engine.globalObject().setProperty(QSL("run"), m_engine.newQObject(&m_filterRun));
}

FilterAccount& FilteringSystem::filterAccount() {
  return m_filterAccount;
}

FilterRun& FilteringSystem::filterRun() {
  return m_filterRun;
}

FilteringSystem::FiteringUseCase FilteringSystem::mode() const {
  return m_mode;
}

QList<Label*>& FilteringSystem::availableLabels() {
  return m_availableLabels;
}

QSqlDatabase& FilteringSystem::database() {
  return m_db;
}

ServiceRoot* FilteringSystem::account() const {
  return m_account;
}

Feed* FilteringSystem::feed() const {
  return m_feed;
}
