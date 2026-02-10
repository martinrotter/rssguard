// For license of this file, see <project-root-folder>/LICENSE.md
// and
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gui/webviewers/qlitehtml/documentcontainer.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/downloader.h"
#include "network-web/webfactory.h"

#include <algorithm>

#include <QClipboard>
#include <QColor>
#include <QConicalGradient>
#include <QCursor>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QLinearGradient>
#include <QMutexLocker>
#include <QPaintDevice>
#include <QPainter>
#include <QPalette>
#include <QPen>
#include <QPixmap>
#include <QRadialGradient>
#include <QRegularExpression>
#include <QTextBoundaryFinder>

const int kDragDistance = 5;

static QFont toQFont(litehtml::uint_ptr fnt) {
  return *reinterpret_cast<QFont*>(fnt);
}

static QPainter* toQPainter(litehtml::uint_ptr hdc, bool shape_aa) {
  QPainter* painter = reinterpret_cast<QPainter*>(hdc);

  painter->setRenderHint(QPainter::RenderHint::Antialiasing, shape_aa);

  return painter;
}

static QRectF toQRect(litehtml::position position) {
  return {position.x, position.y, position.width, position.height};
}

static bool isVisible(const litehtml::element::ptr& element) {
  // TODO: Render_item::is_visible() would also take m_skip into account, so this might be wrong.
  return element->css().get_display() != litehtml::display_none &&
         element->css().get_visibility() == litehtml::visibility_visible;
}

static litehtml::elements_list path(const litehtml::element::ptr& element) {
  litehtml::elements_list result;
  litehtml::element::ptr current = element;

  while (current) {
    result.push_back(current);
    current = current->parent();
  }

  std::reverse(std::begin(result), std::end(result));

  return result;
}

static std::tuple<litehtml::element::ptr, litehtml::element::ptr, litehtml::element::ptr>
getCommonParent(const litehtml::elements_list& a, const litehtml::elements_list& b) {
  litehtml::element::ptr parent;
  auto ait = a.cbegin();
  auto bit = b.cbegin();

  while (ait != a.cend() && bit != b.cend()) {
    if (*ait != *bit) {
      return {parent, *ait, *bit};
    }

    parent = *ait;
    ++ait;
    ++bit;
  }
  return {parent,
          (ait != a.cend() ? *ait : litehtml::element::ptr()),
          (bit != b.cend() ? *bit : litehtml::element::ptr())};
}

static std::pair<Selection::Element, Selection::Element> startAndEnd(const Selection::Element& a,
                                                                     const Selection::Element& b) {
  if (a.element == b.element) {
    if (a.index <= b.index) {
      return {a, b};
    }

    return {b, a};
  }

  const litehtml::elements_list a_path = path(a.element);
  const litehtml::elements_list b_path = path(b.element);
  litehtml::element::ptr common_parent;
  litehtml::element::ptr a_branch;
  litehtml::element::ptr b_branch;

  std::tie(common_parent, a_branch, b_branch) = getCommonParent(a_path, b_path);

  if (!common_parent) {
    qWarningNN << LOGSEC_HTMLVIEWER << "Internal error: litehtml elements do not have common parent.";
    return {a, b};
  }

  if (common_parent == a.element) {
    return {a, a}; // 'a' already contains 'b'
  }

  if (common_parent == b.element) {
    return {b, b};
  }

  // find out if a or b is first in the child sub-trees of commonParent
  for (const litehtml::element::ptr& child : common_parent->children()) {
    if (child == a_branch) {
      return {a, b};
    }

    if (child == b_branch) {
      return {b, a};
    }
  }

  qWarningNN << LOGSEC_HTMLVIEWER << "Internal error: failed to find out order of litehtml elements.";
  return {a, b};
}

// 1) stops right away if element == stop, otherwise stops whenever stop element is encountered
// 2) moves down the first children from element until there is none anymore
static litehtml::element::ptr firstLeaf(const litehtml::element::ptr& element, const litehtml::element::ptr& stop) {
  if (element == stop) {
    return element;
  }

  litehtml::element::ptr current = element;

  while (current != stop && !current->children().empty()) {
    current = current->children().front();
  }

  return current;
}

// 1) stops right away if element == stop, otherwise stops whenever stop element is encountered
// 2) starts at next sibling (up the hierarchy chain) if possible, otherwise root
// 3) returns first leaf of the element found in 2
static litehtml::element::ptr nextLeaf(const litehtml::element::ptr& element, const litehtml::element::ptr& stop) {
  if (element == stop) {
    return element;
  }

  litehtml::element::ptr current = element;

  if (!current->is_root()) {
    // find next sibling
    const litehtml::element::ptr parent = current->parent();
    const litehtml::elements_list& children = parent->children();
    auto child_it = std::find_if(children.cbegin(), children.cend(), [&current](const litehtml::element::ptr& e) {
      return e == current;
    });

    if (child_it == children.cend()) {
      qWarningNN << LOGSEC_HTMLVIEWER << "Internal error: filed to find litehtml child element in parent.";
      return stop;
    }

    ++child_it;

    if (child_it == children.cend()) { // no sibling, move up
      return nextLeaf(parent, stop);
    }

    current = *child_it;
  }
  return firstLeaf(current, stop);
}

static Selection::Element selectionDetails(const litehtml::element::ptr& element, const QString& text, QPointF pos) {
  // shortcut, which _might_ not really be correct
  if (!element->children().empty()) {
    return {element, -1, -1}; // everything selected
  }

  const QFont& font = toQFont(element->css().get_font());
  const QFontMetrics fm(font);
  int previous = 0;

  for (int i = 0; i < text.size(); ++i) {
    const int width = fm.size(0, text.left(i + 1)).width();

    if ((width + previous) / 2 >= pos.x()) {
      return {element, i, previous};
    }

    previous = width;
  }

  return {element, int(text.size()), previous};
}

// Returns whether the intended child was found and stop.
// Does a depth-first iteration over elements that "pos" is inside, and executes
// \a action with them. If \a action returns \c true, the iteration is stopped.
static bool deepestChildAtPoint(const litehtml::element::ptr& element,
                                QPointF pos,
                                QPointF viewport_pos,
                                const std::function<bool(const litehtml::element::ptr&)>& action,
                                int level = 0) {
  // TODO are there elements for which we should take viewportPos into account instead?
  // E.g. fixed position elements?
  if (!element) {
    return false /*continue iterating*/;
  }

  const QRectF placement = toQRect(element->get_placement());

  // Do not continue down elements that do not cover the position. Exceptions:
  // - elements with 0 size (includes anchors and other weird elements)
  // - html and body, which for some reason have size == viewport size
  if (!placement.size().isEmpty() && element->tag() != litehtml::_html_ && element->tag() != litehtml::_body_ &&
      !placement.contains(pos)) {
    return false /*continue iterating*/;
  }

  const litehtml::elements_list& children = element->children();

  for (auto it = children.cbegin(); it != children.cend(); ++it) {
    if (deepestChildAtPoint(*it, pos, viewport_pos, action, level + 1)) {
      return true;
    }
  }

  if (placement.contains(pos)) {
    return action(element);
  }

  return false /*continue iterating*/;
}

static Selection::Element selectionElementAtPoint(const litehtml::element::ptr& element,
                                                  QPointF pos,
                                                  QPointF viewport_pos,
                                                  Selection::Mode mode) {
  Selection::Element result;

  deepestChildAtPoint(element, pos, viewport_pos, [mode, &result, &pos](const litehtml::element::ptr& element) {
    const QRectF placement = toQRect(element->get_placement());
    std::string text;
    element->get_text(text);

    if (!text.empty()) {
      result = mode == Selection::Mode::Free
                 ? selectionDetails(element, QString::fromStdString(text), pos - placement.topLeft())
                 : Selection::Element({element, -1, -1});
      return true;
    }

    return false; /*continue*/
  });

  return result;
}

