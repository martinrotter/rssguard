// For license of this file, see <project-root-folder>/LICENSE.md
// and
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gui/webviewers/qlitehtml/documentcontainer.h"

#include <functional>

#include <QAbstractScrollArea>
#include <QTextDocument>

class QPrinter;

class QLiteHtmlWidget : public QAbstractScrollArea {
    Q_OBJECT

  public:
    explicit QLiteHtmlWidget(QWidget* parent = nullptr);
    virtual ~QLiteHtmlWidget();

    void setUrl(const QUrl& url);
    QUrl url() const;

    void setHtml(const QString& content);
    QString html() const;

    QString title() const;
    QString selectedText() const;

    const DocumentContainer* documentContainer() const;
    DocumentContainer* documentContainer();

    void setZoomFactor(qreal scale);
    qreal zoomFactor() const;

    QPoint scrollPosition() const;

    bool findText(const QString& text, QTextDocument::FindFlags flags, bool incremental, bool* wrapped = nullptr);

    void setDefaultFont(const QFont& font);
    QFont defaultFont() const;

    void setFontAntialiasing(bool on);

    void scrollToAnchor(const QString& name);

    using ResourceHandler = std::function<QByteArray(QUrl)>;
    void setResourceHandler(const DocumentContainer::DataCallback& handler);

    void print(QPrinter* printer);

  signals:
    void linkClicked(const QUrl& url);
    void linkHighlighted(const QUrl& url);
    void copyAvailable(bool available);
    void contextMenuRequested(const QPoint& pos, const QUrl& url);

  protected:
    virtual void paintEvent(QPaintEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void leaveEvent(QEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);

  protected:
    void render();
    void withFixedTextPosition(const std::function<void()>& action);
    void htmlPos(QPointF pos, QPointF* viewport_pos, QPointF* html_pos) const;

    QPointF toVirtual(QPointF p) const;
    QSizeF toVirtual(QSizeF s) const;
    QRectF toVirtual(const QRectF& r) const;
    QRectF fromVirtual(const QRectF& r) const;

  private:
    void updateHightlightedLink();
    void setHightlightedLink(const QUrl& url);

    QString m_html;
    QUrl m_url;
    DocumentContainer m_documentContainer;
    qreal m_zoomFactor = 1;
    QUrl m_lastHighlightedLink;
};
