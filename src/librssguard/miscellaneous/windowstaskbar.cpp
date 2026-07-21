// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/windowstaskbar.h"

#if defined(Q_OS_WIN)

#include "definitions/globals.h"

#include <QAction>
#include <QApplication>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QVector>
#include <ShObjIdl.h>

#if QT_VERSION_MAJOR == 5
#include <QtWinExtras/QtWin>
#endif

namespace {
constexpr quint32 TaskbarButtonPauseFeedFetching = 1;
constexpr quint32 TaskbarButtonFetchAll = 2;
constexpr quint32 TaskbarButtonSettings = 3;
constexpr int TaskbarButtonsCount = 3;

HICON toHicon(const QImage& image) {
#if QT_VERSION_MAJOR == 5
    return QtWin::toHICON(QPixmap::fromImage(image));
#else
    return image.toHICON();
#endif
  }

HICON toHicon(const QIcon& icon) {
#if QT_VERSION_MAJOR == 5
    return QtWin::toHICON(icon.pixmap(QSize(16, 16)));
#else
    return icon.pixmap(QSize(16, 16)).toImage().toHICON();
#endif
}

QString buttonTooltip(const QAction& action) {
  QString tooltip = action.text();

  tooltip.remove('&');
  return tooltip;
}

TBPFLAG nativeProgressState(WindowsTaskbar::ProgressState state) {
  switch (state) {
    case WindowsTaskbar::ProgressState::None:
      return TBPF_NOPROGRESS;

    case WindowsTaskbar::ProgressState::Normal:
      return TBPF_NORMAL;

    case WindowsTaskbar::ProgressState::Error:
      return TBPF_ERROR;

    case WindowsTaskbar::ProgressState::Paused:
      return TBPF_PAUSED;

    case WindowsTaskbar::ProgressState::Indeterminate:
      return TBPF_INDETERMINATE;
  }

  return TBPF_NOPROGRESS;
}
} // namespace

WindowsTaskbar::WindowsTaskbar(QObject* parent)
  : QObject(parent), m_taskbar(nullptr), m_thumbnailWindowId(0), m_thumbnailButtonsEnabled(true),
    m_thumbnailButtonsReady(false), m_thumbnailButtonsAdded(false) {
  const GUID iid_taskbar_list4 = {0xc43dc798, 0x95d1, 0x4bea, {0x90, 0x30, 0xbb, 0x99, 0xe2, 0x98, 0x3a, 0x1a}};
  const HRESULT create_result = CoCreateInstance(CLSID_TaskbarList,
                                                 nullptr,
                                                 CLSCTX_INPROC_SERVER,
                                                 iid_taskbar_list4,
                                                 reinterpret_cast<void**>(&m_taskbar));

  if (FAILED(create_result)) {
    qCriticalNN << LOGSEC_CORE << "Taskbar integration for Windows failed to initialize with HRESULT:"
                << QUOTE_W_SPACE_DOT(create_result);
    m_taskbar = nullptr;
  }
  else {
    const HRESULT init_result = m_taskbar->HrInit();

    if (FAILED(init_result)) {
      qCriticalNN << LOGSEC_CORE << "Taskbar integration for Windows failed to initialize with inner HRESULT:"
                  << QUOTE_W_SPACE_DOT(init_result);
      m_taskbar->Release();
      m_taskbar = nullptr;
    }
  }
}

WindowsTaskbar::~WindowsTaskbar() {
  if (m_taskbar != nullptr) {
    m_taskbar->Release();
  }
}

bool WindowsTaskbar::isAvailable() const {
  return m_taskbar != nullptr;
}

void WindowsTaskbar::setThumbnailActions(const QList<QAction*>& actions,
                                         const QIcon& pause_icon,
                                         const QIcon& resume_icon) {
  disconnect(nullptr, nullptr, this, nullptr);
  m_thumbnailActions.clear();

  for (QAction* action : actions) {
    m_thumbnailActions.append(action);

#if QT_VERSION_MAJOR > 5
    connect(action, &QAction::enabledChanged, this, &WindowsTaskbar::updateThumbnailButtons);
#else
    connect(action, &QAction::changed, this, &WindowsTaskbar::updateThumbnailButtons);
#endif
  }

  if (!m_thumbnailActions.isEmpty()) {
    connect(m_thumbnailActions.constFirst(), &QAction::toggled, this, &WindowsTaskbar::updateThumbnailButtons);
  }

  m_pauseIcon = pause_icon;
  m_resumeIcon = resume_icon;
  updateThumbnailButtons();
}

void WindowsTaskbar::setThumbnailButtonsEnabled(bool enabled) {
  m_thumbnailButtonsEnabled = enabled;
  updateThumbnailButtons();
}