// CSS: 400 == normal, 700 == bold.
// Qt5: 50 == normal, 75 == bold
// Qt6: == CSS
static QFont::Weight cssWeightToQtWeight(int css_weight) {
#if QT_VERSION_MAJOR >= 6
  return QFont::Weight(css_weight);
#else
  if (css_weight <= 400) {
    return QFont::Weight(css_weight * 50 / 400);
  }

  if (css_weight >= 700) {
    return QFont::Weight(75 + (css_weight - 700) * 25 / 300);
  }

  return QFont::Weight(50 + (css_weight - 400) * 25 / 300);
#endif
}

static QFont::Style toQFontStyle(litehtml::font_style style) {
  switch (style) {
    case litehtml::font_style::font_style_normal:
      return QFont::Style::StyleNormal;

    case litehtml::font_style::font_style_italic:
      return QFont::Style::StyleItalic;
  }

  qWarningNN << LOGSEC_HTMLVIEWER << "Unknown litehtml font style:" << QUOTE_W_SPACE_DOT(style);
  return QFont::Style::StyleNormal;
}

static QColor toQColor(litehtml::web_color color) {
  return {color.red, color.green, color.blue, color.alpha};
}

static Qt::PenStyle borderPenStyle(litehtml::border_style style) {
  switch (style) {
    case litehtml::border_style::border_style_dotted:
      return Qt::PenStyle::DotLine;

    case litehtml::border_style::border_style_dashed:
      return Qt::PenStyle::DashLine;

    case litehtml::border_style::border_style_solid:
      return Qt::PenStyle::SolidLine;

    default:
      return Qt::PenStyle::SolidLine;
  }
}

static QPen borderToQPen(litehtml::border border) {
  return {toQColor(border.color), qreal(border.width), borderPenStyle(border.style)};
}

static QCursor toQCursor(const QString& c) {
  if (c == QSL("alias")) {
    return {Qt::CursorShape::PointingHandCursor};
  }

  if (c == QSL("all-scroll")) {
    return {Qt::CursorShape::SizeAllCursor};
  }

  if (c == QSL("auto")) {
    return {Qt::CursorShape::ArrowCursor};
  }

  if (c == QSL("cell")) {
    return {Qt::CursorShape::UpArrowCursor};
  }

  if (c == QSL("context-menu")) {
    return {Qt::CursorShape::ArrowCursor};
  }

  if (c == QSL("col-resize")) {
    return {Qt::CursorShape::SplitHCursor};
  }

  if (c == QSL("copy")) {
    return {Qt::CursorShape::DragCopyCursor};
  }

  if (c == QSL("crosshair")) {
    return {Qt::CursorShape::CrossCursor};
  }

  if (c == QSL("default")) {
    return {Qt::CursorShape::ArrowCursor};
  }

  if (c == QSL("e-resize")) {
    return {Qt::CursorShape::SizeHorCursor};
  }

  if (c == QSL("ew-resize")) {
    return {Qt::CursorShape::SizeHorCursor};
  }

  if (c == QSL("grab")) {
    return {Qt::CursorShape::OpenHandCursor};
  }

  if (c == QSL("grabbing")) {
    return {Qt::CursorShape::ClosedHandCursor};
  }

  if (c == QSL("help")) {
    return {Qt::CursorShape::WhatsThisCursor};
  }

  if (c == QSL("move")) {
    return {Qt::CursorShape::SizeAllCursor};
  }

  if (c == QSL("n-resize")) {
    return {Qt::CursorShape::SizeVerCursor};
  }

  if (c == QSL("ne-resize")) {
    return {Qt::CursorShape::SizeBDiagCursor};
  }

  if (c == QSL("nesw-resize")) {
    return {Qt::CursorShape::SizeBDiagCursor};
  }

  if (c == QSL("ns-resize")) {
    return {Qt::CursorShape::SizeVerCursor};
  }

  if (c == QSL("nw-resize")) {
    return {Qt::CursorShape::SizeFDiagCursor};
  }

  if (c == QSL("nwse-resize")) {
    return {Qt::CursorShape::SizeFDiagCursor};
  }

  if (c == QSL("no-drop")) {
    return {Qt::CursorShape::ForbiddenCursor};
  }

  if (c == QSL("none")) {
    return {Qt::CursorShape::BlankCursor};
  }

  if (c == QSL("not-allowed")) {
    return {Qt::CursorShape::ForbiddenCursor};
  }

  if (c == QSL("pointer")) {
    return {Qt::CursorShape::PointingHandCursor};
  }

  if (c == QSL("progress")) {
    return {Qt::CursorShape::BusyCursor};
  }

  if (c == QSL("row-resize")) {
    return {Qt::CursorShape::SplitVCursor};
  }

  if (c == QSL("s-resize")) {
    return {Qt::CursorShape::SizeVerCursor};
  }

  if (c == QSL("se-resize")) {
    return {Qt::CursorShape::SizeFDiagCursor};
  }

  if (c == QSL("sw-resize")) {
    return {Qt::CursorShape::SizeBDiagCursor};
  }

  if (c == QSL("text")) {
    return {Qt::CursorShape::IBeamCursor};
  }

  if (c == QSL("url")) {
    return {Qt::CursorShape::ArrowCursor};
  }

  if (c == QSL("w-resize")) {
    return {Qt::CursorShape::SizeHorCursor};
  }

  if (c == QSL("wait")) {
    return {Qt::CursorShape::BusyCursor};
  }

  if (c == QSL("zoom-in")) {
    return {Qt::CursorShape::ArrowCursor};
  }

  return {Qt::CursorShape::ArrowCursor};
}

bool Selection::isValid() const {
  return !m_selection.isEmpty();
}

void Selection::update() {
  const auto add_element = [this](const Selection::Element& element, const Selection::Element& end = {}) {
    std::string elem_text;
    element.element->get_text(elem_text);

    const QString text_str = QString::fromStdString(elem_text);

    if (!text_str.isEmpty()) {
      QRectF rect = toQRect(element.element->get_placement()).adjusted(-1, -1, 1, 1);

      if (element.index < 0) { // fully selected
        m_text += text_str;
      }
      else if (end.element) { // select from element "to end"
        if (element.element == end.element) {
          // end.index is guaranteed to be >= element.index by caller, same for x
          m_text += text_str.mid(element.index, end.index - element.index);
          const int left = rect.left();
          rect.setLeft(left + element.x);
          rect.setRight(left + end.x);
        }
        else {
          m_text += text_str.mid(element.index);
          rect.setLeft(rect.left() + element.x);
        }
      }
      else { // select from start of element
        m_text += text_str.left(element.index);
        rect.setRight(rect.left() + element.x);
      }

      m_selection.append(rect);
    }
  };

  if (m_startElem.element && m_endElem.element) {
    // Edge cases:
    // start and end elements could be reversed or children of each other
    Selection::Element start;
    Selection::Element end;
    std::tie(start, end) = startAndEnd(m_startElem, m_endElem);

    m_selection.clear();
    m_text.clear();

    // Treats start element as a leaf even if it isn't, because it already contains all its
    // children
    add_element(start, end);

    if (start.element != end.element) {
      litehtml::element::ptr current = start.element;

      do {
        current = nextLeaf(current, end.element);

        if (current == end.element) {
          add_element(end);
        }
        else {
          add_element({current, -1, -1});
        }
      }
      while (current != end.element);
    }
  }
  else {
    m_selection = {};
    m_text.clear();
  }

  QClipboard* cb = QGuiApplication::clipboard();

  if (cb != nullptr && cb->supportsSelection()) {
    cb->setText(m_text, QClipboard::Mode::Selection);
  }
}

QRectF Selection::boundingRect() const {
  QRectF rect;

  for (const QRectF& r : m_selection) {
    rect = rect.united(r);
  }

  return rect;
}

