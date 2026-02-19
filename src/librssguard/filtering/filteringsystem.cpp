// For license of this file, see <project-root-folder>/LICENSE.md.

#include "filtering/filteringsystem.h"

#include "exceptions/filteringexception.h"
#include "miscellaneous/application.h"
#include "qtlinq/qtlinq.h"
#include "services/abstract/labelsnode.h"

FilteringSystem::FilteringSystem(FiteringUseCase mode, Feed* feed, ServiceRoot* account, QObject* parent)
  : QObject(parent), m_mode(mode), m_feed(feed), m_account(account) {
  initializeEngine();

  m_availableLabels =
    (m_account != nullptr && m_account->labelsNode() != nullptr) ? m_account->labelsNode()->labels() : QList<Label*>();

  m_filterFeed.setSystem(this);
  m_filterApp.setSystem(this);
  m_filterMessage.setSystem(this);
  m_filterUtils.setSystem(this);
  m_filterAccount.setSystem(this);
  m_filterFs.setSystem(this);
}

void FilteringSystem::setMessage(Message* message) {
  m_filterMessage.setMessage(message);
}

void FilteringSystem::pushMessageStatesToServices(QList<Message>& read_msgs,
                                                  QList<Message>& important_msgs,
                                                  RootItem* item,
                                                  ServiceRoot* account) {
  if (!read_msgs.isEmpty()) {
    // Now we push new read states to the service.
    account->onBeforeSetMessagesRead(item, read_msgs, RootItem::ReadStatus::Read);
  }

  if (!important_msgs.isEmpty()) {
    // Now we push new read states to the service.
    auto chngs = qlinq::from(important_msgs)
                   .select([](const Message& msg) {
                     return RootItem::ImportanceChange(msg, RootItem::Importance::Important);
                   })
                   .toList();

    account->onBeforeSwitchMessageImportance(item, chngs);
  }
}

void FilteringSystem::compareAndWriteArticleStates(Message* msg_original,
                                                   Message* msg_filtered,
                                                   QList<Message>& read_msgs,
                                                   QList<Message>& important_msgs) {
  if (!msg_original->m_isRead && msg_filtered->m_isRead) {
    qDebugNN << LOGSEC_ARTICLEFILTER << "Message with custom ID:" << QUOTE_W_SPACE(msg_original->m_customId)
             << "was marked as read by message scripts.";

    read_msgs << *msg_filtered;
  }

  if (!msg_original->m_isImportant && msg_filtered->m_isImportant) {
    qDebugNN << LOGSEC_ARTICLEFILTER << "Message with custom ID:" << QUOTE_W_SPACE(msg_original->m_customId)
             << "was marked as important by message scripts.";

    important_msgs << *msg_filtered;
  }

  // Process changed labels.
  for (Label* lbl : std::as_const(msg_original->m_assignedLabels)) {
    if (!msg_filtered->m_assignedLabels.contains(lbl)) {
      // Label is not there anymore, it was deassigned.
      msg_filtered->m_deassignedLabelsByFilter << lbl;

      qDebugNN << LOGSEC_ARTICLEFILTER << "It was detected that label" << QUOTE_W_SPACE(lbl->customId())
               << "was DEASSIGNED from message" << QUOTE_W_SPACE(msg_filtered->m_customId) << "by message filter(s).";
    }
  }

  for (Label* lbl : std::as_const(msg_filtered->m_assignedLabels)) {
    if (!msg_original->m_assignedLabels.contains(lbl)) {
      // Label is in new message, but is not in old message, it
      // was newly assigned.
      msg_filtered->m_assignedLabelsByFilter << lbl;

      qDebugNN << LOGSEC_ARTICLEFILTER << "It was detected that label" << QUOTE_W_SPACE(lbl->customId())
               << "was ASSIGNED to message" << QUOTE_W_SPACE(msg_filtered->m_customId) << "by message filter(s).";
    }
  }
}

FilterMessage::FilteringAction FilteringSystem::filterMessage(const MessageFilter& filter) {
  QJSValue filter_func = m_engine.evaluate(qApp->replaceUserDataFolderPlaceholder(filter.script(), true));

  if (filter_func.isError()) {
    throw FilteringException(filter_func);
  }

  auto filter_output = m_engine.evaluate(QSL("filterMessage()"));

  if (filter_output.isError()) {
    throw FilteringException(filter_output);
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

  // fs
  m_engine.globalObject().setProperty(QSL("fs"), m_engine.newQObject(&m_filterFs));
}

FilterApp& FilteringSystem::filterApp() {
  return m_filterApp;
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

ServiceRoot* FilteringSystem::account() const {
  return m_account;
}

Feed* FilteringSystem::feed() const {
  return m_feed;
}
