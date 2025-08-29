// For license of this file, see <project-root-folder>/LICENSE.md.

#include "filtering/messagefilter.h"

#include "exceptions/filteringexception.h"
#include "filtering/filterutils.h"
#include "miscellaneous/application.h"

MessageFilter::MessageFilter(int id, QObject* parent) : QObject(parent), m_id(id) {}

MessageObject::FilteringAction MessageFilter::filterMessage(QJSEngine* engine) {
  QJSValue filter_func = engine->evaluate(qApp->replaceUserDataFolderPlaceholder(m_script));

  if (filter_func.isError()) {
    QJSValue::ErrorType error = filter_func.errorType();
    QString message = filter_func.toString();

    throw FilteringException(error, message);
  }

  auto filter_output = engine->evaluate(QSL("filterMessage()"));

  if (filter_output.isError()) {
    QJSValue::ErrorType error = filter_output.errorType();
    QString message = filter_output.toString();

    throw FilteringException(error, message);
  }

  return MessageObject::FilteringAction(filter_output.toInt());
}

int MessageFilter::id() const {
  return m_id;
}

QString MessageFilter::name() const {
  return m_name;
}

void MessageFilter::setName(const QString& name) {
  m_name = name;
}

QString MessageFilter::script() const {
  return m_script;
}

void MessageFilter::setScript(const QString& script) {
  m_script = script;
}

void MessageFilter::initializeFilteringEngine(QJSEngine& engine, MessageObject* message_wrapper) {
  engine.installExtensions(QJSEngine::Extension::AllExtensions);
  engine.setUiLanguage(qApp->localization()->loadedLanguage());

  // Register the meta wrapper.
  auto meta_js = engine.newQMetaObject(&MessageObject::staticMetaObject);
  engine.globalObject().setProperty(QSL("Message"), meta_js);

  // Register working objects.
  auto message_js = engine.newQObject(message_wrapper);
  engine.globalObject().setProperty(QSL("msg"), message_js);

  // Register "utils".
  auto* utils = new FilterUtils(&engine);
  auto js_utils = engine.newQObject(utils);

  engine.globalObject().setProperty(QSL("utils"), js_utils);
}

void MessageFilter::setId(int id) {
  m_id = id;
}
