// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "container_qpainter.h"

#include <litehtml.h>

#include <QPaintDevice>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QVector>

#include <litehtml/types.h>
#include <unordered_map>

class Selection
{
public:
    struct Element
    {
        litehtml::element::ptr element;
        int index = -1;
        int x = -1;
    };

    enum class Mode { Free, Word };

    bool isValid() const;

    void update();
    QRect boundingRect() const;

    Element startElem;
    Element endElem;
    QVector<QRect> selection;
    QString text;

    QPoint selectionStartDocumentPos;
    Mode mode = Mode::Free;
    bool isSelecting = false;
};

struct Index
{
    QString text;
    // only contains leaf elements
    std::unordered_map<litehtml::element::ptr, int> elementToIndex;

    using Entry = std::pair<int, litehtml::element::ptr>;
    std::vector<Entry> indexToElement;

    Entry findElement(int index) const;
};

class DocumentContainerPrivate final : public litehtml::document_container
{
public: // document_container API
    litehtml::uint_ptr create_font(const char *faceName,
                                   int size,
                                   int weight,
                                   litehtml::font_style italic,
                                   unsigned int decoration,
                                   litehtml::font_metrics *fm) override;
    void delete_font(litehtml::uint_ptr hFont) override;
    int text_width(const char *text, litehtml::uint_ptr hFont) override;
    void draw_text(litehtml::uint_ptr hdc,
                   const char *text,
                   litehtml::uint_ptr hFont,
                   litehtml::web_color color,
                   const litehtml::position &pos) override;
    int pt_to_px(int pt) const override;
    int get_default_font_size() const override;
    const char *get_default_font_name() const override;
    void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker &marker) override;
    void load_image(const char *src, const char *baseurl, bool redraw_on_ready) override;
    void get_image_size(const char *src, const char *baseurl, litehtml::size &sz) override;
    void draw_background(litehtml::uint_ptr hdc,
                         const std::vector<litehtml::background_paint> &bgs) override;
    void draw_borders(litehtml::uint_ptr hdc,
                      const litehtml::borders &borders,
                      const litehtml::position &draw_pos,
                      bool root) override;
    void set_caption(const char *caption) override;
    void set_base_url(const char *base_url) override;
    void link(const std::shared_ptr<litehtml::document> &doc,
              const litehtml::element::ptr &el) override;
    void on_anchor_click(const char *url, const litehtml::element::ptr &el) override;
    void set_cursor(const char *cursor) override;
    void transform_text(std::string &text, litehtml::text_transform tt) override;
    void import_css(std::string &text, const std::string &url, std::string &baseurl) override;
    void set_clip(const litehtml::position &pos,
                  const litehtml::border_radiuses &bdr_radius) override;
    void del_clip() override;
    void get_client_rect(litehtml::position &client) const override;
    std::shared_ptr<litehtml::element> create_element(
        const char *tag_name,
        const litehtml::string_map &attributes,
        const std::shared_ptr<litehtml::document> &doc) override;
    void get_media_features(litehtml::media_features &media) const override;
    void get_language(std::string &language, std::string &culture) const override;

    QPixmap getPixmap(const QString &imageUrl, const QString &baseUrl);
    QString serifFont() const;
    QString sansSerifFont() const;
    QString monospaceFont() const;
    QUrl resolveUrl(const QString &url, const QString &baseUrl) const;
    void drawSelection(QPainter *painter, const QRect &clip) const;
    void buildIndex();
    void updateSelection();
    void clearSelection();

    QPaintDevice *m_paintDevice = nullptr;
    litehtml::document::ptr m_document;
    litehtml::media_type mediaType = litehtml::media_type_screen;
    Index m_index;
    QString m_baseUrl;
    QRect m_clientRect;
    QPoint m_scrollPosition;
    QString m_caption;
    QFont m_defaultFont = QFont(sansSerifFont(), 16);
    QByteArray m_defaultFontFamilyName = m_defaultFont.family().toUtf8();
    bool m_antialias = true;
    QHash<QUrl, QPixmap> m_pixmaps;
    Selection m_selection;
    DocumentContainer::DataCallback m_dataCallback;
    DocumentContainer::CursorCallback m_cursorCallback;
    DocumentContainer::LinkCallback m_linkCallback;
    DocumentContainer::PaletteCallback m_paletteCallback;
    DocumentContainer::ClipboardCallback m_clipboardCallback;
    bool m_blockLinks = false;
};

class DocumentContainerContextPrivate
{
public:
    QString masterCss;
};