DocumentContainer::DocumentContainer()
  : m_placeholderImage(qApp->icons()->miscPixmap(QSL("image-placeholder"))),
    m_placeholderImageError(qApp->icons()->miscPixmap(QSL("image-placeholder-error"))), m_loadExternalResources(false),
    m_dataFileCacheFolder(qApp->web()->webCacheFolder()), m_downloader(new Downloader(this)) {
  m_timerForPendingExternalResources.setSingleShot(true);
  m_timerForPendingExternalResources.setInterval(100);
  connect(&m_timerForPendingExternalResources,
          &QTimer::timeout,
          this,
          &DocumentContainer::downloadNextExternalResource);

  m_timerRerender.setSingleShot(true);
  m_timerRerender.setInterval(500);

  bool created_cache_folder = QDir().mkpath(m_dataFileCacheFolder);

  if (created_cache_folder) {
    qDebugNN << LOGSEC_HTMLVIEWER
             << "Created or reused HTML viewer cache folder:" << QUOTE_W_SPACE_DOT(m_dataFileCacheFolder);
  }
  else {
    qCriticalNN << LOGSEC_HTMLVIEWER
                << "Failed to create or reuse HTML viewer cache folder:" << QUOTE_W_SPACE_DOT(m_dataFileCacheFolder);
  }

  connect(m_downloader, &Downloader::completed, this, &DocumentContainer::onResourceDownloadCompleted);
}

DocumentContainer::~DocumentContainer() = default;

litehtml::uint_ptr DocumentContainer::create_font(const litehtml::font_description& descr,
                                                  const litehtml::document* doc,
                                                  litehtml::font_metrics* fm) {
  Q_UNUSED(doc)

  QStringList split_names = QString::fromStdString(descr.family).split(QChar(','), SPLIT_BEHAVIOR::SkipEmptyParts);
  QStringList family_names;

  std::transform(split_names.cbegin(), split_names.cend(), std::back_inserter(family_names), [this](const QString& s) {
    QString name = s.trimmed();

    if (name.startsWith('\"')) {
      name = name.mid(1);
    }

    if (name.endsWith('\"')) {
      name.chop(1);
    }

    const QString lower_name = name.toLower();

    if (lower_name == QSL("serif")) {
      return serifFont();
    }

    if (lower_name == QSL("sans-serif")) {
      return sansSerifFont();
    }

    if (lower_name == QSL("monospace")) {
      return monospaceFont();
    }

    return name;
  });

  auto font = new QFont();

  font->setFamilies(family_names);
  font->setPixelSize(descr.size);
  font->setWeight(cssWeightToQtWeight(descr.weight));
  font->setStyle(toQFontStyle(descr.style));
  font->setStyleStrategy(m_fontAntialiasing ? QFont::StyleStrategy::PreferAntialias
                                            : QFont::StyleStrategy::NoAntialias);

  if (descr.decoration_line == litehtml::text_decoration_line::text_decoration_line_underline) {
    font->setUnderline(true);
  }

  if (descr.decoration_line == litehtml::text_decoration_line::text_decoration_line_overline) {
    font->setOverline(true);
  }

  if (descr.decoration_line == litehtml::text_decoration_line::text_decoration_line_line_through) {
    font->setStrikeOut(true);
  }

  if (fm != nullptr) {
    const QFontMetrics metrics(*font);

    fm->font_size = metrics.height();
    fm->height = metrics.height();
    fm->ascent = metrics.ascent();
    fm->descent = metrics.descent();
    fm->x_height = metrics.xHeight();
    fm->sub_shift = descr.size / 5;
    fm->super_shift = descr.size / 3;
    fm->draw_spaces = true;
  }
  return reinterpret_cast<litehtml::uint_ptr>(font);
}

void DocumentContainer::delete_font(litehtml::uint_ptr fnt) {
  auto font = reinterpret_cast<QFont*>(fnt);
  delete font;
}

litehtml::pixel_t DocumentContainer::text_width(const char* text, litehtml::uint_ptr fnt) {
  const QFontMetrics fm(toQFont(fnt));
  return fm.horizontalAdvance(QString::fromUtf8(text));
}

void DocumentContainer::draw_text(litehtml::uint_ptr hdc,
                                  const char* text,
                                  litehtml::uint_ptr fnt,
                                  litehtml::web_color color,
                                  const litehtml::position& pos) {
  auto painter = toQPainter(hdc, m_shapeAntialiasing);

  painter->setFont(toQFont(fnt));
  painter->setPen(toQColor(color));
  painter->drawText(toQRect(pos), Qt::TextFlag::TextDontClip, QString::fromUtf8(text));
}

litehtml::pixel_t DocumentContainer::pt_to_px(float pt) const {
  // magic factor of 11/12 to account for differences to webengine/webkit
  return m_paintDevice->physicalDpiY() * pt * 11.0f / m_paintDevice->logicalDpiY() / 12.0f;
}

litehtml::pixel_t DocumentContainer::get_default_font_size() const {
  return m_defaultFont.pointSizeF();
}

const char* DocumentContainer::get_default_font_name() const {
  return m_defaultFontFamilyName.constData();
}

void DocumentContainer::draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) {
  auto painter = toQPainter(hdc, m_shapeAntialiasing);

  if (marker.image.empty()) {
    if (marker.marker_type == litehtml::list_style_type_square) {
      painter->setPen(Qt::PenStyle::NoPen);
      painter->setBrush(toQColor(marker.color));
      painter->drawRect(toQRect(marker.pos));
    }
    else if (marker.marker_type == litehtml::list_style_type_disc) {
      painter->setPen(Qt::PenStyle::NoPen);
      painter->setBrush(toQColor(marker.color));
      painter->drawEllipse(toQRect(marker.pos));
    }
    else if (marker.marker_type == litehtml::list_style_type_circle) {
      painter->setPen(toQColor(marker.color));
      painter->setBrush(Qt::BrushStyle::NoBrush);
      painter->drawEllipse(toQRect(marker.pos));
    }
    else {
      painter->setPen(Qt::PenStyle::NoPen);
      painter->setBrush(toQColor(marker.color));
      painter->drawEllipse(toQRect(marker.pos));

      qWarningNN << LOGSEC_HTMLVIEWER << "List marker of type" << marker.marker_type << "not supported";
    }
  }
  else {
    const QPixmap pixmap = getPixmap(QString::fromStdString(marker.image), QString::fromStdString(marker.baseurl));
    painter->drawPixmap(toQRect(marker.pos), pixmap, QRectF());
  }
}

void DocumentContainer::load_image(const char* src, const char* baseurl, bool redraw_on_ready) {
  Q_UNUSED(redraw_on_ready)

  const auto qt_src = QString::fromUtf8(src);
  const auto qt_baseurl = QString::fromUtf8(baseurl);
  const QUrl url = resolveUrl(qt_src, qt_baseurl);

  // NOTE: Just initiate/request image download.
  handleExternalResource(RequestType::ImageDownload, url);
}

void DocumentContainer::get_image_size(const char* src, const char* baseurl, litehtml::size& sz) {
  const auto qt_src = QString::fromUtf8(src);
  const auto qt_baseurl = QString::fromUtf8(baseurl);

  if (qt_src.isEmpty()) {
    return;
  }

  const QPixmap pm = getPixmap(qt_src, qt_baseurl);

  sz.width = pm.width();
  sz.height = pm.height();
}

void DocumentContainer::drawSelection(QPainter* painter, const QRectF& clip) const {
  painter->save();
  painter->setClipRect(clip, Qt::ClipOperation::IntersectClip);

  for (const QRectF& r : m_selection.m_selection) {
    const QRectF client_rect = r.translated(-m_scrollPosition);
    const QPalette palette = m_paletteCallback();

    painter->fillRect(client_rect, palette.brush(QPalette::ColorRole::Highlight));
  }

  painter->restore();
}

static bool isInBody(const litehtml::element::ptr& e) {
  litehtml::element::ptr current = e;

  while (current && QString::fromUtf8(current->get_tagName()).toLower() != "body") {
    current = current->parent();
  }

  return (bool)current;
}

void DocumentContainer::buildIndex() {
  m_index.m_elementToIndex.clear();
  m_index.m_indexToElement.clear();
  m_index.m_text.clear();

  int index = 0;
  bool in_body = false;
  litehtml::element::ptr current = firstLeaf(m_document->root(), nullptr);

  while (current != m_document->root()) {
    m_index.m_elementToIndex.insert({current, index});

    if (!in_body) {
      in_body = isInBody(current);
    }

    if (in_body && isVisible(current)) {
      std::string text;
      current->get_text(text);

      if (!text.empty()) {
        m_index.m_indexToElement.push_back({index, current});
        const QString str = QString::fromStdString(text);
        m_index.m_text += str;
        index += str.size();
      }
    }

    current = nextLeaf(current, m_document->root());
  }
}

