#ifndef FEEDSMODELSTANDARDFEED_H
#define FEEDSMODELSTANDARDFEED_H

#include "core/feedsmodelfeed.h"

#include <QDateTime>
#include <QSqlRecord>


class Message;

// Represents STANDARD RSS/RDF/ATOM feed with no
// online synchronization services (NO TT-RSS, NO FEEDLY).
// So, parent item is either root item or category.
class FeedsModelStandardFeed : public FeedsModelFeed {
  public:
    // Constructors and destructors.
    explicit FeedsModelStandardFeed(FeedsModelRootItem *parent_item = NULL);
    virtual ~FeedsModelStandardFeed();

    // Obtains data related to this feed.
    QVariant data(int column, int role) const;

    // Perform fetching of new messages.
    void update();

    // Removes this standard feed from persistent
    // storage.
    bool removeItself();

    // Various getters/setters.
    inline QString encoding() const {
      return m_encoding;
    }

    inline void setEncoding(const QString &encoding) {
      m_encoding = encoding;
    }

    inline QString url() const {
      return m_url;
    }

    inline void setUrl(const QString &url) {
      m_url = url;
    }

    // Loads standard feed object from given SQL record.
    static FeedsModelStandardFeed *loadFromRecord(const QSqlRecord &record);

  protected:
    // Persistently stores given messages into the database
    // and updates existing messages if newer version is
    // available.
    void updateMessages(const QList<Message>  &messages);

  private:
    QString m_encoding;
    QString m_url;
};

#endif // FEEDSMODELSTANDARDFEED_H
