// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messagesmodelcache.h"

MessagesModelCache::MessagesModelCache(QObject* parent) : QObject(parent) {}

void MessagesModelCache::setData(const QModelIndex& index, const QVariant& value, const QSqlRecord& record) {
  if (!m_msgCache.contains(index.row())) {
    m_msgCache[index.row()] = record;
  }

  m_msgCache[index.row()].setValue(index.column(), value);
}

QVariant MessagesModelCache::data(const QModelIndex& idx) {
  return m_msgCache[idx.row()].value(idx.column());
}
