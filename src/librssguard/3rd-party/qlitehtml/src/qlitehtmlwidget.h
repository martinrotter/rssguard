// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

///////////////////////
///
/// Edits!!!
/// - move QLiteHtmlWidgetPrivate to the header
/// - move container_qpainter.h to the header
/// - in draw_background function of container_qpainter.cpp, comment out the break
///   which was skipping painting of root backgrounds (body backgrounds)
///
//////////////////////

#include "qlitehtml_global.h"

#include "container_qpainter.h"

#include <QAbstractScrollArea>
#include <QTextDocument>

QT_FORWARD_DECLARE_CLASS(QPrinter)

#include <functional>

class QLiteHtmlWidgetPrivate
{
public:
    QString html;
    DocumentContainerContext context;
    QUrl url;
    DocumentContainer documentContainer;
    qreal zoomFactor = 1;
    QUrl lastHighlightedLink;
};

class QLITEHTML_EXPORT QLiteHtmlWidget : public QAbstractScrollArea
{
    Q_OBJECT
public:
    explicit QLiteHtmlWidget(QWidget *parent = nullptr);
    ~QLiteHtmlWidget() override;

    // declaring the getters Q_INVOKABLE to make them Squish-testable
    void setUrl(const QUrl &url);
    Q_INVOKABLE QUrl url() const;
    void setHtml(const QString &content);
    Q_INVOKABLE QString html() const;
    Q_INVOKABLE QString title() const;

    void setZoomFactor(qreal scale);
    qreal zoomFactor() const;

    bool findText(const QString &text,
                  QTextDocument::FindFlags flags,
                  bool incremental,
                  bool *wrapped = nullptr);

    void setDefaultFont(const QFont &font);
    QFont defaultFont() const;
    void setAntialias(bool on);

    void scrollToAnchor(const QString &name);

    using ResourceHandler = std::function<QByteArray(QUrl)>;
    void setResourceHandler(const ResourceHandler &handler);

    // declaring this Q_INVOKABLE to make it Squish-testable
    Q_INVOKABLE QString selectedText() const;

    void print(QPrinter *printer);

signals:
    void linkClicked(const QUrl &url);
    void linkHighlighted(const QUrl &url);
    void copyAvailable(bool available);
    void contextMenuRequested(const QPoint &pos, const QUrl &url);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

protected:
    void updateHightlightedLink();
    void setHightlightedLink(const QUrl &url);
    void withFixedTextPosition(const std::function<void()> &action);
    void render();
    QPoint scrollPosition() const;
    void htmlPos(const QPoint &pos, QPoint *viewportPos, QPoint *htmlPos) const;
    QPoint toVirtual(const QPoint &p) const;
    QSize toVirtual(const QSize &s) const;
    QRect toVirtual(const QRect &r) const;
    QRect fromVirtual(const QRect &r) const;

    QLiteHtmlWidgetPrivate *d;
};
