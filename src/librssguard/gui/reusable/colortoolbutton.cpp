// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/colortoolbutton.h"

#include "gui/dialogs/filedialog.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "qtlinq/qtlinq.h"

#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QImageReader>
#include <QInputDialog>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>

ColorIconToolButton::ColorIconToolButton(QWidget* parent)
  : QToolButton(parent), m_color(Qt::GlobalColor::green), m_colorOnlyMode(true), m_menu(new QMenu(this)) {
  setToolTip(tr("Click me!"));
  setMenu(m_menu);
  setArrowType(Qt::ArrowType::NoArrow);
  setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
  connect(m_menu, &QMenu::aboutToShow, this, &ColorIconToolButton::updateMenu);
}

void ColorIconToolButton::appendExtraAction(QAction* action) {
  m_extraActions.append(action);
}

QColor ColorIconToolButton::color() const {
  return m_color;
}

void ColorIconToolButton::setColor(const QColor& color, bool inform_about_changes) {
  m_color = color;

  setIcon(IconFactory::fromColor(color), inform_about_changes);

  if (inform_about_changes) {
    emit colorChanged(m_color);
  }
}

QIcon ColorIconToolButton::icon() const {
  return QToolButton::icon();
}

void ColorIconToolButton::setIcon(const QIcon& icon, bool inform_about_changes) {
  QToolButton::setIcon(icon);

  if (inform_about_changes) {
    emit iconChanged(icon);
  }
}

void ColorIconToolButton::setRandomColor() {
  setColor(TextFactory::generateRandomColor());
}

void ColorIconToolButton::askForColor() {
  auto new_color = QColorDialog::getColor(m_color,
                                          parentWidget(),
                                          tr("Select new color"),
                                          QColorDialog::ColorDialogOption::DontUseNativeDialog |
                                            QColorDialog::ColorDialogOption::ShowAlphaChannel);

  if (new_color.isValid()) {
    setColor(new_color);
  }
}

void ColorIconToolButton::getIconFromUrl() {
  bool ok = false;
  QString src = qApp->clipboard()->text().simplified().replace(QRegularExpression("\\r|\\n"), QString());

  if (src.isEmpty()) {
    src = m_suggestedUrl;
  }

  QString url = QInputDialog::getText(window(),
                                      tr("Enter URL"),
                                      tr("Enter direct URL pointing to the image"),
                                      QLineEdit::EchoMode::Normal,
                                      src,
                                      &ok);

  if (!ok || url.isEmpty()) {
    return;
  }

  QList<IconLocation> icon_loc = {IconLocation(url, true), IconLocation(url, false)};
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QPixmap pixmap;

  if (NetworkFactory::downloadIcon(icon_loc,
                                   timeout,
                                   pixmap,
                                   {},
                                   m_proxy.has_value() ? m_proxy.value() : QNetworkProxy::ProxyType::DefaultProxy) ==
      QNetworkReply::NetworkError::NoError) {
    setIcon(QIcon(pixmap));
  }
  else {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         GuiMessage(tr("Icon not fetched"),
                                    tr("Icon was not fetched due to network error."),
                                    QSystemTrayIcon::MessageIcon::Critical),
                         GuiMessageDestination(true, true));
  }
}

void ColorIconToolButton::askForIcon() {
  auto supported_formats = QImageReader::supportedImageFormats();
  auto list_formats = qlinq::from(supported_formats)
                        .select([](const QByteArray& frmt) {
                          return QSL("*.%1").arg(QString::fromLocal8Bit(frmt));
                        })
                        .toList();

  QString fil = FileDialog::openFileName(this,
                                         tr("Select icon file"),
                                         qApp->homeFolder(),
                                         {},
                                         tr("Images (%1)").arg(list_formats.join(QL1C(' '))),
                                         nullptr,
                                         GENERAL_REMEMBERED_PATH);

  if (!fil.isEmpty()) {
    setIcon(QIcon(fil));
  }
}

void ColorIconToolButton::updateMenu() {
  m_menu->clear();
  m_menu->addAction(IconFactory::fromColor(m_color),
                    tr("Select new &color..."),
                    this,
                    &ColorIconToolButton::askForColor);

  if (m_colorOnlyMode) {
    return;
  }

  m_menu->addAction(qApp->icons()->fromTheme(QSL("image-x-generic")),
                    tr("Load icon from &file..."),
                    this,
                    &ColorIconToolButton::askForIcon);

  m_menu->addAction(qApp->icons()->fromTheme(QSL("emblem-downloads"), QSL("download")),
                    tr("Download icon from &URL..."),
                    this,
                    &ColorIconToolButton::getIconFromUrl);

  if (m_defaultIcon.has_value()) {
    m_menu->addAction(qApp->icons()->fromTheme(QSL("folder")), tr("Use default icon from icon theme"), [this]() {
      setIcon(m_defaultIcon.value());
    });
  }

  m_menu->addActions(m_extraActions);

  if (!m_additionalIcons.isEmpty()) {
    auto* icons_selection = IconFactory::iconSelectionMenu(m_menu, m_additionalIcons, [this](const QIcon& icon) {
      setIcon(icon);
    });

    m_menu->addAction(icons_selection);
  }
}

bool ColorIconToolButton::colorOnlyMode() const {
  return m_colorOnlyMode;
}

void ColorIconToolButton::setColorOnlyMode(bool color_only_mode) {
  m_colorOnlyMode = color_only_mode;
}

QColor ColorIconToolButton::alternateColor() const {
  return m_alternateColor;
}

void ColorIconToolButton::setAlternateColor(const QColor& alt_color) {
  m_alternateColor = alt_color;
}

void ColorIconToolButton::mouseReleaseEvent(QMouseEvent* event) {
  QToolButton::mouseReleaseEvent(event);

  if (event->button() == Qt::MouseButton::RightButton) {
    setColor(m_alternateColor);
  }
}

std::optional<QNetworkProxy> ColorIconToolButton::proxy() const {
  return m_proxy;
}

void ColorIconToolButton::setProxy(const QNetworkProxy& proxy) {
  m_proxy = proxy;
}

QString ColorIconToolButton::suggestedUrl() const {
  return m_suggestedUrl;
}

void ColorIconToolButton::setSuggestedUrl(const QString& url) {
  m_suggestedUrl = url;
}

QList<QIcon> ColorIconToolButton::additionalIcons() const {
  return m_additionalIcons;
}

void ColorIconToolButton::setAdditionalIcons(const QList<QIcon>& icons) {
  m_additionalIcons = icons;
}

std::optional<QIcon> ColorIconToolButton::defaultIcon() const {
  return m_defaultIcon;
}

void ColorIconToolButton::setDefaultIcon(const QIcon& icon) {
  m_defaultIcon = std::make_optional<QIcon>(icon);
}