void DocumentContainer::updateSelection() {
  const QString old_text = m_selection.m_text;

  m_selection.update();

  if (!m_clipboardCallback) {
    return;
  }

  const QString new_text = m_selection.m_text;

  if (old_text.isEmpty() && !new_text.isEmpty()) {
    m_clipboardCallback(true);
  }
  else if (!old_text.isEmpty() && new_text.isEmpty()) {
    m_clipboardCallback(false);
  }
}

void DocumentContainer::clearSelection() {
  const QString old_text = m_selection.m_text;

  m_selection = {};

  if (!m_clipboardCallback) {
    return;
  }

  if (!old_text.isEmpty()) {
    m_clipboardCallback(false);
  }
}

QString DocumentContainer::userCss() const {
  return m_userCss;
}

void DocumentContainer::setUserCss(const QString& user_css) {
  m_userCss = user_css;
}

bool DocumentContainer::shapeAntialiasing() const {
  return m_shapeAntialiasing;
}

void DocumentContainer::setShapeAntialiasing(bool on) {
  m_shapeAntialiasing = on;
}

bool DocumentContainer::loadExternalResources() const {
  return m_loadExternalResources;
}

void DocumentContainer::setLoadExternalResources(bool load_resources) {
  m_loadExternalResources = load_resources;
}

void DocumentContainer::onResourceDownloadCompleted(const QUrl& url,
                                                    QNetworkReply::NetworkError status,
                                                    int http_code,
                                                    const QByteArray& contents) {
  saveExternalResourceToFileCache(url, contents);
  m_pendingExternalResources.removeOne(url);

  if (status != QNetworkReply::NetworkError::NoError) {
    qWarningNN << LOGSEC_HTMLVIEWER << "Async external data" << QUOTE_W_SPACE(url) << "was not loaded due to error"
               << QUOTE_W_SPACE_DOT(status);
  }

  if (m_pendingExternalResources.isEmpty()) {
    emit renderRequested();
  }
  else {
    downloadNextExternalResource();
  }
}

void DocumentContainer::saveExternalResourceToFileCache(const QUrl& url, const QByteArray& data) {
  if (!url.isValid()) {
    return;
  }

  QString folder = m_dataFileCacheFolder + QDir::separator() + url.host() + QDir::separator();
  QString filename = folder + generateExternalResourceCachedFilename(url);

  QDir().mkdir(folder);

  qDebugNN << LOGSEC_HTMLVIEWER << "Saving file for" << QUOTE_W_SPACE(url.toString()) << "to cache.";
  IOFactory::writeFile(filename, data);
}

QByteArray DocumentContainer::getExternalResourceFromFileCache(const QUrl& url) const {
  if (!url.isValid()) {
    throw ApplicationException(tr("invalid URL cannot be used to load file from cache"));
  }

  QString folder = m_dataFileCacheFolder + QDir::separator() + url.host() + QDir::separator();
  QString filename = folder + generateExternalResourceCachedFilename(url);

  QDir().mkdir(folder);

  return IOFactory::readFile(filename);
}

QString DocumentContainer::generateExternalResourceCachedFilename(const QUrl& url) const {
  QByteArray hash = QCryptographicHash::hash(url.toString().toUtf8(), QCryptographicHash::Algorithm::Md5);
  return hash.toHex();
}

QVariant DocumentContainer::handleExternalResource(DocumentContainer::RequestType type, const QUrl& url) {
  qDebugNN << LOGSEC_HTMLVIEWER << "Request for external resource" << QUOTE_W_SPACE(url.toString()) << "of type"
           << QUOTE_W_SPACE_DOT(int(type));

  try {
    QByteArray resource_from_file_cache = getExternalResourceFromFileCache(url);

    qDebugNN << LOGSEC_HTMLVIEWER << "Loading file for" << QUOTE_W_SPACE(url.toString()) << "from cache.";
    return resource_from_file_cache;
  }
  catch (const ApplicationException&) {
    // NOTE: File is not in file cache. Just continue.
  }

  if (!loadExternalResources()) {
    if (type == DocumentContainer::RequestType::ImageDisplay || type == DocumentContainer::RequestType::ImageDownload) {
      // External resources are not enabled, display placeholders.
      return m_placeholderImage;
    }
    else {
      // NOTE: Probably empty CSS.
      return QByteArray();
    }
  }

  if (type == DocumentContainer::RequestType::ImageDisplay) {
    // External resources are enabled and if the picture is not in the cache and we want to display it, display
    // placeholder.
    return m_placeholderImage;
  }

  if (type == DocumentContainer::RequestType::ImageDownload) {
    // Just add this resource to list of needed resources and timed downloader
    // will eventually download it and re-render the document.
    if (!m_pendingExternalResources.contains(url)) {
      qDebugNN << LOGSEC_HTMLVIEWER << "Adding" << QUOTE_W_SPACE(url) << "to list of resources for async download.";
      m_pendingExternalResources.append(url);
    }
    else {
      qDebugNN << LOGSEC_HTMLVIEWER << "Resource" << QUOTE_W_SPACE(url) << "is already queued for async download.";
    }

    return m_placeholderImage;
  }

  // Here we download CSS files synchronously.
  QByteArray data;
  NetworkResult res = NetworkFactory::performNetworkOperation(url.toString(),
                                                              3000,
                                                              {},
                                                              data,
                                                              QNetworkAccessManager::Operation::GetOperation,
                                                              {},
                                                              false,
                                                              {},
                                                              {},
                                                              networkProxy());

  if (res.m_networkError != QNetworkReply::NetworkError::NoError) {
    qWarningNN << LOGSEC_HTMLVIEWER << "External data" << QUOTE_W_SPACE(url) << "was not loaded due to error"
               << QUOTE_W_SPACE_DOT(res.m_networkError);
  }

  switch (type) {
    case DocumentContainer::RequestType::CssDownload:
    default:
      saveExternalResourceToFileCache(url, data);
      return data;
  }
}

QNetworkProxy DocumentContainer::networkProxy() const {
  return m_downloader->proxy();
}

void DocumentContainer::setNetworkProxy(const QNetworkProxy& network_proxy) {
  m_downloader->setProxy(network_proxy);
}

Downloader* DocumentContainer::downloader() const {
  return m_downloader;
}

void DocumentContainer::drawRectWithLambda(litehtml::uint_ptr hdc,
                                           const litehtml::background_layer& layer,
                                           std::function<void(QPainter*)> lmbd) {
  auto painter = toQPainter(hdc, m_shapeAntialiasing);
  const QRegion initial_clip_region = painter->clipRegion();
  const Qt::ClipOperation initial_clip_operation =
    initial_clip_region.isEmpty() ? Qt::ClipOperation::ReplaceClip : Qt::ClipOperation::IntersectClip;

  painter->save();

  if (!initial_clip_region.isEmpty()) {
    painter->setClipRegion(initial_clip_region);
  }

  painter->setClipRect(toQRect(layer.clip_box), initial_clip_operation);

  const QRegion horizontal_middle(QRect(layer.border_box.x,
                                        layer.border_box.y + layer.border_radius.top_left_y,
                                        layer.border_box.width,
                                        layer.border_box.height - layer.border_radius.top_left_y -
                                          layer.border_radius.bottom_left_y));
  const QRegion horizontal_top(QRect(layer.border_box.x + layer.border_radius.top_left_x,
                                     layer.border_box.y,
                                     layer.border_box.width - layer.border_radius.top_left_x -
                                       layer.border_radius.top_right_x,
                                     layer.border_radius.top_left_y));
  const QRegion horizontal_bottom(QRect(layer.border_box.x + layer.border_radius.bottom_left_x,
                                        layer.border_box.bottom() - layer.border_radius.bottom_left_y,
                                        layer.border_box.width - layer.border_radius.bottom_left_x -
                                          layer.border_radius.bottom_right_x,
                                        layer.border_radius.bottom_left_y));
  const QRegion top_left(QRect(layer.border_box.left(),
                               layer.border_box.top(),
                               2 * layer.border_radius.top_left_x,
                               2 * layer.border_radius.top_left_y),
                         QRegion::RegionType::Ellipse);
  const QRegion top_right(QRect(layer.border_box.right() - 2 * layer.border_radius.top_right_x,
                                layer.border_box.top(),
                                2 * layer.border_radius.top_right_x,
                                2 * layer.border_radius.top_right_y),
                          QRegion::RegionType::Ellipse);
  const QRegion bottom_left(QRect(layer.border_box.left(),
                                  layer.border_box.bottom() - 2 * layer.border_radius.bottom_left_y,
                                  2 * layer.border_radius.bottom_left_x,
                                  2 * layer.border_radius.bottom_left_y),
                            QRegion::RegionType::Ellipse);
  const QRegion bottom_right(QRect(layer.border_box.right() - 2 * layer.border_radius.bottom_right_x,
                                   layer.border_box.bottom() - 2 * layer.border_radius.bottom_right_y,
                                   2 * layer.border_radius.bottom_right_x,
                                   2 * layer.border_radius.bottom_right_y),
                             QRegion::RegionType::Ellipse);
  const QRegion clip_region = horizontal_middle.united(horizontal_top)
                                .united(horizontal_bottom)
                                .united(top_left)
                                .united(top_right)
                                .united(bottom_left)
                                .united(bottom_right);

  painter->setClipRegion(clip_region, Qt::ClipOperation::IntersectClip);
  painter->setPen(Qt::PenStyle::NoPen);
  painter->setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);

  lmbd(painter);

  painter->drawRect(layer.border_box.x, layer.border_box.y, layer.border_box.width, layer.border_box.height);

  drawSelection(painter, toQRect(layer.border_box));

  painter->restore();
}

