// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messagesmodelcache.h"

#include "core/messagesmodel.h"

MessagesModelCache::MessagesModelCache(QObject* parent) : QObject(parent) {}

void MessagesModelCache::setData(const QModelIndex& index, const QVariant& value) {
  if (!m_msgCache.contains(index.row())) {
    m_msgCache[index.row()] = static_cast<const MessagesModel*>(index.model())->record(index.row());
  }

  m_msgCache[index.row()].setValue(index.column(), value);
}

QVariant MessagesModelCache::data(const QModelIndex& idx) {
  return m_msgCache[idx.row()].value(idx.column());
}
