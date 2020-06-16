// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGESMODELCACHE_H
#define MESSAGESMODELCACHE_H

#include <QObject>

#include "core/message.h"

#include <QModelIndex>
#include <QVariant>

class MessagesModelCache : public QObject {
  Q_OBJECT

  public:
    explicit MessagesModelCache(QObject* parent = nullptr);
    virtual ~MessagesModelCache() = default;

    bool containsData(int row_idx) const;
    QSqlRecord record(int row_idx) const;
    QVariant data(const QModelIndex& idx);

    void clear();
    void setData(const QModelIndex& index, const QVariant& value, const QSqlRecord& record);

  private:
    QHash<int, QSqlRecord> m_msgCache;
};

inline bool MessagesModelCache::containsData(int row_idx) const {
  return m_msgCache.contains(row_idx);
}

inline QSqlRecord MessagesModelCache::record(int row_idx) const {
  return m_msgCache.value(row_idx);
}

inline void MessagesModelCache::clear() {
  m_msgCache.clear();
}

#endif // MESSAGESMODELCACHE_H
