// For license of this file, see <project-root-folder>/LICENSE.md
// and
// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <functional>
#include <litehtml.h>
#include <litehtml/types.h>
#include <memory>
#include <unordered_map>

#include <QPoint>
#include <QRect>
#include <QString>
#include <QTextDocument>
#include <QUrl>
#include <QVector>

class QPaintDevice;
class QPainter;

class Selection {
  public:
    struct Element {
        litehtml::element::ptr element;
        int index = -1;
        int x = -1;
    };

    enum class Mode {
      Free,
      Word
    };

    bool isValid() const;

    void update();
    QRect boundingRect() const;

    Element m_startElem;
    Element m_endElem;
    QVector<QRect> m_selection;
    QString m_text;

    QPoint m_startingPos;
    Mode m_mode = Mode::Free;
    bool m_isSelecting = false;
};

struct Index {
    QString m_text;

    // NOTE: Only contains leaf elements.
    std::unordered_map<litehtml::element::ptr, int> m_elementToIndex;

    using Entry = std::pair<int, litehtml::element::ptr>;
    std::vector<Entry> m_indexToElement;

    Entry findElement(int index) const;
};

class DocumentContainer : public litehtml::document_container {
  public:
    explicit DocumentContainer();
    virtual ~DocumentContainer();

  public: // Outside API.
    void setPaintDevice(QPaintDevice* paintDevice);
    void setDocument(const QByteArray& data);
    bool hasDocument() const;
    void setBaseUrl(const QString& url);
    QString baseUrl() const;
    void setScrollPosition(const QPoint& pos);
    void render(int width, int height);
    void draw(QPainter* painter, const QRect& clip);
    int documentWidth() const;
    int documentHeight() const;
    int anchorY(const QString& anchorName) const;

    enum class MediaType {
      All,
      Screen,
      Print
    };

    void setMediaType(MediaType t);

    // these return areas to redraw in document space
    QVector<QRect> mousePressEvent(const QPoint& documentPos, const QPoint& viewportPos, Qt::MouseButton button);
    QVector<QRect> mouseMoveEvent(const QPoint& documentPos, const QPoint& viewportPos);
    QVector<QRect> mouseReleaseEvent(const QPoint& documentPos, const QPoint& viewportPos, Qt::MouseButton button);
    QVector<QRect> mouseDoubleClickEvent(const QPoint& documentPos, const QPoint& viewportPos, Qt::MouseButton button);
    QVector<QRect> leaveEvent();

    QUrl linkAt(const QPoint& documentPos, const QPoint& viewportPos) const;

    QString caption() const;
    QString selectedText() const;

    void findText(const QString& text,
                  QTextDocument::FindFlags flags,
                  bool incremental,
                  bool* wrapped,
                  bool* success,
                  QVector<QRect>* oldSelection,
                  QVector<QRect>* newSelection);

    void setDefaultFont(const QFont& font);
    QFont defaultFont() const;

    void setFontAntialiasing(bool on);
    bool fontAntialiasing() const;

    QString masterCss() const;
    void setMasterCss(const QString& master_css);

    using DataCallback = std::function<QByteArray(QUrl)>;
    using CursorCallback = std::function<void(QCursor)>;
    using LinkCallback = std::function<void(QUrl)>;
    using PaletteCallback = std::function<QPalette()>;
    using ClipboardCallback = std::function<void(bool)>;

    void setDataCallback(const DataCallback& callback);
    void setCursorCallback(const CursorCallback& callback);
    void setLinkCallback(const LinkCallback& callback);
    void setPaletteCallback(const PaletteCallback& callback);
    void setClipboardCallback(const ClipboardCallback& callback);

    int withFixedElementPosition(int y, const std::function<void()>& action);

