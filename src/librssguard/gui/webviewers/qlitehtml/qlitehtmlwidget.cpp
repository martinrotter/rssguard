// For license of this file, see <project-root-folder>/LICENSE.md
// and
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qlitehtmlwidget.h"

#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>

#if QLITEHTML_HAS_QPRINTER
#include <QtPrintSupport/qprinter.h>
#endif

const int kScrollBarStep = 40;

// copied from include/litehtml/master_css.h
const char master_css[] = R"##(
html {
        display: block;
}

head {
        display: none
}

meta {
        display: none
}

title {
        display: none
}

link {
        display: none
}

style {
        display: none
}

script {
        display: none
}

body {
        display:block;
        margin:8px;
}

p {
        display:block;
        margin-top:1em;
        margin-bottom:1em;
}

b, strong {
        display:inline;
        font-weight:bold;
}

i, em, cite {
        display:inline;
        font-style:italic;
}

ins, u {
        text-decoration:underline
}

del, s, strike {
        text-decoration:line-through
}

center
{
        text-align:center;
        display:block;
}

a:link
{
        text-decoration: underline;
        color: #00f;
        cursor: pointer;
}

h1, h2, h3, h4, h5, h6, div {
        display:block;
}

h1 {
        font-weight:bold;
        margin-top:0.67em;
        margin-bottom:0.67em;
        font-size: 2em;
}

h2 {
        font-weight:bold;
        margin-top:0.83em;
        margin-bottom:0.83em;
        font-size: 1.5em;
}

h3 {
        font-weight:bold;
        margin-top:1em;
        margin-bottom:1em;
        font-size:1.17em;
}

h4 {
        font-weight:bold;
        margin-top:1.33em;
        margin-bottom:1.33em
}

h5 {
        font-weight:bold;
        margin-top:1.67em;
        margin-bottom:1.67em;
        font-size:.83em;
}

h6 {
        font-weight:bold;
        margin-top:2.33em;
        margin-bottom:2.33em;
        font-size:.67em;
}

br {
        display:inline-block;
}

br[clear="all"]
{
        clear:both;
}

br[clear="left"]
{
        clear:left;
}

br[clear="right"]
{
        clear:right;
}

span {
        display:inline
}

img {
        display: inline-block;
}

img[align="right"]
{
        float: right;
}

img[align="left"]
{
        float: left;
}

hr {
        display: block;
        margin-top: 0.5em;
        margin-bottom: 0.5em;
        margin-left: auto;
        margin-right: auto;
        border-style: inset;
        border-width: 1px
}


/***************** TABLES ********************/

table {
        display: table;
        border-collapse: separate;
        border-spacing: 2px;
        border-top-color:gray;
        border-left-color:gray;
        border-bottom-color:black;
        border-right-color:black;
        font-size: medium;
        font-weight: normal;
        font-style: normal;
}

tbody, tfoot, thead {
        display:table-row-group;
        vertical-align:middle;
}

tr {
        display: table-row;
        vertical-align: inherit;
        border-color: inherit;
}

td, th {
        display: table-cell;
        vertical-align: inherit;
        border-width:1px;
        padding:1px;
}

th {
        font-weight: bold;
}

table[border] {
        border-style:solid;
}

table[border^="0"] {
        border-style:none;
}

table[border] td, table[border] th {
        border-style:solid;
        border-top-color:black;
        border-left-color:black;
        border-bottom-color:gray;
        border-right-color:gray;
}

table[border^="0"] td, table[border^="0"] th {
        border-style:none;
}

table[align=left] {
   float: left;
}

table[align=right] {
   float: right;
}

table[align=center] {
   margin-left: auto;
   margin-right: auto;
}

caption {
        display: table-caption;
}

td[nowrap], th[nowrap] {
        white-space:nowrap;
}

tt, code, kbd, samp {
        font-family: monospace
}

pre, xmp, plaintext, listing {
        display: block;
        font-family: monospace;
        white-space: pre;
        margin: 1em 0
}

/***************** LISTS ********************/

