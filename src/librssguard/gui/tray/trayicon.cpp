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
                   QObject* parent)
  : QObject(parent), m_id(id), m_title(title), m_normalIcon(normal_icon), m_plainIcon(plain_icon) {
  m_font.setBold(true);
}

void TrayIcon::setNumber(int number) {
  if (number <= 0 || !qApp->settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersInTrayIcon)).toBool()) {
    // Either no unread messages or numbers in tray icon are disabled.
    setToolTip(QSL(APP_LONG_NAME));
    setPixmap(m_normalIcon);
    setStatus(TrayIcon::Status::Passive);
  }
  else {
    setToolTip(tr("Unread news: %1").arg(QString::number(number)));
    QPixmap background(m_plainIcon);
    QPainter tray_painter;

    tray_painter.begin(&background);
    tray_painter.setPen(Qt::GlobalColor::white);
    tray_painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
    tray_painter.setRenderHint(QPainter::RenderHint::TextAntialiasing, false);

    // Numbers with more than 5 digits won't be readable, display
    // infinity symbol in that case.
    QString num_txt;

    if (number > 99999) {
      num_txt = QChar(8734);
    }
    else if (number > 999) {
      num_txt = QSL("%1k").arg(int(number / 1000));
    }
    else {
      num_txt = QString::number(number);
    }

    switch (num_txt.size()) {
      case 3:
        m_font.setPixelSize(background.width() * 0.48);
        break;

      case 2:
        m_font.setPixelSize(background.width() * 0.65);
        break;

      case 1:
      default:
        m_font.setPixelSize(background.width() * 0.8);
        break;
    }

    tray_painter.setFont(m_font);
    tray_painter.drawText(background.rect(), Qt::AlignmentFlag::AlignCenter, num_txt);
    tray_painter.end();

    setPixmap(background);
    setStatus(TrayIcon::Status::Active);
  }
}

bool TrayIcon::isSystemTrayDesired() {
  return qApp->settings()->value(GROUP(GUI), SETTING(GUI::UseTrayIcon)).toBool();
}
