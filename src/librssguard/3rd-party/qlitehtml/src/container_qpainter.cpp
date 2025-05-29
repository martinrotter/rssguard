// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "container_qpainter.h"
#include "container_qpainter_p.h"

#if QT_CONFIG(clipboard)
#include <QClipboard>
#endif
#include <QCursor>
#include <QDebug>
#include <QDir>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QPainter>
#include <QPalette>
#include <QRegularExpression>
#include <QScreen>
#include <QTextLayout>
#include <QUrl>

#include <algorithm>

const int kDragDistance = 5;

using Font = QFont;
using Context = QPainter;

#ifdef Q_STATIC_LOGGING_CATEGORY
Q_STATIC_LOGGING_CATEGORY(log, "qlitehtml", QtCriticalMsg)
#else
static Q_LOGGING_CATEGORY(log, "qlitehtml", QtCriticalMsg)
#endif

static QFont toQFont(litehtml::uint_ptr hFont)
{
    return *reinterpret_cast<Font *>(hFont);
}

static QPainter *toQPainter(litehtml::uint_ptr hdc)
{
    return reinterpret_cast<Context *>(hdc);
}

static QRect toQRect(litehtml::position position)
{
    return {position.x, position.y, position.width, position.height};
}

static bool isVisible(const litehtml::element::ptr &element)
{
    // TODO render_item::is_visible() would also take m_skip into account, so this might be wrong
    return element->css().get_display() != litehtml::display_none
           && element->css().get_visibility() == litehtml::visibility_visible;
}

static litehtml::elements_list path(const litehtml::element::ptr &element)
{
    litehtml::elements_list result;
    litehtml::element::ptr current = element;
    while (current) {
        result.push_back(current);
        current = current->parent();
    }
    std::reverse(std::begin(result), std::end(result));
    return result;
}

// <parent, first_different_child_a, first_different_child_b>
static std::tuple<litehtml::element::ptr, litehtml::element::ptr, litehtml::element::ptr>
getCommonParent(const litehtml::elements_list &a, const litehtml::elements_list &b)
{
    litehtml::element::ptr parent;
    auto ait = a.cbegin();
    auto bit = b.cbegin();
    while (ait != a.cend() && bit != b.cend()) {
        if (*ait != *bit)
            return {parent, *ait, *bit};
        parent = *ait;
        ++ait;
        ++bit;
    }
    return {parent,
            (ait != a.cend() ? *ait : litehtml::element::ptr()),
            (bit != b.cend() ? *bit : litehtml::element::ptr())};
}

static std::pair<Selection::Element, Selection::Element> getStartAndEnd(const Selection::Element &a,
                                                                        const Selection::Element &b)
{
    if (a.element == b.element) {
        if (a.index <= b.index)
            return {a, b};
        return {b, a};
    }
    const litehtml::elements_list aPath = path(a.element);
    const litehtml::elements_list bPath = path(b.element);
    litehtml::element::ptr commonParent;
    litehtml::element::ptr aBranch;
    litehtml::element::ptr bBranch;
    std::tie(commonParent, aBranch, bBranch) = getCommonParent(aPath, bPath);
    if (!commonParent) {
        qWarning() << "internal error: litehtml elements do not have common parent";
        return {a, b};
    }
    if (commonParent == a.element)
        return {a, a}; // 'a' already contains 'b'
    if (commonParent == b.element)
        return {b, b};
    // find out if a or b is first in the child sub-trees of commonParent
    for (const litehtml::element::ptr &child : commonParent->children()) {
        if (child == aBranch)
            return {a, b};
        if (child == bBranch)
            return {b, a};
    }
    qWarning() << "internal error: failed to find out order of litehtml elements";
    return {a, b};
}

// 1) stops right away if element == stop, otherwise stops whenever stop element is encountered
// 2) moves down the first children from element until there is none anymore
static litehtml::element::ptr firstLeaf(const litehtml::element::ptr &element,
                                        const litehtml::element::ptr &stop)
{
    if (element == stop)
        return element;
    litehtml::element::ptr current = element;
    while (current != stop && !current->children().empty())
        current = current->children().front();
    return current;
}

// 1) stops right away if element == stop, otherwise stops whenever stop element is encountered
// 2) starts at next sibling (up the hierarchy chain) if possible, otherwise root
// 3) returns first leaf of the element found in 2
static litehtml::element::ptr nextLeaf(const litehtml::element::ptr &element,
                                       const litehtml::element::ptr &stop)
{
    if (element == stop)
        return element;
    litehtml::element::ptr current = element;
    if (!current->is_root()) {
        // find next sibling
        const litehtml::element::ptr parent = current->parent();
        const litehtml::elements_list &children = parent->children();
        auto childIt = std::find_if(children.cbegin(),
                                    children.cend(),
                                    [&current](const litehtml::element::ptr &e) {
                                        return e == current;
                                    });
        if (childIt == children.cend()) {
            qWarning() << "internal error: filed to find litehtml child element in parent";
            return stop;
        }
        ++childIt;
        if (childIt == children.cend()) // no sibling, move up
            return nextLeaf(parent, stop);
        current = *childIt;
    }
    return firstLeaf(current, stop);
}

static Selection::Element selectionDetails(const litehtml::element::ptr &element,
                                           const QString &text,
                                           const QPoint &pos)
{
    // shortcut, which _might_ not really be correct
    if (!element->children().empty())
        return {element, -1, -1}; // everything selected
    const QFont &font = toQFont(element->css().get_font());
    const QFontMetrics fm(font);
    int previous = 0;
    for (int i = 0; i < text.size(); ++i) {
        const int width = fm.size(0, text.left(i + 1)).width();
        if ((width + previous) / 2 >= pos.x())
            return {element, i, previous};
        previous = width;
    }
    return {element, int(text.size()), previous};
}

