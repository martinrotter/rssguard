// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messagefilter.h"

#include "core/message.h"
#include "exceptions/filteringexception.h"

MessageFilter::MessageFilter(int id, QObject* parent) : QObject(parent), m_id(id) {}

MessageObject::FilteringAction MessageFilter::filterMessage(QJSEngine* engine) {
  // NOTE: Filter is represented by JavaScript code, each filter must define
  // function with "filterMessage()" prototype. There is a global "msg" object
  // representing "message" available.
  //
  // All standard classes/functions as specified by ECMA-262 are available.
  //
  // MessageObject "msg" global object has some writable properties such as "title" or "author",
  // see core/message.h file for more info.
  //
  // Note that function "filterMessage() must return integer values corresponding
  // to enumeration "FilteringAction" (see file core/message.h).
  // Also, there is a method MessageObject.isDuplicateWithAttribute(int) which is callable
  // with "msg" variable and this method checks if given message already exists in
  // RSS Guard's database. Method is parameterized and the parameter is integer representation
  // of DuplicationAttributeCheck enumeration (see file core/message.h).
  //
  // Example filtering script might look like this:
  //
  //  function helper() {
  //    if (msg.title.includes("A")) {
  //      msg.isImportant = true;
  //    }
  //
  //    return 1;
  //  }
  //
  //  function filterMessage() {
  //    return helper();
  //  }

  QJSValue filter_func = engine->evaluate(m_script);

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
  engine.installExtensions(QJSEngine::Extension::ConsoleExtension);
  engine.globalObject().setProperty("MSG_ACCEPT", int(MessageObject::FilteringAction::Accept));
  engine.globalObject().setProperty("MSG_IGNORE", int(MessageObject::FilteringAction::Ignore));

  // Register the wrapper.
  auto js_object = engine.newQObject(message_wrapper);
  auto js_meta_object = engine.newQMetaObject(&message_wrapper->staticMetaObject);

  engine.globalObject().setProperty("msg", js_object);
  engine.globalObject().setProperty(message_wrapper->staticMetaObject.className(), js_meta_object);
}

void MessageFilter::setId(int id) {
  m_id = id;
}