  public: // document_container API
    virtual litehtml::uint_ptr create_font(const litehtml::font_description& descr,
                                           const litehtml::document* doc,
                                           litehtml::font_metrics* fm);
    void delete_font(litehtml::uint_ptr hFont) override;
    int text_width(const char* text, litehtml::uint_ptr hFont) override;
    void draw_text(litehtml::uint_ptr hdc,
                   const char* text,
                   litehtml::uint_ptr hFont,
                   litehtml::web_color color,
                   const litehtml::position& pos) override;
    int pt_to_px(int pt) const override;
    int get_default_font_size() const override;
    const char* get_default_font_name() const override;
    void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override;
    void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
    void get_image_size(const char* src, const char* baseurl, litehtml::size& sz) override;

    virtual void draw_image(litehtml::uint_ptr hdc,
                            const litehtml::background_layer& layer,
                            const std::string& url,
                            const std::string& base_url);
    virtual void draw_solid_fill(litehtml::uint_ptr hdc,
                                 const litehtml::background_layer& layer,
                                 const litehtml::web_color& color);

    virtual void draw_linear_gradient(litehtml::uint_ptr hdc,
                                      const litehtml::background_layer& layer,
                                      const litehtml::background_layer::linear_gradient& gradient);
    virtual void draw_radial_gradient(litehtml::uint_ptr hdc,
                                      const litehtml::background_layer& layer,
                                      const litehtml::background_layer::radial_gradient& gradient);
    virtual void draw_conic_gradient(litehtml::uint_ptr hdc,
                                     const litehtml::background_layer& layer,
                                     const litehtml::background_layer::conic_gradient& gradient);

    virtual void on_mouse_event(const litehtml::element::ptr& el, litehtml::mouse_event event);
    virtual void get_viewport(litehtml::position& viewport) const;

    void draw_borders(litehtml::uint_ptr hdc,
                      const litehtml::borders& borders,
                      const litehtml::position& draw_pos,
                      bool root) override;
    void set_caption(const char* caption) override;
    void set_base_url(const char* base_url) override;
    void link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) override;
    void on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
    void set_cursor(const char* cursor) override;
    void transform_text(std::string& text, litehtml::text_transform tt) override;
    void import_css(std::string& text, const std::string& url, std::string& baseurl) override;
    void set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) override;
    void del_clip() override;
    std::shared_ptr<litehtml::element> create_element(const char* tag_name,
                                                      const litehtml::string_map& attributes,
                                                      const std::shared_ptr<litehtml::document>& doc) override;
    void get_media_features(litehtml::media_features& media) const override;
    void get_language(std::string& language, std::string& culture) const override;

    QPixmap getPixmap(const QString& imageUrl, const QString& baseUrl);
    QString serifFont() const;
    QString sansSerifFont() const;
    QString monospaceFont() const;
    QUrl resolveUrl(const QString& url, const QString& baseUrl) const;
    void drawSelection(QPainter* painter, const QRect& clip) const;
    void buildIndex();
    void updateSelection();
    void clearSelection();

    QPaintDevice* m_paintDevice = nullptr;
    litehtml::document::ptr m_document;
    litehtml::media_type mediaType = litehtml::media_type_screen;
    Index m_index;
    QString m_baseUrl;
    QRect m_clientRect;
    QPoint m_scrollPosition;
    QString m_caption;
    QFont m_defaultFont = QFont(sansSerifFont(), 16);
    QByteArray m_defaultFontFamilyName = m_defaultFont.family().toUtf8();
    bool m_fontAntialiasing = true;
    QHash<QUrl, QPixmap> m_pixmaps;
    Selection m_selection;
    DocumentContainer::DataCallback m_dataCallback;
    DocumentContainer::CursorCallback m_cursorCallback;
    DocumentContainer::LinkCallback m_linkCallback;
    DocumentContainer::PaletteCallback m_paletteCallback;
    DocumentContainer::ClipboardCallback m_clipboardCallback;
    bool m_blockLinks = false;

  private:
    QString m_masterCss;
};