ul, menu, dir {
        display: block;
        list-style-type: disc;
        margin-top: 1em;
        margin-bottom: 1em;
        margin-left: 0;
        margin-right: 0;
        padding-left: 40px
}

ol {
        display: block;
        list-style-type: decimal;
        margin-top: 1em;
        margin-bottom: 1em;
        margin-left: 0;
        margin-right: 0;
        padding-left: 40px
}

li {
        display: list-item;
}

ul ul, ol ul {
        list-style-type: circle;
}

ol ol ul, ol ul ul, ul ol ul, ul ul ul {
        list-style-type: square;
}

dd {
        display: block;
        margin-left: 40px;
}

dl {
        display: block;
        margin-top: 1em;
        margin-bottom: 1em;
        margin-left: 0;
        margin-right: 0;
}

dt {
        display: block;
}

ol ul, ul ol, ul ul, ol ol {
        margin-top: 0;
        margin-bottom: 0
}

blockquote {
        display: block;
        margin-top: 1em;
        margin-bottom: 1em;
        margin-left: 40px;
        margin-right: 40px;
}

/*********** FORM ELEMENTS ************/

form {
        display: block;
        margin-top: 0em;
}

option {
        display: none;
}

input, textarea, keygen, select, button, isindex {
        margin: 0em;
        color: initial;
        line-height: normal;
        text-transform: none;
        text-indent: 0;
        text-shadow: none;
        display: inline-block;
}
input[type="hidden"] {
        display: none;
}


article, aside, footer, header, hgroup, nav, section
{
        display: block;
}

sub {
        vertical-align: sub;
        font-size: smaller;
}

sup {
        vertical-align: super;
        font-size: smaller;
}

figure {
        display: block;
        margin-top: 1em;
        margin-bottom: 1em;
        margin-left: 40px;
        margin-right: 40px;
}

figcaption {
        display: block;
}

)##";

QLiteHtmlWidget::QLiteHtmlWidget(QWidget* parent) : QAbstractScrollArea(parent) {
  setMouseTracking(true);
  horizontalScrollBar()->setSingleStep(kScrollBarStep);
  verticalScrollBar()->setSingleStep(kScrollBarStep);

  m_documentContainer.setCursorCallback([this](const QCursor& c) {
    viewport()->setCursor(c);
  });

  m_documentContainer.setPaletteCallback([this] {
    return palette();
  });

  m_documentContainer.setLinkCallback([this](const QUrl& url) {
    QUrl fullUrl = url;
    if (url.isRelative() && url.path(QUrl::FullyEncoded).isEmpty()) { // fragment/anchor only
      fullUrl = m_url;
      fullUrl.setFragment(url.fragment(QUrl::FullyEncoded));
    }
    // delay because document may not be changed directly during this callback
    QMetaObject::invokeMethod(
      this,
      [this, fullUrl] {
        emit linkClicked(fullUrl);
      },
      Qt::QueuedConnection);
  });
  m_documentContainer.setClipboardCallback([this](bool yes) {
    emit copyAvailable(yes);
  });

  // TODO adapt mastercss to palette (default text & background color)
  m_context.setMasterStyleSheet(master_css);
}

QLiteHtmlWidget::~QLiteHtmlWidget() {}

void QLiteHtmlWidget::setAntialias(bool on) {
  withFixedTextPosition([this, on] {
    m_documentContainer.setAntialias(on);
    // force litehtml to recreate fonts
    setHtml(m_html);
  });
}

void QLiteHtmlWidget::setUrl(const QUrl& url) {
  m_url = url;
  QUrl baseUrl = url;
  baseUrl.setFragment({});
  const QString path = baseUrl.path(QUrl::FullyEncoded);
  const int lastSlash = path.lastIndexOf('/');
  const QString basePath = lastSlash >= 0 ? path.left(lastSlash) : QString();
  baseUrl.setPath(basePath);
  m_documentContainer.setBaseUrl(baseUrl.toString(QUrl::FullyEncoded));
  QMetaObject::invokeMethod(
    this,
    [this] {
      updateHightlightedLink();
    },
    Qt::QueuedConnection);
}