// Returns whether the intended child was found and stop.
// Does a depth-first iteration over elements that "pos" is inside, and executes
// \a action with them. If \a action returns \c true, the iteration is stopped.
static bool deepest_child_at_point(const litehtml::element::ptr &element,
                                   const QPoint &pos,
                                   const QPoint &viewportPos,
                                   const std::function<bool(const litehtml::element::ptr &)> &action,
                                   int level = 0)
{
    // TODO are there elements for which we should take viewportPos into account instead?
    // E.g. fixed position elements?
    if (!element)
        return false /*continue iterating*/;
    const QRect placement = toQRect(element->get_placement());
    // Do not continue down elements that do not cover the position. Exceptions:
    // - elements with 0 size (includes anchors and other weird elements)
    // - html and body, which for some reason have size == viewport size
    if (!placement.size().isEmpty() && element->tag() != litehtml::_html_
        && element->tag() != litehtml::_body_ && !placement.contains(pos)) {
        return false /*continue iterating*/;
    }
    // qDebug() << qPrintable(QString(level * 2, ' ')) << element->dump_get_name() << placement << pos;

    const litehtml::elements_list &children = element->children();
    for (auto it = children.cbegin(); it != children.cend(); ++it) {
        if (deepest_child_at_point(*it, pos, viewportPos, action, level + 1))
            return true;
    }
    if (placement.contains(pos))
        return action(element);
    return false /*continue iterating*/;
}