void DocumentContainer::draw_solid_fill(litehtml::uint_ptr hdc,
                                        const litehtml::background_layer& layer,
                                        const litehtml::web_color& color) {
  if (color == litehtml::web_color::transparent) {
    return;
  }

  drawRectWithLambda(hdc, layer, [&](QPainter* painter) {
    painter->setBrush(toQColor(color));
  });
}

void DocumentContainer::draw_image(litehtml::uint_ptr hdc,
                                   const litehtml::background_layer& layer,
                                   const std::string& url,
                                   const std::string& base_url) {
  if (url.empty() || (!layer.clip_box.width && !layer.clip_box.height)) {
    return;
  }

  auto painter = toQPainter(hdc, m_shapeAntialiasing);
  const QRegion initial_clip_region = painter->clipRegion();
  const Qt::ClipOperation initial_clip_operation = initial_clip_region.isEmpty() ? Qt::ReplaceClip : Qt::IntersectClip;

  painter->save();

  if (!initial_clip_region.isEmpty()) {
    painter->setClipRegion(initial_clip_region);
  }

  painter->setClipRect(toQRect(layer.clip_box), initial_clip_operation);

  const QRegion horizontal_middle(QRect(layer.border_box.x,
                                        layer.border_box.y + layer.border_radius.top_left_y,
                                        layer.border_box.width,
                                        layer.border_box.height - layer.border_radius.top_left_y -
                                          layer.border_radius.bottom_left_y));
  const QRegion horizontal_top(QRect(layer.border_box.x + layer.border_radius.top_left_x,
                                     layer.border_box.y,
                                     layer.border_box.width - layer.border_radius.top_left_x -
                                       layer.border_radius.top_right_x,
                                     layer.border_radius.top_left_y));
  const QRegion horizontal_bottom(QRect(layer.border_box.x + layer.border_radius.bottom_left_x,
                                        layer.border_box.bottom() - layer.border_radius.bottom_left_y,
                                        layer.border_box.width - layer.border_radius.bottom_left_x -
                                          layer.border_radius.bottom_right_x,
                                        layer.border_radius.bottom_left_y));
  const QRegion top_left(QRect(layer.border_box.left(),
                               layer.border_box.top(),
                               2 * layer.border_radius.top_left_x,
                               2 * layer.border_radius.top_left_y),
                         QRegion::RegionType::Ellipse);
  const QRegion top_right(QRect(layer.border_box.right() - 2 * layer.border_radius.top_right_x,
                                layer.border_box.top(),
                                2 * layer.border_radius.top_right_x,
                                2 * layer.border_radius.top_right_y),
                          QRegion::RegionType::Ellipse);
  const QRegion bottom_left(QRect(layer.border_box.left(),
                                  layer.border_box.bottom() - 2 * layer.border_radius.bottom_left_y,
                                  2 * layer.border_radius.bottom_left_x,
                                  2 * layer.border_radius.bottom_left_y),
                            QRegion::RegionType::Ellipse);
  const QRegion bottom_right(QRect(layer.border_box.right() - 2 * layer.border_radius.bottom_right_x,
                                   layer.border_box.bottom() - 2 * layer.border_radius.bottom_right_y,
                                   2 * layer.border_radius.bottom_right_x,
                                   2 * layer.border_radius.bottom_right_y),
                             QRegion::RegionType::Ellipse);
  const QRegion clip_region = horizontal_middle.united(horizontal_top)
                                .united(horizontal_bottom)
                                .united(top_left)
                                .united(top_right)
                                .united(bottom_left)
                                .united(bottom_right);

  painter->setClipRegion(clip_region, Qt::ClipOperation::IntersectClip);
  painter->setPen(Qt::PenStyle::NoPen);
  painter->setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);

  drawSelection(painter, toQRect(layer.border_box));

  const QPixmap pixmap = getPixmap(QString::fromStdString(url), QString::fromStdString(base_url));

  if (layer.repeat == litehtml::background_repeat_no_repeat) {
    painter->drawPixmap(QRect(layer.origin_box.x, layer.origin_box.y, layer.origin_box.width, layer.origin_box.height),
                        pixmap);
  }
  else if (layer.repeat == litehtml::background_repeat_repeat_x) {
    if (layer.origin_box.width > 0) {
      int x = layer.border_box.left();

      while (x <= layer.border_box.right()) {
        painter->drawPixmap(QRect(x, layer.border_box.top(), layer.origin_box.width, layer.origin_box.height), pixmap);
        x += layer.origin_box.width;
      }
    }
  }

  painter->restore();
}

void DocumentContainer::draw_borders(litehtml::uint_ptr hdc,
                                     const litehtml::borders& borders,
                                     const litehtml::position& draw_pos,
                                     bool root) {
  Q_UNUSED(root)

  auto painter = toQPainter(hdc, m_shapeAntialiasing);

  if (borders.top.style != litehtml::border_style_none && borders.top.style != litehtml::border_style_hidden) {
    painter->setPen(borderToQPen(borders.top));
    painter->drawLine(draw_pos.left() + borders.radius.top_left_x,
                      draw_pos.top(),
                      draw_pos.right() - borders.radius.top_right_x,
                      draw_pos.top());
    painter->drawArc(draw_pos.left(),
                     draw_pos.top(),
                     2 * borders.radius.top_left_x,
                     2 * borders.radius.top_left_y,
                     90 * 16,
                     90 * 16);
    painter->drawArc(draw_pos.right() - 2 * borders.radius.top_right_x,
                     draw_pos.top(),
                     2 * borders.radius.top_right_x,
                     2 * borders.radius.top_right_y,
                     0,
                     90 * 16);
  }

  if (borders.bottom.style != litehtml::border_style_none && borders.bottom.style != litehtml::border_style_hidden) {
    painter->setPen(borderToQPen(borders.bottom));
    painter->drawLine(draw_pos.left() + borders.radius.bottom_left_x,
                      draw_pos.bottom(),
                      draw_pos.right() - borders.radius.bottom_right_x,
                      draw_pos.bottom());
    painter->drawArc(draw_pos.left(),
                     draw_pos.bottom() - 2 * borders.radius.bottom_left_y,
                     2 * borders.radius.bottom_left_x,
                     2 * borders.radius.bottom_left_y,
                     180 * 16,
                     90 * 16);
    painter->drawArc(draw_pos.right() - 2 * borders.radius.bottom_right_x,
                     draw_pos.bottom() - 2 * borders.radius.bottom_right_y,
                     2 * borders.radius.bottom_right_x,
                     2 * borders.radius.bottom_right_y,
                     270 * 16,
                     90 * 16);
  }

  if (borders.left.style != litehtml::border_style_none && borders.left.style != litehtml::border_style_hidden) {
    painter->setPen(borderToQPen(borders.left));
    painter->drawLine(draw_pos.left(),
                      draw_pos.top() + borders.radius.top_left_y,
                      draw_pos.left(),
                      draw_pos.bottom() - borders.radius.bottom_left_y);
  }

  if (borders.right.style != litehtml::border_style_none && borders.right.style != litehtml::border_style_hidden) {
    painter->setPen(borderToQPen(borders.right));
    painter->drawLine(draw_pos.right(),
                      draw_pos.top() + borders.radius.top_right_y,
                      draw_pos.right(),
                      draw_pos.bottom() - borders.radius.bottom_right_y);
  }
}

