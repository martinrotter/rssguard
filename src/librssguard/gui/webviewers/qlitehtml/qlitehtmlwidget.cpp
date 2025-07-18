// For license of this file, see <project-root-folder>/LICENSE.md
// and
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gui/webviewers/qlitehtml/qlitehtmlwidget.h"

#include "miscellaneous/iofactory.h"

#include <QDebug>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QStyle>

#if QLITEHTML_HAS_QPRINTER
#include <QtPrintSupport/qprinter.h>
#endif

const int kScrollBarStep = 40;

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
    QUrl full_url = url;

    if (url.isRelative() && url.path(QUrl::ComponentFormattingOption::FullyEncoded).isEmpty()) {
      full_url = m_url;
      full_url.setFragment(url.fragment(QUrl::ComponentFormattingOption::FullyEncoded));
    }

    QMetaObject::invokeMethod(
      this,
      [this, full_url] {
        emit linkClicked(full_url);
      },
      Qt::ConnectionType::QueuedConnection);
  });

  m_documentContainer.setClipboardCallback([this](bool yes) {
    emit copyAvailable(yes);
  });

  m_documentContainer.setMasterCss(QString::fromUtf8(IOFactory::readFile(QSL(":/litehtml/master.css"))));
}

QLiteHtmlWidget::~QLiteHtmlWidget() {}

void QLiteHtmlWidget::setFontAntialiasing(bool on) {
  withFixedTextPosition([this, on] {
    m_documentContainer.setFontAntialiasing(on);

    // NOTE: Force litehtml to recreate fonts.
    setHtml(m_html);
  });
}

void QLiteHtmlWidget::setUrl(const QUrl& url) {
  m_url = url;

  QUrl base_url = url;
  base_url.setFragment({});

  const QString path = base_url.path(QUrl::ComponentFormattingOption::FullyEncoded);
  const int last_slash = path.lastIndexOf('/');
  const QString base_path = last_slash >= 0 ? path.left(last_slash) : QString();

  base_url.setPath(base_path);
  m_documentContainer.setBaseUrl(base_url.toString(QUrl::ComponentFormattingOption::FullyEncoded));

  QMetaObject::invokeMethod(
    this,
    [this] {
      updateHightlightedLink();
    },
    Qt::ConnectionType::QueuedConnection);
}

QUrl QLiteHtmlWidget::url() const {
  return m_url;
}

void QLiteHtmlWidget::setHtml(const QString& content) {
  m_html = content;

  m_documentContainer.setPaintDevice(viewport());
  m_documentContainer.setDocument(content.toUtf8());

  verticalScrollBar()->setValue(0);
  horizontalScrollBar()->setValue(0);
  render();

  QMetaObject::invokeMethod(
    this,
    [this] {
      updateHightlightedLink();
    },
    Qt::ConnectionType::QueuedConnection);
}

QString QLiteHtmlWidget::html() const {
  return m_html;
}

QString QLiteHtmlWidget::title() const {
  return m_documentContainer.caption();
}