void WindowsTaskbar::thumbnailButtonsCreated(WId window_id) {
  m_thumbnailWindowId = window_id;
  m_thumbnailButtonsReady = true;
  m_thumbnailButtonsAdded = false;
  updateThumbnailButtons();
}

bool WindowsTaskbar::triggerThumbnailButton(quint32 button_id) {
  if (m_thumbnailActions.size() != TaskbarButtonsCount) {
    return false;
  }

  QAction* action = nullptr;

  switch (button_id) {
    case TaskbarButtonPauseFeedFetching:
      action = m_thumbnailActions.at(0);
      break;

    case TaskbarButtonFetchAll:
      action = m_thumbnailActions.at(1);
      break;

    case TaskbarButtonSettings:
      action = m_thumbnailActions.at(2);
      break;

    default:
      return false;
  }

  if (action == nullptr || !action->isEnabled()) {
    return false;
  }

  action->trigger();
  return true;
}

bool WindowsTaskbar::setUnreadOverlayIcon(WId window_id, int number, bool show_pause) const {
  return setOverlayIcon(window_id, generateOverlayIcon(number, show_pause));
}

bool WindowsTaskbar::clearOverlayIcon(WId window_id) const {
  return setOverlayIcon(window_id, {});
}

bool WindowsTaskbar::setProgressState(WId window_id, ProgressState state) const {
  return isAvailable() && reportResult(m_taskbar->SetProgressState(reinterpret_cast<HWND>(window_id),
                                                                    nativeProgressState(state)),
                                       QSL("set taskbar progress state"));
}

bool WindowsTaskbar::setOverlayIcon(WId window_id, const QImage& icon) const {
  if (!isAvailable()) {
    return false;
  }

  HICON hicon = icon.isNull() ? nullptr : toHicon(icon);
  const HRESULT result = m_taskbar->SetOverlayIcon(reinterpret_cast<HWND>(window_id), hicon, nullptr);

  if (hicon != nullptr) {
    DestroyIcon(hicon);
  }

  return reportResult(result, QSL("set taskbar overlay icon"));
}

bool WindowsTaskbar::setProgressValue(WId window_id, qulonglong current, qulonglong total) const {
  return setProgressState(window_id, ProgressState::Normal) &&
         reportResult(m_taskbar->SetProgressValue(reinterpret_cast<HWND>(window_id), current, total),
                      QSL("set taskbar progress value"));
}

bool WindowsTaskbar::clearProgress(WId window_id) const {
  return setProgressState(window_id, ProgressState::None);
}

void WindowsTaskbar::updateThumbnailButtons() {
  if (!isThumbnailButtonReady()) {
    return;
  }

  if (!m_thumbnailButtonsEnabled) {
    hideThumbnailButtons();
    return;
  }

  const bool updated = setThumbnailButtons(m_thumbnailWindowId,
                                            thumbnailButtons(true),
                                            !m_thumbnailButtonsAdded);

  if (updated) {
    m_thumbnailButtonsAdded = true;
  }
}

void WindowsTaskbar::hideThumbnailButtons() {
  if (m_thumbnailButtonsAdded) {
    setThumbnailButtons(m_thumbnailWindowId, thumbnailButtons(false), false);
  }
}

QList<WindowsTaskbar::ThumbnailButton> WindowsTaskbar::thumbnailButtons(bool visible) const {
  QList<ThumbnailButton> buttons;

  if (m_thumbnailActions.size() != TaskbarButtonsCount) {
    return buttons;
  }

  const QAction* pause_action = m_thumbnailActions.at(0);
  const bool feed_fetching_paused = pause_action->isChecked();
  const QList<QIcon> icons = {feed_fetching_paused ? m_resumeIcon : m_pauseIcon,
                               m_thumbnailActions.at(1)->icon(),
                               m_thumbnailActions.at(2)->icon()};
  const QStringList tooltips = {feed_fetching_paused ? tr("Resume automatic feed fetching")
                                                      : tr("Pause automatic feed fetching"),
                                buttonTooltip(*m_thumbnailActions.at(1)),
                                buttonTooltip(*m_thumbnailActions.at(2))};
  const quint32 ids[] = {TaskbarButtonPauseFeedFetching, TaskbarButtonFetchAll, TaskbarButtonSettings};

  for (int i = 0; i < TaskbarButtonsCount; ++i) {
    const QAction* action = m_thumbnailActions.at(i);

    buttons.append({int(ids[i]), icons.at(i), tooltips.at(i), action->isEnabled(), visible});
  }

  return buttons;
}

