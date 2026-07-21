// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/tray/trayicon.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QMenu>
#include <QPainter>
#include <QTimer>

TrayIconMenu::TrayIconMenu(const QString& title, QWidget* parent) : QMenu(title, parent) {}

bool TrayIconMenu::event(QEvent* event) {
  if (event->type() == QEvent::Type::Show && QApplication::activeModalWidget() != nullptr) {
    QTimer::singleShot(0, this, &TrayIconMenu::hide);
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Close dialogs"),
                          tr("Close opened modal dialogs first."),
                          QSystemTrayIcon::MessageIcon::Warning});
  }

  return QMenu::event(event);
}

TrayIcon::TrayIcon(const QString& id,
                   const QString& title,
                   const QPixmap& normal_icon,
                   const QPixmap& plain_icon,
                   const QColor& text_color,
                   QObject* parent)
  : QObject(parent), m_id(id), m_title(title), m_normalIcon(normal_icon), m_plainIcon(plain_icon),
    m_textColor(text_color.isValid() ? text_color : QColor(Qt::GlobalColor::white)),
    m_showUnreadArticlesCount(qApp->settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersInTrayIcon)).toBool()) {
  m_font.setBold(true);
}

void TrayIcon::setNumber(int number) {
  if (number <= 0) {
    // Either no unread messages or numbers in tray icon are disabled.
    const bool feed_fetching_paused = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::PauseFeedFetching)).toBool();

    setToolTip(feed_fetching_paused ? tr("Feed fetching is paused.") : QSL(APP_LONG_NAME));

    if (feed_fetching_paused) {
      QPixmap background(m_plainIcon);
      QPainter tray_painter;

      tray_painter.begin(&background);
      tray_painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
      tray_painter.setRenderHint(QPainter::RenderHint::TextAntialiasing, false);

      // Keep the pause mark visually consistent with the unread-count overlay.
      const QRectF background_rect(background.rect());
      const qreal bar_width = background_rect.width() * 0.18;
      const qreal bar_height = background_rect.height() * 0.56;
      const qreal bar_gap = background_rect.width() * 0.12;
      const qreal total_width = (2.0 * bar_width) + bar_gap;
      const qreal left = background_rect.center().x() - (total_width / 2.0);
      const qreal top = background_rect.center().y() - (bar_height / 2.0);
      const qreal corner_radius = bar_width * 0.2;

      tray_painter.setPen(Qt::PenStyle::NoPen);
      tray_painter.setBrush(m_textColor);
      tray_painter.drawRoundedRect(QRectF(left, top, bar_width, bar_height), corner_radius, corner_radius);
      tray_painter.drawRoundedRect(QRectF(left + bar_width + bar_gap, top, bar_width, bar_height),
                                  corner_radius,
                                  corner_radius);

      tray_painter.end();

      setPixmap(background);
    }
    else {
      setPixmap(m_normalIcon);
    }

    setStatus(TrayIcon::Status::Passive);
  }
  else {
    setToolTip(tr("Unread news: %1").arg(QString::number(number)));

    if (m_showUnreadArticlesCount) {
      QPixmap background(m_plainIcon);
      QPainter tray_painter;

      tray_painter.begin(&background);
      tray_painter.setPen(m_textColor);
      tray_painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
      tray_painter.setRenderHint(QPainter::RenderHint::TextAntialiasing, false);

      // Numbers with more than 3 digits won't be readable, display
      // infinity symbol in that case.
      QString num_txt;

      if (number >= 100000) {
        num_txt = QChar(8734);
      }
      else if (number >= 1000) {
        // For example 15k.
        num_txt = QSL("%1k").arg(number / 1000);
      }
      else {
        num_txt = QString::number(number);
      }

      switch (num_txt.size()) {
        case 3:
          m_font.setPixelSize(background.width() * 0.55);
          break;

        case 2:
          m_font.setPixelSize(background.width() * 0.79);
          break;

        case 1:
        default:
          m_font.setPixelSize(background.width() * 0.88);
          break;
      }

      tray_painter.setFont(m_font);
      tray_painter.drawText(background.rect(), Qt::AlignmentFlag::AlignCenter, num_txt);
      tray_painter.end();

      setPixmap(background);
    }
    else {
      setPixmap(m_plainIcon);
    }

    setStatus(TrayIcon::Status::Active);
  }
}

bool TrayIcon::isSystemTrayDesired() {
  return qApp->settings()->value(GROUP(GUI), SETTING(GUI::UseTrayIcon)).toBool();
}
