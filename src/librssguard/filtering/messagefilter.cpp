// For license of this file, see <project-root-folder>/LICENSE.md.

#include "filtering/messagefilter.h"

MessageFilter::MessageFilter(int id, QObject* parent) : QObject(parent), m_id(id) {}

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

void MessageFilter::setId(int id) {
  m_id = id;
}
