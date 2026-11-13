// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gcalendarfeed.h"

#include <librssguard/exceptions/applicationexception.h>

GCalendarFeed::GCalendarFeed(RootItem* parent) : Feed(parent) {}

QVariantHash GCalendarFeed::customDatabaseData() const {
  return m_data;
}

void GCalendarFeed::setCustomDatabaseData(const QVariantHash& data) {
  m_data = data;
}

QJsonObject GCalendarFeed::calendarMetadata() const {
  return QJsonObject::fromVariantHash(m_data).value(QStringLiteral("gcalendar")).toObject();
}

QJsonObject GCalendarFeed::calendarResource() const {
  const auto metadata = calendarMetadata();
  return metadata.value(QStringLiteral("version")).toInt() == 1 ? metadata.value(QStringLiteral("resource")).toObject()
                                                                : QJsonObject();
}

void GCalendarFeed::setCalendarResource(const QJsonObject& resource) {
  const auto value = QJsonObject::fromVariantHash(m_data).value(QStringLiteral("gcalendar"));
  if ((!value.isUndefined() && (!value.isObject() || value.toObject().value(QStringLiteral("version")).toInt() != 1)) ||
      resource.value(QStringLiteral("id")).toString().isEmpty()) {
    throw ApplicationException(QObject::tr("Invalid or unsupported calendar metadata."));
  }
  auto metadata = value.toObject();
  metadata[QStringLiteral("version")] = 1;
  metadata[QStringLiteral("resource")] = resource;
  m_data[QStringLiteral("gcalendar")] = metadata.toVariantHash();
  setCustomId(resource.value(QStringLiteral("id")).toString());
  const auto local_title = resource.value(QStringLiteral("summaryOverride")).toString();
  setTitle(local_title.isEmpty() ? resource.value(QStringLiteral("summary")).toString(customId()) : local_title);
}
