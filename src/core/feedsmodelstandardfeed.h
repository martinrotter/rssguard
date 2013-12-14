#ifndef FEEDSMODELSTANDARDFEED_H
#define FEEDSMODELSTANDARDFEED_H

#include <QDateTime>
#include <QSqlRecord>

#include "core/feedsmodelfeed.h"


// Represents STANDARD RSS/RDF/ATOM feed with no
// online synchronization services (NO TT-RSS, NO FEEDLY).
// So, parent item is either root item or category.
class FeedsModelStandardFeed : public FeedsModelFeed {
  public:
    // Constructors and destructors.
    explicit FeedsModelStandardFeed(FeedsModelRootItem *parent_item = NULL);
    virtual ~FeedsModelStandardFeed();

    QVariant data(int column, int role) const;

    // Perform fetching of new messages.
    void update();

    // Various getters/setters.
    QString description() const;
    void setDescription(const QString &description);

    QDateTime creationDate() const;
    void setCreationDate(const QDateTime &creation_date);

    QString encoding() const;
    void setEncoding(const QString &encoding);

    QString url() const;
    void setUrl(const QString &url);

    QString language() const;
    void setLanguage(const QString &language);

    static FeedsModelStandardFeed *loadFromRecord(const QSqlRecord &record);

  private:
    QDateTime m_creationDate;
    QString m_encoding;
    QString m_url;
    QString m_description;
    QString m_language;
};

#endif // FEEDSMODELSTANDARDFEED_H
