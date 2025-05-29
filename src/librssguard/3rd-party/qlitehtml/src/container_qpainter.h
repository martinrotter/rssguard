// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qlitehtml_global.h"

#include <QByteArray>
#include <QPaintDevice>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QTextDocument>
#include <QUrl>
#include <QVector>

#include <functional>
#include <memory>

class DocumentContainerPrivate;
class DocumentContainerContextPrivate;

class QLITEHTML_EXPORT DocumentContainerContext
{
public:
    DocumentContainerContext();
    ~DocumentContainerContext();

    void setMasterStyleSheet(const QString &css);

private:
    std::unique_ptr<DocumentContainerContextPrivate> d;

    friend class DocumentContainer;
    friend class DocumentContainerPrivate;
};

class QLITEHTML_EXPORT DocumentContainer
{
public:
    DocumentContainer();
    virtual ~DocumentContainer();

public: // outside API
    void setPaintDevice(QPaintDevice *paintDevice);
    void setDocument(const QByteArray &data, DocumentContainerContext *context);
    bool hasDocument() const;
    void setBaseUrl(const QString &url);
    QString baseUrl() const;
    void setScrollPosition(const QPoint &pos);
    void render(int width, int height);
    void draw(QPainter *painter, const QRect &clip);
    int documentWidth() const;
    int documentHeight() const;
    int anchorY(const QString &anchorName) const;

    enum class MediaType
    {
        None,
        All,
        Screen,
        Print,
        Braille,
        Embossed,
        Handheld,
        Projection,
        Speech,
        TTY,
        TV
    };

    void setMediaType(MediaType t);

    // these return areas to redraw in document space
    QVector<QRect> mousePressEvent(const QPoint &documentPos,
                                   const QPoint &viewportPos,
                                   Qt::MouseButton button);
    QVector<QRect> mouseMoveEvent(const QPoint &documentPos, const QPoint &viewportPos);
    QVector<QRect> mouseReleaseEvent(const QPoint &documentPos,
                                     const QPoint &viewportPos,
                                     Qt::MouseButton button);
    QVector<QRect> mouseDoubleClickEvent(const QPoint &documentPos,
                                         const QPoint &viewportPos,
                                         Qt::MouseButton button);
    QVector<QRect> leaveEvent();

    QUrl linkAt(const QPoint &documentPos, const QPoint &viewportPos);

    QString caption() const;
    QString selectedText() const;

    void findText(const QString &text,
                  QTextDocument::FindFlags flags,
                  bool incremental,
                  bool *wrapped,
                  bool *success,
                  QVector<QRect> *oldSelection,
                  QVector<QRect> *newSelection);

    void setDefaultFont(const QFont &font);
    QFont defaultFont() const;
    void setAntialias(bool on);
    bool antialias() const;

    using DataCallback = std::function<QByteArray(QUrl)>;
    void setDataCallback(const DataCallback &callback);
    DataCallback dataCallback() const;

    using CursorCallback = std::function<void(QCursor)>;
    void setCursorCallback(const CursorCallback &callback);

    using LinkCallback = std::function<void(QUrl)>;
    void setLinkCallback(const LinkCallback &callback);

    using PaletteCallback = std::function<QPalette()>;
    void setPaletteCallback(const PaletteCallback &callback);
    PaletteCallback paletteCallback() const;

    using ClipboardCallback = std::function<void(bool)>;
    void setClipboardCallback(const ClipboardCallback &callback);

    int withFixedElementPosition(int y, const std::function<void()> &action);

private:
    std::unique_ptr<DocumentContainerPrivate> d;
};