QImage WindowsTaskbar::generateOverlayIcon(int number, bool show_pause) const {
  QImage image(128, 128, QImage::Format::Format_ARGB32);
  QPainter painter;
  QString number_text;

  if (!show_pause) {
    if (number < 1000) {
      number_text = QString::number(number);
    }
    else if (number < 100000) {
      number_text = QSL("%1k").arg(number / 1000);
    }
    else {
      number_text = QChar(8734);
    }
  }

  QPainterPath rounded_rectangle;
  rounded_rectangle.addRoundedRect(QRectF(image.rect()), 15, 15);
  painter.begin(&image);
  painter.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
  painter.setRenderHint(QPainter::RenderHint::TextAntialiasing, true);
  image.fill(Qt::GlobalColor::transparent);
  painter.fillPath(rounded_rectangle, Qt::GlobalColor::white);
  painter.setPen(Qt::GlobalColor::black);
  painter.drawPath(rounded_rectangle);

  if (show_pause) {
    const QRectF image_rect(image.rect());
    const qreal bar_width = image_rect.width() * 0.18;
    const qreal bar_height = image_rect.height() * 0.56;
    const qreal bar_gap = image_rect.width() * 0.12;
    const qreal total_width = (2.0 * bar_width) + bar_gap;
    const qreal left = image_rect.center().x() - (total_width / 2.0);
    const qreal top = image_rect.center().y() - (bar_height / 2.0);
    const qreal corner_radius = bar_width * 0.2;

    painter.setPen(Qt::PenStyle::NoPen);
    painter.setBrush(Qt::GlobalColor::black);
    painter.drawRoundedRect(QRectF(left, top, bar_width, bar_height), corner_radius, corner_radius);
    painter.drawRoundedRect(QRectF(left + bar_width + bar_gap, top, bar_width, bar_height),
                            corner_radius,
                            corner_radius);
  }
  else {
    QFont font = QApplication::font();

    if (number_text.size() == 3) {
      font.setPixelSize(image.width() * 0.52);
    }
    else if (number_text.size() == 2) {
      font.setPixelSize(image.width() * 0.68);
    }
    else {
      font.setPixelSize(image.width() * 0.79);
    }

    painter.setFont(font);
    painter.drawText(image.rect().marginsRemoved(QMargins(0, 0, 0, image.height() * 0.05)),
                     number_text,
                     QTextOption(Qt::AlignmentFlag::AlignCenter));
  }

  painter.end();
  return image;
}

bool WindowsTaskbar::isThumbnailButtonReady() const {
  return isAvailable() && m_thumbnailButtonsReady && m_thumbnailWindowId != 0 &&
         m_thumbnailActions.size() == TaskbarButtonsCount &&
         !m_thumbnailActions.at(0).isNull() && !m_thumbnailActions.at(1).isNull() && !m_thumbnailActions.at(2).isNull();
}

bool WindowsTaskbar::setThumbnailButtons(WId window_id, const QList<ThumbnailButton>& buttons, bool add) const {
  if (!isAvailable()) {
    return false;
  }

  QVector<THUMBBUTTON> native_buttons(buttons.size());
  QList<HICON> icons;

  for (int i = 0; i < buttons.size(); ++i) {
    const ThumbnailButton& source_button = buttons.at(i);
    THUMBBUTTON& native_button = native_buttons[i];

    native_button.iId = source_button.m_id;
    native_button.dwMask = THB_FLAGS;
    native_button.dwFlags =
      source_button.m_visible ? (source_button.m_enabled ? THBF_ENABLED : THBF_DISABLED) : THBF_HIDDEN;

    if (source_button.m_visible) {
      native_button.dwMask |= THB_ICON | THB_TOOLTIP;
      native_button.hIcon = toHicon(source_button.m_icon);
      source_button.m_tooltip.left(MAX_PATH - 1).toWCharArray(native_button.szTip);
      icons.append(native_button.hIcon);
    }
  }

  const HRESULT result =
    add ? m_taskbar->ThumbBarAddButtons(reinterpret_cast<HWND>(window_id), native_buttons.size(), native_buttons.data())
        : m_taskbar->ThumbBarUpdateButtons(reinterpret_cast<HWND>(window_id),
                                           native_buttons.size(),
                                           native_buttons.data());

  for (HICON icon : icons) {
    if (icon != nullptr) {
      DestroyIcon(icon);
    }
  }

  return reportResult(result, add ? QSL("add taskbar thumbnail buttons") : QSL("update taskbar thumbnail buttons"));
}

bool WindowsTaskbar::reportResult(long result, const QString& operation) const {
  if (SUCCEEDED(result)) {
    return true;
  }

  qCriticalNN << LOGSEC_GUI << "Failed to" << operation << "with HRESULT:" << QUOTE_W_SPACE_DOT(result);
  return false;
}

#endif