void DocumentContainer::set_caption(const char* caption) {
  m_caption = QString::fromUtf8(caption);
}

void DocumentContainer::set_base_url(const char* base_url) {
  m_baseUrl = QString::fromUtf8(base_url);
}

void DocumentContainer::link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) {
  Q_UNUSED(doc)
  Q_UNUSED(el)
}

void DocumentContainer::on_anchor_click(const char* url, const litehtml::element::ptr& el) {
  Q_UNUSED(el)

  if (!m_blockLinks) {
    m_linkCallback(resolveUrl(QString::fromUtf8(url), m_baseUrl));
  }
}

void DocumentContainer::set_cursor(const char* cursor) {
  m_cursorCallback(toQCursor(QString::fromUtf8(cursor)));
}

void DocumentContainer::transform_text(std::string& text, litehtml::text_transform tt) {
  switch (tt) {
    case litehtml::text_transform::text_transform_none:
      break;

    case litehtml::text_transform::text_transform_capitalize: {
      auto str = QString::fromStdString(text);
      QTextBoundaryFinder finder(QTextBoundaryFinder::Word, str);
      auto position = finder.toNextBoundary();

      while (0 <= position) {
        if (finder.boundaryReasons() & QTextBoundaryFinder::BoundaryReason::EndOfItem) {
          str.replace(0, 1, str[0].toUpper());
        }
        else if (finder.boundaryReasons() & QTextBoundaryFinder::BoundaryReason::StartOfItem) {
          str.replace(position, 1, str[position].toUpper());
        }

        position = finder.toNextBoundary();
      }

      text = str.toStdString();
      break;
    }

    case litehtml::text_transform::text_transform_uppercase:
      std::transform(text.begin(), text.end(), text.begin(), ::toupper);
      break;

    case litehtml::text_transform::text_transform_lowercase:
      std::transform(text.begin(), text.end(), text.begin(), ::toupper);
      break;
  }
}

void DocumentContainer::import_css(std::string& text, const std::string& url, std::string& baseurl) {
  const QUrl actual_url = resolveUrl(QString::fromStdString(url), QString::fromStdString(baseurl));
  const QString url_string = actual_url.toString(QUrl::UrlFormattingOption::None);
  const int last_slash = url_string.lastIndexOf('/');

  baseurl = url_string.left(last_slash).toStdString();
  text = QString::fromUtf8(handleExternalResource(RequestType::CssDownload, actual_url).toByteArray()).toStdString();
}

void DocumentContainer::set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) {
  Q_UNUSED(pos)
  Q_UNUSED(bdr_radius)
}

void DocumentContainer::del_clip() {}

std::shared_ptr<litehtml::element> DocumentContainer::create_element(const char* tag_name,
                                                                     const litehtml::string_map& attributes,
                                                                     const std::shared_ptr<litehtml::document>& doc) {
  Q_UNUSED(attributes)
  Q_UNUSED(doc)

  return {};
}

void DocumentContainer::get_media_features(litehtml::media_features& media) const {
  media.type = m_mediaType;
}

void DocumentContainer::get_language(std::string& language, std::string& culture) const {
  Q_UNUSED(language)
  Q_UNUSED(culture)
}

void DocumentContainer::setPaintDevice(QPaintDevice* paint_device) {
  m_paintDevice = paint_device;
}

void DocumentContainer::setScrollPosition(const QPoint& pos) {
  m_scrollPosition = pos;
}

void DocumentContainer::setDocument(const QByteArray& data) {
  clearSelection();

  m_pendingExternalResources.clear();
  m_document =
    litehtml::document::createFromString(data.constData(), this, m_masterCss.toStdString(), m_userCss.toStdString());
  m_timerForPendingExternalResources.start();

  buildIndex();
}

bool DocumentContainer::hasDocument() const {
  return m_document.get();
}

void DocumentContainer::setBaseUrl(const QString& url) {
  set_base_url(url.toUtf8().constData());
}

QString DocumentContainer::baseUrl() const {
  return m_baseUrl;
}

void DocumentContainer::render(int width, int height) {
  m_clientRect = {0, 0, width, height};

  if (!m_document) {
    return;
  }

  m_document->render(width);
  updateSelection();
}

void DocumentContainer::draw(QPainter* painter, QRectF clip) {
  drawSelection(painter, clip);

  const QPoint pos = -m_scrollPosition;
  const litehtml::position clipRect = {litehtml::pixel_t(clip.x()),
                                       litehtml::pixel_t(clip.y()),
                                       litehtml::pixel_t(clip.width()),
                                       litehtml::pixel_t(clip.height())};

  m_document->draw(reinterpret_cast<litehtml::uint_ptr>(painter), pos.x(), pos.y(), &clipRect);
}

int DocumentContainer::documentWidth() const {
  return m_document->width();
}

int DocumentContainer::documentHeight() const {
  return m_document->height();
}

int DocumentContainer::anchorY(const QString& anchor_name) const {
  litehtml::element::ptr element = m_document->root()->select_one(QString("#%1").arg(anchor_name).toStdString());

  if (!element) {
    element = m_document->root()->select_one(QString("[name=%1]").arg(anchor_name).toStdString());
  }

  if (!element) {
    return -1;
  }

  while (element) {
    if (element->get_placement().y > 0) {
      return element->get_placement().y;
    }

    element = element->parent();
  }

  return 0;
}

static litehtml::media_type fromQt(const DocumentContainer::MediaType mt) {
  using MT = DocumentContainer::MediaType;

  switch (mt) {
    case MT::All:
      return litehtml::media_type_all;

    case MT::Screen:
      return litehtml::media_type_screen;

    case MT::Print:
      return litehtml::media_type_print;

    default:
      return litehtml::media_type_unknown;
  }

  Q_UNREACHABLE();
}

void DocumentContainer::setMediaType(MediaType mt) {
  m_mediaType = fromQt(mt);
}

QVector<QRectF> DocumentContainer::mousePressEvent(QPointF document_pos, QPointF viewport_pos, Qt::MouseButton button) {
  if (!m_document || button != Qt::LeftButton) {
    return {};
  }

  QVector<QRectF> redraw_rects;

  if (m_selection.isValid()) {
    redraw_rects.append(m_selection.boundingRect());
  }

  clearSelection();

  m_selection.m_startingPos = document_pos;
  m_selection.m_startElem = selectionElementAtPoint(m_document->root(), document_pos, viewport_pos, m_selection.m_mode);

  litehtml::position::vector redraw_boxes;

  if (m_document
        ->on_lbutton_down(document_pos.x(), document_pos.y(), viewport_pos.x(), viewport_pos.y(), redraw_boxes)) {
    redraw_rects.reserve(redraw_boxes.size());

    for (const litehtml::position& box : redraw_boxes) {
      redraw_rects.append(toQRect(box));
    }
  }

  return redraw_rects;
}