QUrl QLiteHtmlWidget::url() const {
  return m_url;
}

void QLiteHtmlWidget::setHtml(const QString& content) {
  m_html = content;
  m_documentContainer.setPaintDevice(viewport());
  m_documentContainer.setDocument(content.toUtf8(), &m_context);
  verticalScrollBar()->setValue(0);
  horizontalScrollBar()->setValue(0);
  render();
  QMetaObject::invokeMethod(
    this,
    [this] {
      updateHightlightedLink();
    },
    Qt::QueuedConnection);
}

QString QLiteHtmlWidget::html() const {
  return m_html;
}

QString QLiteHtmlWidget::title() const {
  return m_documentContainer.caption();
}

void QLiteHtmlWidget::setZoomFactor(qreal scale) {
  Q_ASSERT(scale != 0);
  m_zoomFactor = scale;
  withFixedTextPosition([this] {
    render();
  });
}

qreal QLiteHtmlWidget::zoomFactor() const {
  return m_zoomFactor;
}

bool QLiteHtmlWidget::findText(const QString& text, QTextDocument::FindFlags flags, bool incremental, bool* wrapped) {
  bool success = false;
  QVector<QRect> oldSelection;
  QVector<QRect> newSelection;
  m_documentContainer.findText(text, flags, incremental, wrapped, &success, &oldSelection, &newSelection);
  // scroll to search result position and/or redraw as necessary
  QRect newSelectionCombined;
  for (const QRect& r : std::as_const(newSelection))
    newSelectionCombined = newSelectionCombined.united(r);
  QScrollBar* vBar = verticalScrollBar();
  const int top = newSelectionCombined.top();
  const int bottom = newSelectionCombined.bottom() - toVirtual(viewport()->size()).height();
  if (success && top < vBar->value() && vBar->minimum() <= top) {
    vBar->setValue(top);
  }
  else if (success && vBar->value() < bottom && bottom <= vBar->maximum()) {
    vBar->setValue(bottom);
  }
  else {
    viewport()->update(fromVirtual(newSelectionCombined.translated(-scrollPosition())));
    for (const QRect& r : std::as_const(oldSelection))
      viewport()->update(fromVirtual(r.translated(-scrollPosition())));
  }
  return success;
}

void QLiteHtmlWidget::setDefaultFont(const QFont& font) {
  withFixedTextPosition([this, &font] {
    m_documentContainer.setDefaultFont(font);
    render();
  });
}

QFont QLiteHtmlWidget::defaultFont() const {
  return m_documentContainer.defaultFont();
}

void QLiteHtmlWidget::scrollToAnchor(const QString& name) {
  if (!m_documentContainer.hasDocument())
    return;
  horizontalScrollBar()->setValue(0);
  if (name.isEmpty()) {
    verticalScrollBar()->setValue(0);
    return;
  }
  const int y = m_documentContainer.anchorY(name);
  if (y >= 0)
    verticalScrollBar()->setValue(std::min(y, verticalScrollBar()->maximum()));
}

void QLiteHtmlWidget::setResourceHandler(const QLiteHtmlWidget::ResourceHandler& handler) {
  m_documentContainer.setDataCallback(handler);
}

QString QLiteHtmlWidget::selectedText() const {
  return m_documentContainer.selectedText();
}