static Selection::Element selection_element_at_point(const litehtml::element::ptr &element,
                                                     const QPoint &pos,
                                                     const QPoint &viewportPos,
                                                     Selection::Mode mode)
{
    Selection::Element result;
    deepest_child_at_point(element,
                           pos,
                           viewportPos,
                           [mode, &result, &pos](const litehtml::element::ptr &element) {
                               const QRect placement = toQRect(element->get_placement());
                               std::string text;
                               element->get_text(text);
                               if (!text.empty()) {
                                   result = mode == Selection::Mode::Free
                                                ? selectionDetails(element,
                                                                   QString::fromStdString(text),
                                                                   pos - placement.topLeft())
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
static QFont::Weight cssWeightToQtWeight(int cssWeight)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QFont::Weight(cssWeight);
#else
    if (cssWeight <= 400)
        return QFont::Weight(cssWeight * 50 / 400);
    if (cssWeight >= 700)
        return QFont::Weight(75 + (cssWeight - 700) * 25 / 300);
    return QFont::Weight(50 + (cssWeight - 400) * 25 / 300);
#endif
}

static QFont::Style toQFontStyle(litehtml::font_style style)
{
    switch (style) {
    case litehtml::font_style_normal:
        return QFont::StyleNormal;
    case litehtml::font_style_italic:
        return QFont::StyleItalic;
    }
    // should not happen
    qWarning(log) << "Unknown litehtml font style:" << style;
    return QFont::StyleNormal;
}

static QColor toQColor(const litehtml::web_color &color)
{
    return {color.red, color.green, color.blue, color.alpha};
}

static Qt::PenStyle borderPenStyle(litehtml::border_style style)
{
    switch (style) {
    case litehtml::border_style_dotted:
        return Qt::DotLine;
    case litehtml::border_style_dashed:
        return Qt::DashLine;
    case litehtml::border_style_solid:
        return Qt::SolidLine;
    default:
        qWarning(log) << "Unsupported border style:" << style;
    }
    return Qt::SolidLine;
}

static QPen borderPen(const litehtml::border &border)
{
    return {toQColor(border.color), qreal(border.width), borderPenStyle(border.style)};
}

static QCursor toQCursor(const QString &c)
{
    if (c == "alias")
        return {Qt::PointingHandCursor}; // ???
    if (c == "all-scroll")
        return {Qt::SizeAllCursor};
    if (c == "auto")
        return {Qt::ArrowCursor}; // ???
    if (c == "cell")
        return {Qt::UpArrowCursor};
    if (c == "context-menu")
        return {Qt::ArrowCursor}; // ???
    if (c == "col-resize")
        return {Qt::SplitHCursor};
    if (c == "copy")
        return {Qt::DragCopyCursor};
    if (c == "crosshair")
        return {Qt::CrossCursor};
    if (c == "default")
        return {Qt::ArrowCursor};
    if (c == "e-resize")
        return {Qt::SizeHorCursor}; // ???
    if (c == "ew-resize")
        return {Qt::SizeHorCursor};
    if (c == "grab")
        return {Qt::OpenHandCursor};
    if (c == "grabbing")
        return {Qt::ClosedHandCursor};
    if (c == "help")
        return {Qt::WhatsThisCursor};
    if (c == "move")
        return {Qt::SizeAllCursor};
    if (c == "n-resize")
        return {Qt::SizeVerCursor}; // ???
    if (c == "ne-resize")
        return {Qt::SizeBDiagCursor}; // ???
    if (c == "nesw-resize")
        return {Qt::SizeBDiagCursor};
    if (c == "ns-resize")
        return {Qt::SizeVerCursor};
    if (c == "nw-resize")
        return {Qt::SizeFDiagCursor}; // ???
    if (c == "nwse-resize")
        return {Qt::SizeFDiagCursor};
    if (c == "no-drop")
        return {Qt::ForbiddenCursor};
    if (c == "none")
        return {Qt::BlankCursor};
    if (c == "not-allowed")
        return {Qt::ForbiddenCursor};
    if (c == "pointer")
        return {Qt::PointingHandCursor};
    if (c == "progress")
        return {Qt::BusyCursor};
    if (c == "row-resize")
        return {Qt::SplitVCursor};
    if (c == "s-resize")
        return {Qt::SizeVerCursor}; // ???
    if (c == "se-resize")
        return {Qt::SizeFDiagCursor}; // ???
    if (c == "sw-resize")
        return {Qt::SizeBDiagCursor}; // ???
    if (c == "text")
        return {Qt::IBeamCursor};
    if (c == "url")
        return {Qt::ArrowCursor}; // ???
    if (c == "w-resize")
        return {Qt::SizeHorCursor}; // ???
    if (c == "wait")
        return {Qt::BusyCursor};
    if (c == "zoom-in")
        return {Qt::ArrowCursor}; // ???
    qWarning(log) << QString("unknown cursor property \"%1\"").arg(c).toUtf8().constData();
    return {Qt::ArrowCursor};
}

bool Selection::isValid() const
{
    return !selection.isEmpty();
}

void Selection::update()
{
    const auto addElement = [this](const Selection::Element &element,
                                   const Selection::Element &end = {}) {
        std::string elemText;
        element.element->get_text(elemText);
        const QString textStr = QString::fromStdString(elemText);
        if (!textStr.isEmpty()) {
            QRect rect = toQRect(element.element->get_placement()).adjusted(-1, -1, 1, 1);
            if (element.index < 0) { // fully selected
                text += textStr;
            } else if (end.element) { // select from element "to end"
                if (element.element == end.element) {
                    // end.index is guaranteed to be >= element.index by caller, same for x
                    text += textStr.mid(element.index, end.index - element.index);
                    const int left = rect.left();
                    rect.setLeft(left + element.x);
                    rect.setRight(left + end.x);
                } else {
                    text += textStr.mid(element.index);
                    rect.setLeft(rect.left() + element.x);
                }
            } else { // select from start of element
                text += textStr.left(element.index);
                rect.setRight(rect.left() + element.x);
            }
            selection.append(rect);
        }
    };

    if (startElem.element && endElem.element) {
        // Edge cases:
        // start and end elements could be reversed or children of each other
        Selection::Element start;
        Selection::Element end;
        std::tie(start, end) = getStartAndEnd(startElem, endElem);

        selection.clear();
        text.clear();

        // Treats start element as a leaf even if it isn't, because it already contains all its
        // children
        addElement(start, end);
        if (start.element != end.element) {
            litehtml::element::ptr current = start.element;
            do {
                current = nextLeaf(current, end.element);
                if (current == end.element)
                    addElement(end);
                else
                    addElement({current, -1, -1});
            } while (current != end.element);
        }
    } else {
        selection = {};
        text.clear();
    }
#if QT_CONFIG(clipboard)
    QClipboard *cb = QGuiApplication::clipboard();
    if (cb->supportsSelection())
        cb->setText(text, QClipboard::Selection);
#endif
}

QRect Selection::boundingRect() const
{
    QRect rect;
    for (const QRect &r : selection)
        rect = rect.united(r);
    return rect;
}

DocumentContainer::DocumentContainer()
    : d(new DocumentContainerPrivate)
{}

DocumentContainer::~DocumentContainer() = default;

litehtml::uint_ptr DocumentContainerPrivate::create_font(const char *faceName,
                                                         int size,
                                                         int weight,
                                                         litehtml::font_style italic,
                                                         unsigned int decoration,
                                                         litehtml::font_metrics *fm)
{
    const QStringList splitNames = QString::fromUtf8(faceName).split(',', Qt::SkipEmptyParts);
    QStringList familyNames;
    std::transform(splitNames.cbegin(),
                   splitNames.cend(),
                   std::back_inserter(familyNames),
                   [this](const QString &s) {
                       // clean whitespace and quotes
                       QString name = s.trimmed();
                       if (name.startsWith('\"'))
                           name = name.mid(1);
                       if (name.endsWith('\"'))
                           name.chop(1);
                       const QString lowerName = name.toLower();
                       if (lowerName == "serif")
                           return serifFont();
                       if (lowerName == "sans-serif")
                           return sansSerifFont();
                       if (lowerName == "monospace")
                           return monospaceFont();
                       return name;
                   });
    auto font = new QFont();
    font->setFamilies(familyNames);
    font->setPixelSize(size);
    font->setWeight(cssWeightToQtWeight(weight));
    font->setStyle(toQFontStyle(italic));
    font->setStyleStrategy(m_antialias ? QFont::PreferAntialias : QFont::NoAntialias);
    if (decoration == litehtml::font_decoration_underline)
        font->setUnderline(true);
    if (decoration == litehtml::font_decoration_overline)
        font->setOverline(true);
    if (decoration == litehtml::font_decoration_linethrough)
        font->setStrikeOut(true);
    if (fm) {
        const QFontMetrics metrics(*font);
        fm->height = metrics.height();
        fm->ascent = metrics.ascent();
        fm->descent = metrics.descent();
        fm->x_height = metrics.xHeight();
        fm->draw_spaces = true;
    }
    return reinterpret_cast<litehtml::uint_ptr>(font);
}

void DocumentContainerPrivate::delete_font(litehtml::uint_ptr hFont)
{
    auto font = reinterpret_cast<Font *>(hFont);
    delete font;
}

int DocumentContainerPrivate::text_width(const char *text, litehtml::uint_ptr hFont)
{
    const QFontMetrics fm(toQFont(hFont));
    return fm.horizontalAdvance(QString::fromUtf8(text));
}

void DocumentContainerPrivate::draw_text(litehtml::uint_ptr hdc,
                                         const char *text,
                                         litehtml::uint_ptr hFont,
                                         litehtml::web_color color,
                                         const litehtml::position &pos)
{
    auto painter = toQPainter(hdc);
    painter->setFont(toQFont(hFont));
    painter->setPen(toQColor(color));
    painter->drawText(toQRect(pos), 0, QString::fromUtf8(text));
}

int DocumentContainerPrivate::pt_to_px(int pt) const
{
    // magic factor of 11/12 to account for differences to webengine/webkit
    return m_paintDevice->physicalDpiY() * pt * 11 / m_paintDevice->logicalDpiY() / 12;
}

int DocumentContainerPrivate::get_default_font_size() const
{
    return m_defaultFont.pointSize();
}

const char *DocumentContainerPrivate::get_default_font_name() const
{
    return m_defaultFontFamilyName.constData();
}

void DocumentContainerPrivate::draw_list_marker(litehtml::uint_ptr hdc,
                                                const litehtml::list_marker &marker)
{
    auto painter = toQPainter(hdc);
    if (marker.image.empty()) {
        if (marker.marker_type == litehtml::list_style_type_square) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(toQColor(marker.color));
            painter->drawRect(toQRect(marker.pos));
        } else if (marker.marker_type == litehtml::list_style_type_disc) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(toQColor(marker.color));
            painter->drawEllipse(toQRect(marker.pos));
        } else if (marker.marker_type == litehtml::list_style_type_circle) {
            painter->setPen(toQColor(marker.color));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(toQRect(marker.pos));
        } else {
            // TODO we do not get information about index and font for e.g. decimal / roman
            // at least draw a bullet
            painter->setPen(Qt::NoPen);
            painter->setBrush(toQColor(marker.color));
            painter->drawEllipse(toQRect(marker.pos));
            qWarning(log) << "list marker of type" << marker.marker_type << "not supported";
        }
    } else {
        const QPixmap pixmap = getPixmap(QString::fromStdString(marker.image),
                                         QString::fromStdString(marker.baseurl));
        painter->drawPixmap(toQRect(marker.pos), pixmap);
    }
}

void DocumentContainerPrivate::load_image(const char *src, const char *baseurl, bool redraw_on_ready)
{
    const auto qtSrc = QString::fromUtf8(src);
    const auto qtBaseUrl = QString::fromUtf8(baseurl);
    Q_UNUSED(redraw_on_ready)
    qDebug(log) << "load_image:" << QString("src = \"%1\";").arg(qtSrc).toUtf8().constData()
                << QString("base = \"%1\"").arg(qtBaseUrl).toUtf8().constData();
    const QUrl url = resolveUrl(qtSrc, qtBaseUrl);
    if (m_pixmaps.contains(url))
        return;

    QPixmap pixmap;
    pixmap.loadFromData(m_dataCallback(url));
    m_pixmaps.insert(url, pixmap);
}

void DocumentContainerPrivate::get_image_size(const char *src,
                                              const char *baseurl,
                                              litehtml::size &sz)
{
    const auto qtSrc = QString::fromUtf8(src);
    const auto qtBaseUrl = QString::fromUtf8(baseurl);
    if (qtSrc.isEmpty()) // for some reason that happens
        return;
    qDebug(log) << "get_image_size:" << QString("src = \"%1\";").arg(qtSrc).toUtf8().constData()
                << QString("base = \"%1\"").arg(qtBaseUrl).toUtf8().constData();
    const QPixmap pm = getPixmap(qtSrc, qtBaseUrl);
    sz.width = pm.width();
    sz.height = pm.height();
}

void DocumentContainerPrivate::drawSelection(QPainter *painter, const QRect &clip) const
{
    painter->save();
    painter->setClipRect(clip, Qt::IntersectClip);
    for (const QRect &r : m_selection.selection) {
        const QRect clientRect = r.translated(-m_scrollPosition);
        const QPalette palette = m_paletteCallback();
        painter->fillRect(clientRect, palette.brush(QPalette::Highlight));
    }
    painter->restore();
}

static bool isInBody(const litehtml::element::ptr &e)
{
    litehtml::element::ptr current = e;
    while (current && QString::fromUtf8(current->get_tagName()).toLower() != "body")
        current = current->parent();
    return (bool)current;
}

void DocumentContainerPrivate::buildIndex()
{
    m_index.elementToIndex.clear();
    m_index.indexToElement.clear();
    m_index.text.clear();

    int index = 0;
    bool inBody = false;
    litehtml::element::ptr current = firstLeaf(m_document->root(), nullptr);
    while (current != m_document->root()) {
        m_index.elementToIndex.insert({current, index});
        if (!inBody)
            inBody = isInBody(current);
        if (inBody && isVisible(current)) {
            std::string text;
            current->get_text(text);
            if (!text.empty()) {
                m_index.indexToElement.push_back({index, current});
                const QString str = QString::fromStdString(text);
                m_index.text += str;
                index += str.size();
            }
        }
        current = nextLeaf(current, m_document->root());
    }
}

void DocumentContainerPrivate::updateSelection()
{
    const QString oldText = m_selection.text;
    m_selection.update();
    if (!m_clipboardCallback)
        return;

    const QString newText = m_selection.text;
    if (oldText.isEmpty() && !newText.isEmpty())
        m_clipboardCallback(true);
    else if (!oldText.isEmpty() && newText.isEmpty())
        m_clipboardCallback(false);
}

void DocumentContainerPrivate::clearSelection()
{
    const QString oldText = m_selection.text;
    m_selection = {};
    if (!m_clipboardCallback)
        return;

    if (!oldText.isEmpty())
        m_clipboardCallback(false);
}

void DocumentContainerPrivate::draw_background(litehtml::uint_ptr hdc,
                                               const std::vector<litehtml::background_paint> &bgs)
{
    auto painter = toQPainter(hdc);
    const QRegion initialClipRegion = painter->clipRegion();
    const Qt::ClipOperation initialClipOperation
        = initialClipRegion.isEmpty() ? Qt::ReplaceClip : Qt::IntersectClip;
    painter->save();
    for (const litehtml::background_paint &bg : bgs) {
        if (bg.is_root) {
            // TODO ?
            break;
        }
        if (!initialClipRegion.isEmpty())
            painter->setClipRegion(initialClipRegion);
        painter->setClipRect(toQRect(bg.clip_box), initialClipOperation);
        const QRegion horizontalMiddle(QRect(bg.border_box.x,
                                             bg.border_box.y + bg.border_radius.top_left_y,
                                             bg.border_box.width,
                                             bg.border_box.height - bg.border_radius.top_left_y
                                                 - bg.border_radius.bottom_left_y));
        const QRegion horizontalTop(
            QRect(bg.border_box.x + bg.border_radius.top_left_x,
                  bg.border_box.y,
                  bg.border_box.width - bg.border_radius.top_left_x - bg.border_radius.top_right_x,
                  bg.border_radius.top_left_y));
        const QRegion horizontalBottom(QRect(bg.border_box.x + bg.border_radius.bottom_left_x,
                                             bg.border_box.bottom() - bg.border_radius.bottom_left_y,
                                             bg.border_box.width - bg.border_radius.bottom_left_x
                                                 - bg.border_radius.bottom_right_x,
                                             bg.border_radius.bottom_left_y));
        const QRegion topLeft(QRect(bg.border_box.left(),
                                    bg.border_box.top(),
                                    2 * bg.border_radius.top_left_x,
                                    2 * bg.border_radius.top_left_y),
                              QRegion::Ellipse);
        const QRegion topRight(QRect(bg.border_box.right() - 2 * bg.border_radius.top_right_x,
                                     bg.border_box.top(),
                                     2 * bg.border_radius.top_right_x,
                                     2 * bg.border_radius.top_right_y),
                               QRegion::Ellipse);
        const QRegion bottomLeft(QRect(bg.border_box.left(),
                                       bg.border_box.bottom() - 2 * bg.border_radius.bottom_left_y,
                                       2 * bg.border_radius.bottom_left_x,
                                       2 * bg.border_radius.bottom_left_y),
                                 QRegion::Ellipse);
        const QRegion bottomRight(QRect(bg.border_box.right() - 2 * bg.border_radius.bottom_right_x,
                                        bg.border_box.bottom() - 2 * bg.border_radius.bottom_right_y,
                                        2 * bg.border_radius.bottom_right_x,
                                        2 * bg.border_radius.bottom_right_y),
                                  QRegion::Ellipse);
        const QRegion clipRegion = horizontalMiddle.united(horizontalTop)
                                       .united(horizontalBottom)
                                       .united(topLeft)
                                       .united(topRight)
                                       .united(bottomLeft)
                                       .united(bottomRight);
        painter->setClipRegion(clipRegion, Qt::IntersectClip);
        painter->setPen(Qt::NoPen);
        painter->setBrush(toQColor(bg.color));
        painter->drawRect(bg.border_box.x,
                          bg.border_box.y,
                          bg.border_box.width,
                          bg.border_box.height);
        drawSelection(painter, toQRect(bg.border_box));
        if (!bg.image.empty()) {
            const QPixmap pixmap = getPixmap(QString::fromStdString(bg.image),
                                             QString::fromStdString(bg.baseurl));
            if (bg.repeat == litehtml::background_repeat_no_repeat) {
                painter->drawPixmap(QRect(bg.position_x,
                                          bg.position_y,
                                          bg.image_size.width,
                                          bg.image_size.height),
                                    pixmap);
            } else if (bg.repeat == litehtml::background_repeat_repeat_x) {
                if (bg.image_size.width > 0) {
                    int x = bg.border_box.left();
                    while (x <= bg.border_box.right()) {
                        painter->drawPixmap(QRect(x,
                                                  bg.border_box.top(),
                                                  bg.image_size.width,
                                                  bg.image_size.height),
                                            pixmap);
                        x += bg.image_size.width;
                    }
                }
            } else {
                qWarning(log) << "unsupported background repeat" << bg.repeat;
            }
        }
    }
    painter->restore();
}

void DocumentContainerPrivate::draw_borders(litehtml::uint_ptr hdc,
                                            const litehtml::borders &borders,
                                            const litehtml::position &draw_pos,
                                            bool root)
{
    Q_UNUSED(root)
    // TODO: special border styles
    auto painter = toQPainter(hdc);
    if (borders.top.style != litehtml::border_style_none
        && borders.top.style != litehtml::border_style_hidden) {
        painter->setPen(borderPen(borders.top));
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
    if (borders.bottom.style != litehtml::border_style_none
        && borders.bottom.style != litehtml::border_style_hidden) {
        painter->setPen(borderPen(borders.bottom));
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
    if (borders.left.style != litehtml::border_style_none
        && borders.left.style != litehtml::border_style_hidden) {
        painter->setPen(borderPen(borders.left));
        painter->drawLine(draw_pos.left(),
                          draw_pos.top() + borders.radius.top_left_y,
                          draw_pos.left(),
                          draw_pos.bottom() - borders.radius.bottom_left_y);
    }
    if (borders.right.style != litehtml::border_style_none
        && borders.right.style != litehtml::border_style_hidden) {
        painter->setPen(borderPen(borders.right));
        painter->drawLine(draw_pos.right(),
                          draw_pos.top() + borders.radius.top_right_y,
                          draw_pos.right(),
                          draw_pos.bottom() - borders.radius.bottom_right_y);
    }
}

void DocumentContainerPrivate::set_caption(const char *caption)
{
    m_caption = QString::fromUtf8(caption);
}

void DocumentContainerPrivate::set_base_url(const char *base_url)
{
    m_baseUrl = QString::fromUtf8(base_url);
}

void DocumentContainerPrivate::link(const std::shared_ptr<litehtml::document> &doc,
                                    const litehtml::element::ptr &el)
{
    // TODO
    qDebug(log) << "link";
    Q_UNUSED(doc)
    Q_UNUSED(el)
}

void DocumentContainerPrivate::on_anchor_click(const char *url, const litehtml::element::ptr &el)
{
    Q_UNUSED(el)
    if (!m_blockLinks)
        m_linkCallback(resolveUrl(QString::fromUtf8(url), m_baseUrl));
}

void DocumentContainerPrivate::set_cursor(const char *cursor)
{
    m_cursorCallback(toQCursor(QString::fromUtf8(cursor)));
}

void DocumentContainerPrivate::transform_text(std::string &text, litehtml::text_transform tt)
{
    // TODO
    qDebug(log) << "transform_text";
    Q_UNUSED(text)
    Q_UNUSED(tt)
}

void DocumentContainerPrivate::import_css(std::string &text,
                                          const std::string &url,
                                          std::string &baseurl)
{
    const QUrl actualUrl = resolveUrl(QString::fromStdString(url), QString::fromStdString(baseurl));
    const QString urlString = actualUrl.toString(QUrl::None);
    const int lastSlash = urlString.lastIndexOf('/');
    baseurl = urlString.left(lastSlash).toStdString();
    text = QString::fromUtf8(m_dataCallback(actualUrl)).toStdString();
}

void DocumentContainerPrivate::set_clip(const litehtml::position &pos,
                                        const litehtml::border_radiuses &bdr_radius)
{
    // TODO
    qDebug(log) << "set_clip";
    Q_UNUSED(pos)
    Q_UNUSED(bdr_radius)
}

void DocumentContainerPrivate::del_clip()
{
    // TODO
    qDebug(log) << "del_clip";
}

void DocumentContainerPrivate::get_client_rect(litehtml::position &client) const
{
    client = {m_clientRect.x(), m_clientRect.y(), m_clientRect.width(), m_clientRect.height()};
}

std::shared_ptr<litehtml::element> DocumentContainerPrivate::create_element(
    const char *tag_name,
    const litehtml::string_map &attributes,
    const std::shared_ptr<litehtml::document> &doc)
{
    // TODO
    qDebug(log) << "create_element" << QString::fromUtf8(tag_name);
    Q_UNUSED(attributes)
    Q_UNUSED(doc)
    return {};
}

void DocumentContainerPrivate::get_media_features(litehtml::media_features &media) const
{
    media.type = mediaType;
    qDebug(log) << "get_media_features";
}

void DocumentContainerPrivate::get_language(std::string &language, std::string &culture) const
{
    // TODO
    qDebug(log) << "get_language";
    Q_UNUSED(language)
    Q_UNUSED(culture)
}

void DocumentContainer::setPaintDevice(QPaintDevice *paintDevice)
{
    d->m_paintDevice = paintDevice;
}

void DocumentContainer::setScrollPosition(const QPoint &pos)
{
    d->m_scrollPosition = pos;
}

void DocumentContainer::setDocument(const QByteArray &data, DocumentContainerContext *context)
{
    d->m_pixmaps.clear();
    d->clearSelection();
    d->m_document = litehtml::document::createFromString(data.constData(),
                                                         d.get(),
                                                         context->d->masterCss.toUtf8().constData());
    d->buildIndex();
}

bool DocumentContainer::hasDocument() const
{
    return d->m_document.get();
}

void DocumentContainer::setBaseUrl(const QString &url)
{
    d->set_base_url(url.toUtf8().constData());
}

QString DocumentContainer::baseUrl() const
{
    return d->m_baseUrl;
}

void DocumentContainer::render(int width, int height)
{
    d->m_clientRect = {0, 0, width, height};
    if (!d->m_document)
        return;
    d->m_document->render(width);
    d->updateSelection();
}

void DocumentContainer::draw(QPainter *painter, const QRect &clip)
{
    d->drawSelection(painter, clip);
    const QPoint pos = -d->m_scrollPosition;
    const litehtml::position clipRect = {clip.x(), clip.y(), clip.width(), clip.height()};
    d->m_document->draw(reinterpret_cast<litehtml::uint_ptr>(painter), pos.x(), pos.y(), &clipRect);
}

int DocumentContainer::documentWidth() const
{
    return d->m_document->width();
}

int DocumentContainer::documentHeight() const
{
    return d->m_document->height();
}

int DocumentContainer::anchorY(const QString &anchorName) const
{
    litehtml::element::ptr element = d->m_document->root()->select_one(
        QString("#%1").arg(anchorName).toStdString());
    if (!element) {
        element = d->m_document->root()->select_one(QString("[name=%1]").arg(anchorName).toStdString());
    }
    if (!element)
        return -1;
    while (element) {
        if (element->get_placement().y > 0)
            return element->get_placement().y;
        element = element->parent();
    }
    return 0;
}

static litehtml::media_type fromQt(const DocumentContainer::MediaType mt)
{
    using MT = DocumentContainer::MediaType;
    switch (mt)
    {
    case MT::None:
        return litehtml::media_type_none;
    case MT::All:
        return litehtml::media_type_all;
    case MT::Screen:
        return litehtml::media_type_screen;
    case MT::Print:
        return litehtml::media_type_print;
    case MT::Braille:
        return litehtml::media_type_braille;
    case MT::Embossed:
        return litehtml::media_type_embossed;
    case MT::Handheld:
        return litehtml::media_type_handheld;
    case MT::Projection:
        return litehtml::media_type_projection;
    case MT::Speech:
        return litehtml::media_type_speech;
    case MT::TTY:
        return litehtml::media_type_tty;
    case MT::TV:
        return litehtml::media_type_tv;
    }
    Q_UNREACHABLE();
}

void DocumentContainer::setMediaType(MediaType mt)
{
    d->mediaType = fromQt(mt);
}

QVector<QRect> DocumentContainer::mousePressEvent(const QPoint &documentPos,
                                                  const QPoint &viewportPos,
                                                  Qt::MouseButton button)
{
    if (!d->m_document || button != Qt::LeftButton)
        return {};
    QVector<QRect> redrawRects;
    // selection
    if (d->m_selection.isValid())
        redrawRects.append(d->m_selection.boundingRect());
    d->clearSelection();
    d->m_selection.selectionStartDocumentPos = documentPos;
    d->m_selection.startElem = selection_element_at_point(d->m_document->root(),
                                                          documentPos,
                                                          viewportPos,
                                                          d->m_selection.mode);
    // post to litehtml
    litehtml::position::vector redrawBoxes;
    if (d->m_document->on_lbutton_down(
            documentPos.x(), documentPos.y(), viewportPos.x(), viewportPos.y(), redrawBoxes)) {
        for (const litehtml::position &box : redrawBoxes)
            redrawRects.append(toQRect(box));
    }
    return redrawRects;
}

QVector<QRect> DocumentContainer::mouseMoveEvent(const QPoint &documentPos,
                                                 const QPoint &viewportPos)
{
    if (!d->m_document)
        return {};
    QVector<QRect> redrawRects;
    // selection
    if (d->m_selection.isSelecting
        || (!d->m_selection.selectionStartDocumentPos.isNull()
            && (d->m_selection.selectionStartDocumentPos - documentPos).manhattanLength() >= kDragDistance
            && d->m_selection.startElem.element)) {
        const Selection::Element element = selection_element_at_point(d->m_document->root(),
                                                                      documentPos,
                                                                      viewportPos,
                                                                      d->m_selection.mode);
        if (element.element) {
            redrawRects.append(
                d->m_selection.boundingRect() /*.adjusted(-1, -1, +1, +1)*/); // redraw old selection area
            d->m_selection.endElem = element;
            d->updateSelection();
            redrawRects.append(d->m_selection.boundingRect());
        }
        d->m_selection.isSelecting = true;
    }
    litehtml::position::vector redrawBoxes;
    if (d->m_document->on_mouse_over(
            documentPos.x(), documentPos.y(), viewportPos.x(), viewportPos.y(), redrawBoxes)) {
        for (const litehtml::position &box : redrawBoxes)
            redrawRects.append(toQRect(box));
    }
    return redrawRects;
}

QVector<QRect> DocumentContainer::mouseReleaseEvent(const QPoint &documentPos,
                                                    const QPoint &viewportPos,
                                                    Qt::MouseButton button)
{
    if (!d->m_document || button != Qt::LeftButton)
        return {};
    QVector<QRect> redrawRects;
    // selection
    d->m_selection.isSelecting = false;
    d->m_selection.selectionStartDocumentPos = {};
    if (d->m_selection.isValid())
        d->m_blockLinks = true;
    else
        d->clearSelection();
    litehtml::position::vector redrawBoxes;
    if (d->m_document->on_lbutton_up(
            documentPos.x(), documentPos.y(), viewportPos.x(), viewportPos.y(), redrawBoxes)) {
        for (const litehtml::position &box : redrawBoxes)
            redrawRects.append(toQRect(box));
    }
    d->m_blockLinks = false;
    return redrawRects;
}

QVector<QRect> DocumentContainer::mouseDoubleClickEvent(const QPoint &documentPos,
                                                        const QPoint &viewportPos,
                                                        Qt::MouseButton button)
{
    if (!d->m_document || button != Qt::LeftButton)
        return {};
    QVector<QRect> redrawRects;
    d->clearSelection();
    d->m_selection.mode = Selection::Mode::Word;
    const Selection::Element element = selection_element_at_point(d->m_document->root(),
                                                                  documentPos,
                                                                  viewportPos,
                                                                  d->m_selection.mode);
    if (element.element) {
        d->m_selection.startElem = element;
        d->m_selection.endElem = d->m_selection.startElem;
        d->m_selection.isSelecting = true;
        d->updateSelection();
        if (d->m_selection.isValid())
            redrawRects.append(d->m_selection.boundingRect());
    } else {
        if (d->m_selection.isValid())
            redrawRects.append(d->m_selection.boundingRect());
        d->clearSelection();
    }
    return redrawRects;
}

QVector<QRect> DocumentContainer::leaveEvent()
{
    if (!d->m_document)
        return {};
    litehtml::position::vector redrawBoxes;
    if (d->m_document->on_mouse_leave(redrawBoxes)) {
        QVector<QRect> redrawRects;
        for (const litehtml::position &box : redrawBoxes)
            redrawRects.append(toQRect(box));
        return redrawRects;
    }
    return {};
}

QUrl DocumentContainer::linkAt(const QPoint &documentPos, const QPoint &viewportPos)
{
    if (!d->m_document)
        return {};
    const char *href = nullptr;
    deepest_child_at_point(d->m_document->root(),
                           documentPos,
                           viewportPos,
                           [&href](const litehtml::element::ptr &e) {
                               const litehtml::element::ptr parent = e->parent();
                               if (parent && parent->tag() == litehtml::_a_) {
                                   href = parent->get_attr("href");
                                   if (href)
                                       return true;
                               }
                               return false; /*continue*/
                           });
    if (href)
        return d->resolveUrl(QString::fromUtf8(href), d->m_baseUrl);
    return {};
}

QString DocumentContainer::caption() const
{
    return d->m_caption;
}

QString DocumentContainer::selectedText() const
{
    return d->m_selection.text;
}

void DocumentContainer::findText(const QString &text,
                                 QTextDocument::FindFlags flags,
                                 bool incremental,
                                 bool *wrapped,
                                 bool *success,
                                 QVector<QRect> *oldSelection,
                                 QVector<QRect> *newSelection)
{
    if (success)
        *success = false;
    if (oldSelection)
        oldSelection->clear();
    if (newSelection)
        newSelection->clear();
    if (!d->m_document)
        return;
    const bool backward = flags & QTextDocument::FindBackward;
    int startIndex = backward ? -1 : 0;
    if (d->m_selection.startElem.element && d->m_selection.endElem.element) { // selection
        // poor-man's incremental search starts at beginning of selection,
        // non-incremental at end (forward search) or beginning (backward search)
        Selection::Element start;
        Selection::Element end;
        std::tie(start, end) = getStartAndEnd(d->m_selection.startElem, d->m_selection.endElem);
        Selection::Element searchStart;
        if (incremental || backward) {
            if (start.index < 0) // fully selected
                searchStart = {firstLeaf(start.element, nullptr), 0, -1};
            else
                searchStart = start;
        } else {
            if (end.index < 0) // fully selected
                searchStart = {nextLeaf(end.element, nullptr), 0, -1};
            else
                searchStart = end;
        }
        const auto findInIndex = d->m_index.elementToIndex.find(searchStart.element);
        if (findInIndex == std::end(d->m_index.elementToIndex)) {
            qWarning() << "internal error: cannot find litehmtl element in index";
            return;
        }
        startIndex = findInIndex->second + searchStart.index;
        if (backward)
            --startIndex;
    }

    const auto fillXPos = [](const Selection::Element &e) {
        std::string ttext;
        e.element->get_text(ttext);
        const QString text = QString::fromStdString(ttext);
        const auto fontPtr = e.element->css().get_font();
        if (!fontPtr)
            return e;
        const QFont &font = toQFont(fontPtr);
        const QFontMetrics fm(font);
        return Selection::Element{e.element, e.index, fm.size(0, text.left(e.index)).width()};
    };

    QString term = QRegularExpression::escape(text);
    if (flags & QTextDocument::FindWholeWords)
        term = QString("\\b%1\\b").arg(term);
    const QRegularExpression::PatternOptions patternOptions
        = (flags & QTextDocument::FindCaseSensitively) ? QRegularExpression::NoPatternOption
                                                       : QRegularExpression::CaseInsensitiveOption;
    const QRegularExpression expression(term, patternOptions);

    int foundIndex = backward ? d->m_index.text.lastIndexOf(expression, startIndex)
                              : d->m_index.text.indexOf(expression, startIndex);
    if (foundIndex < 0) { // wrap
        foundIndex = backward ? d->m_index.text.lastIndexOf(expression)
                              : d->m_index.text.indexOf(expression);
        if (wrapped && foundIndex >= 0)
            *wrapped = true;
    }
    if (foundIndex >= 0) {
        const Index::Entry startEntry = d->m_index.findElement(foundIndex);
        const Index::Entry endEntry = d->m_index.findElement(foundIndex + text.size());
        if (!startEntry.second || !endEntry.second) {
            qWarning() << "internal error: search ended up with nullptr elements";
            return;
        }
        if (oldSelection)
            *oldSelection = d->m_selection.selection;
        d->clearSelection();
        d->m_selection.startElem = fillXPos({startEntry.second, foundIndex - startEntry.first, -1});
        d->m_selection.endElem = fillXPos(
            {endEntry.second, int(foundIndex + text.size() - endEntry.first), -1});
        d->updateSelection();
        if (newSelection)
            *newSelection = d->m_selection.selection;
        if (success)
            *success = true;
        return;
    }
    return;
}

void DocumentContainer::setDefaultFont(const QFont &font)
{
    d->m_defaultFont = font;
    d->m_defaultFontFamilyName = d->m_defaultFont.family().toUtf8();
    // Since font family name and size are read only once, when parsing html,
    // we need to trigger the reparse of this info.
    if (d->m_document && d->m_document->root()) {
        d->m_document->root()->refresh_styles();
        d->m_document->root()->compute_styles();
    }
}

QFont DocumentContainer::defaultFont() const
{
    return d->m_defaultFont;
}

void DocumentContainer::setAntialias(bool on)
{
    d->m_antialias = on;
}

bool DocumentContainer::antialias() const
{
    return d->m_antialias;
}

void DocumentContainer::setDataCallback(const DocumentContainer::DataCallback &callback)
{
    d->m_dataCallback = callback;
}

DocumentContainer::DataCallback DocumentContainer::dataCallback() const
{
    return d->m_dataCallback;
}

void DocumentContainer::setCursorCallback(const DocumentContainer::CursorCallback &callback)
{
    d->m_cursorCallback = callback;
}

void DocumentContainer::setLinkCallback(const DocumentContainer::LinkCallback &callback)
{
    d->m_linkCallback = callback;
}

void DocumentContainer::setPaletteCallback(const DocumentContainer::PaletteCallback &callback)
{
    d->m_paletteCallback = callback;
}

DocumentContainer::PaletteCallback DocumentContainer::paletteCallback() const
{
    return d->m_paletteCallback;
}

void DocumentContainer::setClipboardCallback(const DocumentContainer::ClipboardCallback &callback)
{
    d->m_clipboardCallback = callback;
}

static litehtml::element::ptr elementForY(int y, const litehtml::element::ptr &element)
{
    if (!element)
        return {};
    if (element->get_placement().y >= y)
        return element;
    for (const litehtml::element::ptr &child : element->children()) {
        litehtml::element::ptr result = elementForY(y, child);
        if (result)
            return result;
    }
    return {};
}

static litehtml::element::ptr elementForY(int y, const litehtml::document::ptr &document)
{
    if (!document)
        return {};

    return elementForY(y, document->root());
}

int DocumentContainer::withFixedElementPosition(int y, const std::function<void()> &action)
{
    const litehtml::element::ptr element = elementForY(y, d->m_document);
    action();
    if (element)
        return element->get_placement().y;
    return -1;
}

QPixmap DocumentContainerPrivate::getPixmap(const QString &imageUrl, const QString &baseUrl)
{
    const QUrl url = resolveUrl(imageUrl, baseUrl);
    if (!m_pixmaps.contains(url)) {
        qWarning(log) << "draw_background: pixmap not loaded for" << url;
        return {};
    }
    return m_pixmaps.value(url);
}

QString DocumentContainerPrivate::serifFont() const
{
    // TODO make configurable
    return {"Times New Roman"};
}

QString DocumentContainerPrivate::sansSerifFont() const
{
    // TODO make configurable
    return {"Arial"};
}

QString DocumentContainerPrivate::monospaceFont() const
{
    // TODO make configurable
    return {"Courier"};
}

QUrl DocumentContainerPrivate::resolveUrl(const QString &url, const QString &baseUrl) const
{
    // several cases:
    // full url: "https://foo.bar/blah.css"
    // relative path: "foo/bar.css"
    // server relative path: "/foo/bar.css"
    // net path: "//foo.bar/blah.css"
    // fragment only: "#foo-fragment"
    const QUrl qurl = QUrl::fromEncoded(url.toUtf8());
    if (qurl.scheme().isEmpty()) {
        if (url.startsWith('#')) // leave alone if just a fragment
            return qurl;
        const QUrl pageBaseUrl = QUrl(baseUrl.isEmpty() ? m_baseUrl : baseUrl);
        if (url.startsWith("//")) // net path
            return QUrl(pageBaseUrl.scheme() + ":" + url);
        QUrl serverUrl = QUrl(pageBaseUrl);
        serverUrl.setPath("");
        const QString actualBaseUrl = url.startsWith('/')
                                          ? serverUrl.toString(QUrl::FullyEncoded)
                                          : pageBaseUrl.toString(QUrl::FullyEncoded);
        QUrl resolvedUrl(actualBaseUrl + '/' + url);
        resolvedUrl.setPath(resolvedUrl.path(QUrl::FullyEncoded | QUrl::NormalizePathSegments), QUrl::TolerantMode);
        return resolvedUrl;
    }
    return qurl;
}

Index::Entry Index::findElement(int index) const
{
    const auto upper = std::upper_bound(std::begin(indexToElement),
                                        std::end(indexToElement),
                                        Entry{index, {}},
                                        [](const Entry &a, const Entry &b) {
                                            return a.first < b.first;
                                        });
    if (upper == std::begin(indexToElement)) // should not happen for index >= 0
        return {-1, {}};
    return *(upper - 1);
}

DocumentContainerContext::DocumentContainerContext()
    : d(new DocumentContainerContextPrivate)
{}

DocumentContainerContext::~DocumentContainerContext() = default;

void DocumentContainerContext::setMasterStyleSheet(const QString &css)
{
    d->masterCss = css;
}