QVector<QRectF> DocumentContainer::mouseMoveEvent(QPointF document_pos, QPointF viewport_pos) {
  if (!m_document) {
    return {};
  }

  QVector<QRectF> redraw_rects;

  if (m_selection.m_isSelecting || (!m_selection.m_startingPos.isNull() &&
                                    (m_selection.m_startingPos - document_pos).manhattanLength() >= kDragDistance &&
                                    m_selection.m_startElem.element)) {
    const Selection::Element element =
      selectionElementAtPoint(m_document->root(), document_pos, viewport_pos, m_selection.m_mode);

    if (element.element) {
      redraw_rects.append(m_selection.boundingRect() /*.adjusted(-1, -1, +1, +1)*/); // redraw old selection area
      m_selection.m_endElem = element;
      updateSelection();
      redraw_rects.append(m_selection.boundingRect());
    }
    m_selection.m_isSelecting = true;
  }

  litehtml::position::vector redraw_boxes;

  if (m_document->on_mouse_over(document_pos.x(), document_pos.y(), viewport_pos.x(), viewport_pos.y(), redraw_boxes)) {
    redraw_rects.reserve(redraw_boxes.size());

    for (const litehtml::position& box : redraw_boxes) {
      redraw_rects.append(toQRect(box));
    }
  }
  return redraw_rects;
}

QVector<QRectF> DocumentContainer::mouseReleaseEvent(QPointF document_pos,
                                                     QPointF viewport_pos,
                                                     Qt::MouseButton button) {
  if (!m_document || button != Qt::MouseButton::LeftButton) {
    return {};
  }

  QVector<QRectF> redraw_rects;

  m_selection.m_isSelecting = false;
  m_selection.m_startingPos = {};

  if (m_selection.isValid()) {
    m_blockLinks = true;
  }
  else {
    clearSelection();
  }

  litehtml::position::vector redraw_boxes;

  if (m_document->on_lbutton_up(document_pos.x(), document_pos.y(), viewport_pos.x(), viewport_pos.y(), redraw_boxes)) {
    redraw_rects.reserve(redraw_boxes.size());

    for (const litehtml::position& box : redraw_boxes) {
      redraw_rects.append(toQRect(box));
    }
  }

  m_blockLinks = false;
  return redraw_rects;
}

QVector<QRectF> DocumentContainer::mouseDoubleClickEvent(QPointF document_pos,
                                                         QPointF viewport_pos,
                                                         Qt::MouseButton button) {
  if (!m_document || button != Qt::MouseButton::LeftButton) {
    return {};
  }

  QVector<QRectF> redraw_rects;

  clearSelection();

  m_selection.m_mode = Selection::Mode::Word;
  const Selection::Element element =
    selectionElementAtPoint(m_document->root(), document_pos, viewport_pos, m_selection.m_mode);

  if (element.element) {
    m_selection.m_startElem = element;
    m_selection.m_endElem = m_selection.m_startElem;
    m_selection.m_isSelecting = true;

    updateSelection();

    if (m_selection.isValid()) {
      redraw_rects.append(m_selection.boundingRect());
    }
  }
  else {
    if (m_selection.isValid()) {
      redraw_rects.append(m_selection.boundingRect());
    }

    clearSelection();
  }

  return redraw_rects;
}

QVector<QRectF> DocumentContainer::leaveEvent() {
  if (!m_document) {
    return {};
  }

  litehtml::position::vector redraw_boxes;

  if (m_document->on_mouse_leave(redraw_boxes)) {
    QVector<QRectF> redraw_rects;

    redraw_rects.reserve(redraw_boxes.size());

    for (const litehtml::position& box : redraw_boxes) {
      redraw_rects.append(toQRect(box));
    }

    return redraw_rects;
  }

  return {};
}

QUrl DocumentContainer::imgLinkAt(QPointF document_pos, QPointF viewport_pos) const {
  if (!m_document) {
    return {};
  }

  const char* href = nullptr;

  deepestChildAtPoint(m_document->root(), document_pos, viewport_pos, [&href](const litehtml::element::ptr& e) {
    if (e && e->tag() == litehtml::_img_) {
      href = e->get_attr("src", "");

      if (href) {
        return true;
      }
    }

    return false; /*continue*/
  });

  if (href) {
    return resolveUrl(QString::fromUtf8(href), m_baseUrl);
  }

  return {};
}

QUrl DocumentContainer::linkAt(QPointF document_pos, QPointF viewport_pos) const {
  if (!m_document) {
    return {};
  }

  const char* href = nullptr;

  deepestChildAtPoint(m_document->root(), document_pos, viewport_pos, [&href](const litehtml::element::ptr& e) {
    const litehtml::element::ptr parent = e->parent();

    if (parent && parent->tag() == litehtml::_a_) {
      href = parent->get_attr("href");

      if (href) {
        return true;
      }
    }

    return false; /*continue*/
  });

  if (href) {
    return resolveUrl(QString::fromUtf8(href), m_baseUrl);
  }

  return {};
}

QString DocumentContainer::caption() const {
  return m_caption;
}

QString DocumentContainer::selectedText() const {
  return m_selection.m_text;
}

void DocumentContainer::findText(const QString& text,
                                 QTextDocument::FindFlags flags,
                                 bool incremental,
                                 bool* wrapped,
                                 bool* success,
                                 QVector<QRectF>* old_selection,
                                 QVector<QRectF>* new_selection) {
  if (success) {
    *success = false;
  }

  if (old_selection) {
    old_selection->clear();
  }

  if (new_selection) {
    new_selection->clear();
  }

  if (!m_document) {
    return;
  }

  const bool backward = flags & QTextDocument::FindBackward;
  int start_index = backward ? -1 : 0;

  if (m_selection.m_startElem.element && m_selection.m_endElem.element) { // selection
    // poor-man's incremental search starts at beginning of selection,
    // non-incremental at end (forward search) or beginning (backward search)
    Selection::Element start;
    Selection::Element end;
    std::tie(start, end) = startAndEnd(m_selection.m_startElem, m_selection.m_endElem);
    Selection::Element search_start;

    if (incremental || backward) {
      if (start.index < 0) { // fully selected
        search_start = {firstLeaf(start.element, nullptr), 0, -1};
      }
      else {
        search_start = start;
      }
    }
    else {
      if (end.index < 0) { // fully selected
        search_start = {nextLeaf(end.element, nullptr), 0, -1};
      }
      else {
        search_start = end;
      }
    }

    const auto find_in_index = m_index.m_elementToIndex.find(search_start.element);

    if (find_in_index == std::end(m_index.m_elementToIndex)) {
      return;
    }

    start_index = find_in_index->second + search_start.index;

    if (backward) {
      --start_index;
    }
  }

  const auto fill_x_pos = [](const Selection::Element& e) {
    std::string ttext;

    e.element->get_text(ttext);

    const QString text = QString::fromStdString(ttext);
    const auto font_ptr = e.element->css().get_font();

    if (!font_ptr) {
      return e;
    }

    const QFont& font = toQFont(font_ptr);
    const QFontMetrics fm(font);

    return Selection::Element{e.element, e.index, fm.size(0, text.left(e.index)).width()};
  };

  QString term = QRegularExpression::escape(text);

  if (flags & QTextDocument::FindFlag::FindWholeWords) {
    term = QString("\\b%1\\b").arg(term);
  }

  const QRegularExpression::PatternOptions pattern_opts = (flags & QTextDocument::FindFlag::FindCaseSensitively)
                                                            ? QRegularExpression::PatternOption::NoPatternOption
                                                            : QRegularExpression::PatternOption::CaseInsensitiveOption;
  const QRegularExpression expression(term, pattern_opts);

  int found_index =
    backward ? m_index.m_text.lastIndexOf(expression, start_index) : m_index.m_text.indexOf(expression, start_index);

  if (found_index < 0) { // wrap
    found_index = backward ? m_index.m_text.lastIndexOf(expression) : m_index.m_text.indexOf(expression);

    if (wrapped && found_index >= 0) {
      *wrapped = true;
    }
  }

  if (found_index >= 0) {
    const Index::Entry start_entry = m_index.findElement(found_index);
    const Index::Entry end_entry = m_index.findElement(found_index + text.size());

    if (!start_entry.second || !end_entry.second) {
      return;
    }

    if (old_selection) {
      *old_selection = m_selection.m_selection;
    }

    clearSelection();

    m_selection.m_startElem = fill_x_pos({start_entry.second, found_index - start_entry.first, -1});
    m_selection.m_endElem = fill_x_pos({end_entry.second, int(found_index + text.size() - end_entry.first), -1});

    updateSelection();

    if (new_selection) {
      *new_selection = m_selection.m_selection;
    }
    if (success) {
      *success = true;
    }

    return;
  }
  return;
}

