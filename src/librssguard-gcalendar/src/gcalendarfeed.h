// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GCALENDARFEED_H
#define GCALENDARFEED_H

#include <librssguard/services/abstract/feed.h>

#include <QJsonObject>

class GCalendarFeed : public Feed {
  public:
    explicit GCalendarFeed(RootItem* parent = nullptr);
    virtual QVariantHash customDatabaseData() const override;
    virtual void setCustomDatabaseData(const QVariantHash& data) override;
    QJsonObject calendarMetadata() const;
    QJsonObject calendarResource() const;
    void setCalendarResource(const QJsonObject& resource);

  private:
    QVariantHash m_data;
};

#endif // GCALENDARFEED_H