void QLiteHtmlWidget::setZoomFactor(qreal scale) {
  Q_ASSERT(scale != 0.0);

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
  QVector<QRect> old_selection;
  QVector<QRect> new_selection;

  m_documentContainer.findText(text, flags, incremental, wrapped, &success, &old_selection, &new_selection);

  QRect new_selection_combined;

  for (const QRect& r : std::as_const(new_selection)) {
    new_selection_combined = new_selection_combined.united(r);
  }

  QScrollBar* v_bar = verticalScrollBar();
  const int top = new_selection_combined.top();
  const int bottom = new_selection_combined.bottom() - toVirtual(viewport()->size()).height();

  if (success && top < v_bar->value() && v_bar->minimum() <= top) {
    v_bar->setValue(top);
  }
  else if (success && v_bar->value() < bottom && bottom <= v_bar->maximum()) {
    v_bar->setValue(bottom);
  }
  else {
    viewport()->update(fromVirtual(new_selection_combined.translated(-scrollPosition())));

    for (const QRect& r : std::as_const(old_selection)) {
      viewport()->update(fromVirtual(r.translated(-scrollPosition())));
    }
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
  if (!m_documentContainer.hasDocument()) {
    return;
  }

  horizontalScrollBar()->setValue(0);

  if (name.isEmpty()) {
    verticalScrollBar()->setValue(0);
    return;
  }

  const int y = m_documentContainer.anchorY(name);

  if (y >= 0) {
    verticalScrollBar()->setValue(std::min(y, verticalScrollBar()->maximum()));
  }
}

void QLiteHtmlWidget::setResourceHandler(const DocumentContainer::DataCallback &handler) {
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
  if (!m_documentContainer.hasDocument()) {
    return;
  }

  m_documentContainer.setScrollPosition(scrollPosition());

  QPainter p(viewport());

  p.setWorldTransform(QTransform().scale(m_zoomFactor, m_zoomFactor));
  p.setRenderHint(QPainter::RenderHint::SmoothPixmapTransform, true);
  p.setRenderHint(QPainter::RenderHint::Antialiasing, m_documentContainer.fontAntialiasing());

  m_documentContainer.draw(&p, toVirtual(event->rect()));
}

void QLiteHtmlWidget::resizeEvent(QResizeEvent* event) {
  withFixedTextPosition([this, event] {
    QAbstractScrollArea::resizeEvent(event);
    render();
  });
}

void QLiteHtmlWidget::mouseMoveEvent(QMouseEvent* event) {
  QPoint viewport_pos;
  QPoint pos;

  htmlPos(event->pos(), &viewport_pos, &pos);

  const QVector<QRect> areas = m_documentContainer.mouseMoveEvent(pos, viewport_pos);

  for (const QRect& r : areas) {
    viewport()->update(fromVirtual(r.translated(-scrollPosition())));
  }

  updateHightlightedLink();
}

void QLiteHtmlWidget::mousePressEvent(QMouseEvent* event) {
  QPoint viewport_pos;
  QPoint pos;

  htmlPos(event->pos(), &viewport_pos, &pos);

  const QVector<QRect> areas = m_documentContainer.mousePressEvent(pos, viewport_pos, event->button());

  for (const QRect& r : areas) {
    viewport()->update(fromVirtual(r.translated(-scrollPosition())));
  }
}

void QLiteHtmlWidget::mouseReleaseEvent(QMouseEvent* event) {
  QPoint viewport_pos;
  QPoint pos;

  htmlPos(event->pos(), &viewport_pos, &pos);

  const QVector<QRect> areas = m_documentContainer.mouseReleaseEvent(pos, viewport_pos, event->button());

  for (const QRect& r : areas) {
    viewport()->update(fromVirtual(r.translated(-scrollPosition())));
  }
}

void QLiteHtmlWidget::mouseDoubleClickEvent(QMouseEvent* event) {
  QPoint viewport_pos;
  QPoint pos;

  htmlPos(event->pos(), &viewport_pos, &pos);

  const QVector<QRect> areas = m_documentContainer.mouseDoubleClickEvent(pos, viewport_pos, event->button());

  for (const QRect& r : areas) {
    viewport()->update(fromVirtual(r.translated(-scrollPosition())));
  }
}

void QLiteHtmlWidget::leaveEvent(QEvent* event) {
  Q_UNUSED(event)

  const QVector<QRect> areas = m_documentContainer.leaveEvent();

  for (const QRect& r : areas) {
    viewport()->update(fromVirtual(r.translated(-scrollPosition())));
  }

  setHightlightedLink(QUrl());
}

void QLiteHtmlWidget::contextMenuEvent(QContextMenuEvent* event) {
  QPoint viewport_pos;
  QPoint pos;

  htmlPos(event->pos(), &viewport_pos, &pos);

  emit contextMenuRequested(event->pos(), m_documentContainer.linkAt(pos, viewport_pos));
}

static QAbstractSlider::SliderAction getSliderAction(int key) {
  switch (key) {
    case Qt::Key::Key_Home:
      return QAbstractSlider::SliderAction::SliderToMinimum;

    case Qt::Key::Key_End:
      return QAbstractSlider::SliderAction::SliderToMaximum;

    case Qt::Key::Key_PageUp:
      return QAbstractSlider::SliderAction::SliderPageStepSub;

    case Qt::Key::Key_PageDown:
      return QAbstractSlider::SliderAction::SliderPageStepAdd;

    default:
      return QAbstractSlider::SliderAction::SliderNoAction;
  }
}

void QLiteHtmlWidget::keyPressEvent(QKeyEvent* event) {
  if (event->modifiers() == Qt::KeyboardModifier::NoModifier ||
      event->modifiers() == Qt::KeyboardModifier::KeypadModifier) {
    const QAbstractSlider::SliderAction slider_act = getSliderAction(event->key());

    if (slider_act != QAbstractSlider::SliderAction::SliderNoAction) {
      verticalScrollBar()->triggerAction(slider_act);
      event->accept();
      return;
    }
  }

  QAbstractScrollArea::keyPressEvent(event);
}

void QLiteHtmlWidget::updateHightlightedLink() {
  QPoint viewport_pos;
  QPoint pos;

  htmlPos(mapFromGlobal(QCursor::pos()), &viewport_pos, &pos);
  setHightlightedLink(m_documentContainer.linkAt(pos, viewport_pos));
}

void QLiteHtmlWidget::setHightlightedLink(const QUrl& url) {
  if (m_lastHighlightedLink == url) {
    return;
  }

  emit linkHighlighted(m_lastHighlightedLink = url);
}

const DocumentContainer* QLiteHtmlWidget::documentContainer() const {
  return &m_documentContainer;
}

void QLiteHtmlWidget::withFixedTextPosition(const std::function<void()>& action) {
  // remember element to which to scroll after re-rendering
  QPoint viewport_pos;
  QPoint pos;

  htmlPos({}, &viewport_pos, &pos);

  const int y = m_documentContainer.withFixedElementPosition(pos.y(), action);

  if (y >= 0) {
    verticalScrollBar()->setValue(std::min(y, verticalScrollBar()->maximum()));
  }
}

void QLiteHtmlWidget::render() {
  if (!m_documentContainer.hasDocument()) {
    return;
  }

  const int full_width = width() / m_zoomFactor;
  const QSize viewport_size = toVirtual(viewport()->size());
  const int scrollbar_width = style()->pixelMetric(QStyle::PixelMetric::PM_ScrollBarExtent, nullptr, this);
  const int w = full_width - scrollbar_width - 2;

  m_documentContainer.render(w, viewport_size.height());

  horizontalScrollBar()->setPageStep(viewport_size.width());
  horizontalScrollBar()->setRange(0, std::max(0, m_documentContainer.documentWidth() - w));
  verticalScrollBar()->setPageStep(viewport_size.height());
  verticalScrollBar()->setRange(0, std::max(0, m_documentContainer.documentHeight() - viewport_size.height()));

  viewport()->update();
}

QPoint QLiteHtmlWidget::scrollPosition() const {
  return {horizontalScrollBar()->value(), verticalScrollBar()->value()};
}

void QLiteHtmlWidget::htmlPos(QPoint pos, QPoint* viewport_pos, QPoint* html_pos) const {
  *viewport_pos = toVirtual(viewport()->mapFromParent(pos));
  *html_pos = *viewport_pos + scrollPosition();
}

QPoint QLiteHtmlWidget::toVirtual(QPoint p) const {
  return {int(p.x() / m_zoomFactor), int(p.y() / m_zoomFactor)};
}

QSize QLiteHtmlWidget::toVirtual(QSize s) const {
  return {int(s.width() / m_zoomFactor), int(s.height() / m_zoomFactor)};
}

QRect QLiteHtmlWidget::toVirtual(QRect r) const {
  return {toVirtual(r.topLeft()), toVirtual(r.size())};
}

QRect QLiteHtmlWidget::fromVirtual(QRect r) const {
  const QPoint tl{int(r.x() * m_zoomFactor), int(r.y() * m_zoomFactor)};
  const QSize s{int(r.width() * m_zoomFactor + 0.5) + 1, int(r.height() * m_zoomFactor + 0.5) + 1};

  return {tl, s};
}