void QLiteHtmlWidget::print(QPrinter* printer) {
#if QLITEHTML_HAS_QPRINTER
  QPainter painter;
  if (!painter.begin(printer))
    return;

  DocumentContainer dc;
  dc.setDataCallback(m_documentContainer.dataCallback());
  dc.setPaletteCallback(m_documentContainer.paletteCallback());
  dc.setDefaultFont(m_documentContainer.defaultFont());
  dc.setPaintDevice(printer);
  dc.setBaseUrl(m_documentContainer.baseUrl());
  dc.setMediaType(DocumentContainer::MediaType::Print);
  dc.setDocument(m_html.toUtf8(), &m_context);

  const QRect pageRect = printer->pageRect(QPrinter::DevicePixel).toRect();
  dc.render(pageRect.width(), pageRect.height());

  QRect drawRect = pageRect;
  drawRect.moveTo(0, 0);
  painter.setClipping(true);
  painter.setClipRect(drawRect);

  QPoint scrollPosition(0, 0);
  while (true) {
    dc.setScrollPosition(scrollPosition);
    dc.draw(&painter, drawRect);
    scrollPosition.ry() += drawRect.height();
    if (scrollPosition.y() < dc.documentHeight())
      printer->newPage();
    else
      break;
  }

  painter.end();
#else
  Q_UNUSED(printer);
#endif
}

void QLiteHtmlWidget::paintEvent(QPaintEvent* event) {
  if (!m_documentContainer.hasDocument())
    return;
  m_documentContainer.setScrollPosition(scrollPosition());
  QPainter p(viewport());
  p.setWorldTransform(QTransform().scale(m_zoomFactor, m_zoomFactor));
  p.setRenderHint(QPainter::SmoothPixmapTransform, true);
  p.setRenderHint(QPainter::Antialiasing, true);
  m_documentContainer.draw(&p, toVirtual(event->rect()));
}

void QLiteHtmlWidget::resizeEvent(QResizeEvent* event) {
  withFixedTextPosition([this, event] {
    QAbstractScrollArea::resizeEvent(event);
    render();
  });
}

void QLiteHtmlWidget::mouseMoveEvent(QMouseEvent* event) {
  QPoint viewportPos;
  QPoint pos;
  htmlPos(event->pos(), &viewportPos, &pos);
  const QVector<QRect> areas = m_documentContainer.mouseMoveEvent(pos, viewportPos);
  for (const QRect& r : areas)
    viewport()->update(fromVirtual(r.translated(-scrollPosition())));

  updateHightlightedLink();
}

void QLiteHtmlWidget::mousePressEvent(QMouseEvent* event) {
  QPoint viewportPos;
  QPoint pos;
  htmlPos(event->pos(), &viewportPos, &pos);
  const QVector<QRect> areas = m_documentContainer.mousePressEvent(pos, viewportPos, event->button());
  for (const QRect& r : areas)
    viewport()->update(fromVirtual(r.translated(-scrollPosition())));
}

void QLiteHtmlWidget::mouseReleaseEvent(QMouseEvent* event) {
  QPoint viewportPos;
  QPoint pos;
  htmlPos(event->pos(), &viewportPos, &pos);
  const QVector<QRect> areas = m_documentContainer.mouseReleaseEvent(pos, viewportPos, event->button());
  for (const QRect& r : areas)
    viewport()->update(fromVirtual(r.translated(-scrollPosition())));
}

void QLiteHtmlWidget::mouseDoubleClickEvent(QMouseEvent* event) {
  QPoint viewportPos;
  QPoint pos;
  htmlPos(event->pos(), &viewportPos, &pos);
  const QVector<QRect> areas = m_documentContainer.mouseDoubleClickEvent(pos, viewportPos, event->button());
  for (const QRect& r : areas) {
    viewport()->update(fromVirtual(r.translated(-scrollPosition())));
  }
}

void QLiteHtmlWidget::leaveEvent(QEvent* event) {
  Q_UNUSED(event)
  const QVector<QRect> areas = m_documentContainer.leaveEvent();
  for (const QRect& r : areas)
    viewport()->update(fromVirtual(r.translated(-scrollPosition())));
  setHightlightedLink(QUrl());
}

void QLiteHtmlWidget::contextMenuEvent(QContextMenuEvent* event) {
  QPoint viewportPos;
  QPoint pos;
  htmlPos(event->pos(), &viewportPos, &pos);
  emit contextMenuRequested(event->pos(), m_documentContainer.linkAt(pos, viewportPos));
}