void DocumentContainer::setDefaultFont(const QFont& font) {
  m_defaultFont = font;
  m_defaultFontFamilyName = m_defaultFont.family().toUtf8();

  // Since font family name and size are read only once, when parsing html,
  // we need to trigger the reparse of this info.
  if (m_document && m_document->root()) {
    m_document->root()->refresh_styles();
    m_document->root()->compute_styles();
  }
}

QFont DocumentContainer::defaultFont() const {
  return m_defaultFont;
}

void DocumentContainer::setFontAntialiasing(bool on) {
  m_fontAntialiasing = on;
}

bool DocumentContainer::fontAntialiasing() const {
  return m_fontAntialiasing;
}

void DocumentContainer::setCursorCallback(const DocumentContainer::CursorCallback& callback) {
  m_cursorCallback = callback;
}

void DocumentContainer::setLinkCallback(const DocumentContainer::LinkCallback& callback) {
  m_linkCallback = callback;
}

void DocumentContainer::setPaletteCallback(const DocumentContainer::PaletteCallback& callback) {
  m_paletteCallback = callback;
}

void DocumentContainer::setClipboardCallback(const DocumentContainer::ClipboardCallback& callback) {
  m_clipboardCallback = callback;
}

static litehtml::element::ptr elementForY(int y, const litehtml::element::ptr& element) {
  if (!element) {
    return {};
  }

  if (element->get_placement().y >= y) {
    return element;
  }

  for (const litehtml::element::ptr& child : element->children()) {
    litehtml::element::ptr result = elementForY(y, child);

    if (result) {
      return result;
    }
  }
  return {};
}

static litehtml::element::ptr elementForY(int y, const litehtml::document::ptr& document) {
  if (!document) {
    return {};
  }

  return elementForY(y, document->root());
}

int DocumentContainer::withFixedElementPosition(int y, const std::function<void()>& action) {
  const litehtml::element::ptr element = elementForY(y, m_document);

  action();

  if (element) {
    return element->get_placement().y;
  }

  return -1;
}

QString DocumentContainer::masterCss() const {
  return m_masterCss;
}

void DocumentContainer::setMasterCss(const QString& master_css) {
  m_masterCss = master_css;
}

QPixmap DocumentContainer::getPixmap(const QString& image_url, const QString& base_url) {
  const QUrl url = resolveUrl(image_url, base_url);
  QVariant data = handleExternalResource(RequestType::ImageDisplay, url);

  if (data.canConvert<QPixmap>()) {
    return data.value<QPixmap>();
  }
  else {
    QByteArray pixmap_data = data.toByteArray();
    QPixmap px;

    if (!px.loadFromData(pixmap_data)) {
      // IOFactory::writeFile("test", pixmap_data);
      qCriticalNN << LOGSEC_HTMLVIEWER << "Failed to decode image loaded from file cache for URL"
                  << QUOTE_W_SPACE_DOT(url.toString());
    }

    return px;
  }
}

QString DocumentContainer::serifFont() const {
  return QSL("Times New Roman");
}

QString DocumentContainer::sansSerifFont() const {
  return QSL("Arial");
}

QString DocumentContainer::monospaceFont() const {
  return QSL("Courier");
}

QUrl DocumentContainer::resolveUrl(const QString& url, const QString& base_url) const {
  // several cases:
  // full url: "https://foo.bar/blah.css"
  // relative path: "foo/bar.css"
  // server relative path: "/foo/bar.css"
  // net path: "//foo.bar/blah.css"
  // fragment only: "#foo-fragment"
  const QUrl qurl = QUrl::fromEncoded(url.toUtf8());

  if (qurl.scheme().isEmpty()) {
    if (url.startsWith('#')) { // leave alone if just a fragment
      return qurl;
    }

    const QUrl page_base_url = QUrl(base_url.isEmpty() ? m_baseUrl : base_url);

    if (url.startsWith("//")) { // net path
      return QUrl(page_base_url.scheme() + ":" + url);
    }

    QUrl server_url = QUrl(page_base_url);

    server_url.setPath("");

    const QString actual_base_url = url.startsWith('/')
                                      ? server_url.toString(QUrl::ComponentFormattingOption::FullyEncoded)
                                      : page_base_url.toString(QUrl::ComponentFormattingOption::FullyEncoded);
    QUrl resolved_url(actual_base_url + '/' + url);

    resolved_url.setPath(resolved_url.path(QUrl::ComponentFormattingOption::FullyEncoded |
                                           QUrl::UrlFormattingOption::NormalizePathSegments),
                         QUrl::ParsingMode::TolerantMode);

    return resolved_url;
  }

  return qurl;
}

void DocumentContainer::downloadNextExternalResource() {
  if (m_pendingExternalResources.isEmpty()) {
    qDebugNN << LOGSEC_HTMLVIEWER << "There are no external resources to download.";
    return;
  }

  auto url = m_pendingExternalResources.first().toString(QUrl::ComponentFormattingOption::FullyEncoded);

  m_downloader->downloadFile(url, 3000);
  qDebugNN << LOGSEC_HTMLVIEWER << "Downloading external resources" << QUOTE_W_SPACE_DOT(url);
}

Index::Entry Index::findElement(int index) const {
  const auto upper = std::upper_bound(std::begin(m_indexToElement),
                                      std::end(m_indexToElement),
                                      Entry{index, {}},
                                      [](const Entry& a, const Entry& b) {
                                        return a.first < b.first;
                                      });

  if (upper == std::begin(m_indexToElement)) { // should not happen for index >= 0
    return {-1, {}};
  }

  return *(upper - 1);
}

void DocumentContainer::draw_linear_gradient(litehtml::uint_ptr hdc,
                                             const litehtml::background_layer& layer,
                                             const litehtml::background_layer::linear_gradient& gradient) {
  QLinearGradient gr;

  gr.setStart(gradient.start.x, gradient.start.y);
  gr.setFinalStop(gradient.end.x, gradient.end.y);

  for (const litehtml::background_layer::color_point& clr_pt : gradient.color_points) {
    gr.setColorAt(clr_pt.offset, toQColor(clr_pt.color));
  }

  drawRectWithLambda(hdc, layer, [&](QPainter* painter) {
    painter->setBrush(gr);
  });
}

void DocumentContainer::draw_radial_gradient(litehtml::uint_ptr hdc,
                                             const litehtml::background_layer& layer,
                                             const litehtml::background_layer::radial_gradient& gradient) {
  QRadialGradient gr;

  gr.setCenter(gradient.position.x, gradient.position.y);
  gr.setRadius(gradient.radius.x);
  gr.setFocalPoint(gradient.position.x, gradient.position.y);

  for (const litehtml::background_layer::color_point& clr_pt : gradient.color_points) {
    gr.setColorAt(clr_pt.offset, toQColor(clr_pt.color));
  }

  drawRectWithLambda(hdc, layer, [&](QPainter* painter) {
    painter->setBrush(gr);
  });
}

void DocumentContainer::draw_conic_gradient(litehtml::uint_ptr hdc,
                                            const litehtml::background_layer& layer,
                                            const litehtml::background_layer::conic_gradient& gradient) {
  QConicalGradient gr(gradient.position.x, gradient.position.y, (-1 * gradient.angle) + 90);

  for (const litehtml::background_layer::color_point& clr_pt : gradient.color_points) {
    gr.setColorAt(1.0 - clr_pt.offset, toQColor(clr_pt.color));
  }

  drawRectWithLambda(hdc, layer, [&](QPainter* painter) {
    painter->setBrush(gr);
  });
}

void DocumentContainer::on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event) {}

void DocumentContainer::get_viewport(litehtml::position& viewport) const {
  viewport = {litehtml::pixel_t(m_clientRect.x()),
              litehtml::pixel_t(m_clientRect.y()),
              litehtml::pixel_t(m_clientRect.width()),
              litehtml::pixel_t(m_clientRect.height())};
}
