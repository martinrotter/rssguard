// For license of this file, see <project-root-folder>/LICENSE.md.

#include "calendareventpreviewer.h"

#include "src/gcalendarfeed.h"
#include "src/gcalendarserviceroot.h"

#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/network-web/webfactory.h>

namespace {
  bool canOpen(const QUrl& url) {
    return url.isValid() && !url.host().isEmpty() &&
           (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"));
  }
} // namespace

CalendarEventPreviewer::CalendarEventPreviewer(GCalendarServiceRoot* account, QWidget* parent)
  : CustomMessagePreviewer(parent), m_account(account), m_webView(new WebBrowser(nullptr, this)) {
  m_ui.setupUi(this);
  m_ui.m_layout->addWidget(m_webView.data(), 1);
  m_webView->setToolBarVisible(false);
  m_ui.m_lblState->label()->setWordWrap(true);
  m_ui.m_lblState->label()->setTextFormat(Qt::PlainText);
  m_ui.m_btnNewEvent->setIcon(qApp->icons()->fromTheme(QSL("list-add")));
  m_ui.m_btnEdit->setIcon(qApp->icons()->fromTheme(QSL("document-edit")));
  m_ui.m_btnGoogle->setIcon(qApp->icons()->fromTheme(QSL("internet-web-browser")));
  m_ui.m_btnJoin->setIcon(qApp->icons()->fromTheme(QSL("camera-web")));
  connect(m_ui.m_btnNewEvent, &QPushButton::clicked, this, [this]() {
    if (m_account != nullptr) {
      m_account->createEvent(CalendarEvent::fromMessage(m_message).m_calendarId);
    }
  });
  connect(m_ui.m_btnEdit, &QPushButton::clicked, this, [this]() {
    if (m_account != nullptr) {
      m_account->editEvent(m_message);
    }
  });
  connect(m_ui.m_btnGoogle, &QPushButton::clicked, this, [this]() {
    openLink(m_googleUrl);
  });
  connect(m_ui.m_btnJoin, &QPushButton::clicked, this, [this]() {
    openLink(m_meetingUrl);
  });
  connect(account, &GCalendarServiceRoot::eventSaved, this, [this](int id) {
    if (id > 0 && id == m_message.m_id && m_account != nullptr) {
      try {
        const auto message = m_account->cachedMessage(id);
        loadMessage(message, m_selectedItem.data(), m_feed.data());
        emit articleTweaked(message);
      }
      catch (const ApplicationException& ex) {
        setStatus(WidgetWithStatus::StatusType::Error, ex.message());
      }
    }
  });
  connect(account, &GCalendarServiceRoot::accountStopped, this, &CalendarEventPreviewer::clear);
  connect(account, &QObject::destroyed, this, &CalendarEventPreviewer::clear);
  clear();
}

void CalendarEventPreviewer::setStatus(WidgetWithStatus::StatusType status, const QString& text) {
  m_ui.m_lblState->setStatus(status, text, text);
  m_ui.m_lblState->setVisible(!text.isEmpty());
}

WebBrowser* CalendarEventPreviewer::webBrowser() const {
  return m_webView.data();
}

void CalendarEventPreviewer::clear() {
  m_message = Message();
  m_selectedItem.clear();
  m_feed.clear();
  m_googleUrl = QUrl();
  m_meetingUrl = QUrl();
  m_webView->clear(false);
  setStatus(WidgetWithStatus::StatusType::Information, {});
  m_ui.m_btnEdit->setEnabled(false);
  m_ui.m_btnGoogle->setEnabled(false);
  m_ui.m_btnJoin->setEnabled(false);
}

void CalendarEventPreviewer::loadMessage(const Message& message, RootItem* selected_item, Feed* feed) {
  clear();
  m_message = message;
  m_selectedItem = selected_item;
  m_feed = feed;
  const auto event = CalendarEvent::fromMessage(message);
  if (!event.isValid()) {
    setStatus(WidgetWithStatus::StatusType::Error, event.m_error);
    return;
  }
  auto rendered_message = message;
  rendered_message.m_contents = event.toMessage().m_contents;
  m_webView->loadMessage(rendered_message, selected_item, feed);
  auto* calendar = m_account != nullptr
                     ? dynamic_cast<GCalendarFeed*>(m_account->getHashedSubTreeFeeds().value(event.m_calendarId))
                     : nullptr;
  const bool editable = calendar != nullptr && !calendar->isSwitchedOff() && !message.m_isDeleted &&
                        !message.m_isPdeleted &&
                        event.canEdit(calendar->calendarResource().value(QSL("accessRole")).toString());
  m_ui.m_btnEdit->setEnabled(editable);
  if (event.isCancelled()) {
    setStatus(WidgetWithStatus::StatusType::Warning, tr("This event is cancelled."));
  }
  else if (!editable) {
    setStatus(WidgetWithStatus::StatusType::Information, tr("This event is read-only."));
  }
  m_googleUrl = QUrl(event.m_resource.value(QSL("htmlLink")).toString());
  m_meetingUrl = QUrl(event.m_resource.value(QSL("hangoutLink")).toString());
  for (const auto& value :
       event.m_resource.value(QSL("conferenceData")).toObject().value(QSL("entryPoints")).toArray()) {
    const auto entry = value.toObject();
    if (entry.value(QSL("entryPointType")).toString() == QLatin1String("video") &&
        canOpen(QUrl(entry.value(QSL("uri")).toString()))) {
      m_meetingUrl = QUrl(entry.value(QSL("uri")).toString());
      break;
    }
  }
  m_ui.m_btnGoogle->setEnabled(canOpen(m_googleUrl));
  m_ui.m_btnJoin->setEnabled(canOpen(m_meetingUrl));
}

void CalendarEventPreviewer::openLink(const QUrl& url) {
  if (canOpen(url)) {
    qApp->web()->openUrlInExternalBrowser(url);
  }
}