static QAbstractSlider::SliderAction getSliderAction(int key) {
  if (key == Qt::Key_Home)
    return QAbstractSlider::SliderToMinimum;
  if (key == Qt::Key_End)
    return QAbstractSlider::SliderToMaximum;
  if (key == Qt::Key_PageUp)
    return QAbstractSlider::SliderPageStepSub;
  if (key == Qt::Key_PageDown)
    return QAbstractSlider::SliderPageStepAdd;
  return QAbstractSlider::SliderNoAction;
}

void QLiteHtmlWidget::keyPressEvent(QKeyEvent* event) {
  if (event->modifiers() == Qt::NoModifier || event->modifiers() == Qt::KeypadModifier) {
    const QAbstractSlider::SliderAction sliderAction = getSliderAction(event->key());
    if (sliderAction != QAbstractSlider::SliderNoAction) {
      verticalScrollBar()->triggerAction(sliderAction);
      event->accept();
      return;
    }
  }

  QAbstractScrollArea::keyPressEvent(event);
}

void QLiteHtmlWidget::updateHightlightedLink() {
  QPoint viewportPos;
  QPoint pos;
  htmlPos(mapFromGlobal(QCursor::pos()), &viewportPos, &pos);
  setHightlightedLink(m_documentContainer.linkAt(pos, viewportPos));
}

void QLiteHtmlWidget::setHightlightedLink(const QUrl& url) {
  if (m_lastHighlightedLink == url)
    return;
  m_lastHighlightedLink = url;
  emit linkHighlighted(m_lastHighlightedLink);
}

void QLiteHtmlWidget::withFixedTextPosition(const std::function<void()>& action) {
  // remember element to which to scroll after re-rendering
  QPoint viewportPos;
  QPoint pos;
  htmlPos({}, &viewportPos, &pos); // top-left
  const int y = m_documentContainer.withFixedElementPosition(pos.y(), action);
  if (y >= 0)
    verticalScrollBar()->setValue(std::min(y, verticalScrollBar()->maximum()));
}

void QLiteHtmlWidget::render() {
  if (!m_documentContainer.hasDocument())
    return;
  const int fullWidth = width() / m_zoomFactor;
  const QSize vViewportSize = toVirtual(viewport()->size());
  const int scrollbarWidth = style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, this);
  const int w = fullWidth - scrollbarWidth - 2;
  m_documentContainer.render(w, vViewportSize.height());
  // scroll bars reflect virtual/scaled size of html document
  horizontalScrollBar()->setPageStep(vViewportSize.width());
  horizontalScrollBar()->setRange(0, std::max(0, m_documentContainer.documentWidth() - w));
  verticalScrollBar()->setPageStep(vViewportSize.height());
  verticalScrollBar()->setRange(0, std::max(0, m_documentContainer.documentHeight() - vViewportSize.height()));
  viewport()->update();
}

QPoint QLiteHtmlWidget::scrollPosition() const {
  return {horizontalScrollBar()->value(), verticalScrollBar()->value()};
}

void QLiteHtmlWidget::htmlPos(const QPoint& pos, QPoint* viewportPos, QPoint* htmlPos) const {
  *viewportPos = toVirtual(viewport()->mapFromParent(pos));
  *htmlPos = *viewportPos + scrollPosition();
}

QPoint QLiteHtmlWidget::toVirtual(const QPoint& p) const {
  return {int(p.x() / m_zoomFactor), int(p.y() / m_zoomFactor)};
}

QSize QLiteHtmlWidget::toVirtual(const QSize& s) const {
  return {int(s.width() / m_zoomFactor), int(s.height() / m_zoomFactor)};
}

QRect QLiteHtmlWidget::toVirtual(const QRect& r) const {
  return {toVirtual(r.topLeft()), toVirtual(r.size())};
}

QRect QLiteHtmlWidget::fromVirtual(const QRect& r) const {
  const QPoint tl{int(r.x() * m_zoomFactor), int(r.y() * m_zoomFactor)};
  // round size up, and add one since the topleft point was rounded down
  const QSize s{int(r.width() * m_zoomFactor + 0.5) + 1, int(r.height() * m_zoomFactor + 0.5) + 1};
  return {tl, s};
}
