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

#include <QMutex>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QTextDocument>
#include <QTimer>
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
    QRectF boundingRect() const;

    Element m_startElem;
    Element m_endElem;
    QVector<QRectF> m_selection;
    QString m_text;

    QPointF m_startingPos;
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

class Downloader;

class DocumentContainer : public QObject, litehtml::document_container {
    Q_OBJECT

  public:
    explicit DocumentContainer();
    virtual ~DocumentContainer();

    // document_container API.
    virtual litehtml::uint_ptr create_font(const litehtml::font_description& descr,
                                           const litehtml::document* doc,
                                           litehtml::font_metrics* fm);
    virtual void delete_font(litehtml::uint_ptr fnt) override;
    virtual litehtml::pixel_t text_width(const char* text, litehtml::uint_ptr fnt) override;
    virtual void draw_text(litehtml::uint_ptr hdc,
                           const char* text,
                           litehtml::uint_ptr fnt,
                           litehtml::web_color color,
                           const litehtml::position& pos) override;
    virtual litehtml::pixel_t pt_to_px(float pt) const override;
    virtual litehtml::pixel_t get_default_font_size() const override;
    virtual const char* get_default_font_name() const override;
    virtual void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker& marker) override;
    virtual void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
    virtual void get_image_size(const char* src, const char* baseurl, litehtml::size& sz) override;
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
    virtual void draw_borders(litehtml::uint_ptr hdc,
                              const litehtml::borders& borders,
                              const litehtml::position& draw_pos,
                              bool root) override;
    virtual void set_caption(const char* caption) override;
    virtual void set_base_url(const char* base_url) override;
    virtual void link(const std::shared_ptr<litehtml::document>& doc, const litehtml::element::ptr& el) override;
    virtual void on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
    virtual void set_cursor(const char* cursor) override;
    virtual void transform_text(std::string& text, litehtml::text_transform tt) override;
    virtual void import_css(std::string& text, const std::string& url, std::string& baseurl) override;
    virtual void set_clip(const litehtml::position& pos, const litehtml::border_radiuses& bdr_radius) override;
    virtual void del_clip() override;
    virtual std::shared_ptr<litehtml::element> create_element(const char* tag_name,
                                                              const litehtml::string_map& attributes,
                                                              const std::shared_ptr<litehtml::document>& doc) override;
    virtual void get_media_features(litehtml::media_features& media) const override;
    virtual void get_language(std::string& language, std::string& culture) const override;

    // Outside API.
    enum class MediaType {
      All,
      Screen,
      Print
    };

    enum class RequestType {
      // Data handler has to return (placeholder or the actual) QPixmap (can be async).
      // So the image is either directly downloaded and returned or download
      // is started asynchronously and placeholder is returned in the meantime.
      ImageDownload,

      // Data handler has to return QPixmap (only sync).
      // No downloading is performed and cached image is returned.
      ImageDisplay,

      // Data handler has to return QByteArray (only sync).
      // Data is downloaded directly and returned if not present in the cache.
      CssDownload
    };

    using CursorCallback = std::function<void(QCursor)>;
    using LinkCallback = std::function<void(QUrl)>;
    using PaletteCallback = std::function<QPalette()>;
    using ClipboardCallback = std::function<void(bool)>;

    void setPaintDevice(QPaintDevice* paint_device);
    void setScrollPosition(const QPoint& pos);
    void render(int width, int height);
    void draw(QPainter* painter, QRectF clip);

    int documentWidth() const;
    int documentHeight() const;
    int anchorY(const QString& anchor_name) const;

    void setMediaType(MediaType t);

    // these return areas to redraw in document space
    QVector<QRectF> mousePressEvent(QPointF document_pos, QPointF viewportPosos, Qt::MouseButton button);
    QVector<QRectF> mouseMoveEvent(QPointF documentPoss, QPointF viewport_pos);
    QVector<QRectF> mouseReleaseEvent(QPointF document_pos, QPointF viewport_pos, Qt::MouseButton button);
    QVector<QRectF> mouseDoubleClickEvent(QPointF document_pos, QPointF viewport_pos, Qt::MouseButton button);
    QVector<QRectF> leaveEvent();

    Downloader* downloader() const;

    QUrl linkAt(QPointF document_pos, QPointF viewportPos) const;

    QString caption() const;
    QString selectedText() const;

    void findText(const QString& text,
                  QTextDocument::FindFlags flags,
                  bool incremental,
                  bool* wrapped,
                  bool* success,
                  QVector<QRectF>* old_selection,
                  QVector<QRectF>* new_selection);

    void setDocument(const QByteArray& data);
    bool hasDocument() const;

    void setBaseUrl(const QString& url);
    QString baseUrl() const;

    void setDefaultFont(const QFont& font);
    QFont defaultFont() const;

    void setFontAntialiasing(bool on);
    bool fontAntialiasing() const;

    QString masterCss() const;
    void setMasterCss(const QString& master_css);

    void setCursorCallback(const CursorCallback& callback);
    void setLinkCallback(const LinkCallback& callback);
    void setPaletteCallback(const PaletteCallback& callback);
    void setClipboardCallback(const ClipboardCallback& callback);

    int withFixedElementPosition(int y, const std::function<void()>& action);

    QNetworkProxy networkProxy() const;
    void setNetworkProxy(const QNetworkProxy& network_proxy);

    bool loadExternalResources() const;
    void setLoadExternalResources(bool load_resources);

    bool shapeAntialiasing() const;
    void setShapeAntialiasing(bool on);

  signals:
    void renderRequested();

  private slots:
    void downloadNextExternalResource();
    QVariant handleExternalResource(DocumentContainer::RequestType type, const QUrl& url);
    void onResourceDownloadCompleted(const QUrl& url,
                                     QNetworkReply::NetworkError status,
                                     int http_code,
                                     const QByteArray& contents);

  private:
    void drawRectWithLambda(litehtml::uint_ptr hdc,
                            const litehtml::background_layer& layer,
                            std::function<void(QPainter*)> lmbd);

    QPixmap getPixmap(const QString& image_url, const QString& base_url);

    QString serifFont() const;
    QString sansSerifFont() const;
    QString monospaceFont() const;

    QUrl resolveUrl(const QString& url, const QString& base_url) const;

    void drawSelection(QPainter* painter, const QRectF& clip) const;
    void buildIndex();
    void updateSelection();
    void clearSelection();

    QPaintDevice* m_paintDevice = nullptr;
    litehtml::document::ptr m_document;
    litehtml::media_type m_mediaType = litehtml::media_type_screen;
    Index m_index;
    QString m_baseUrl;
    QRect m_clientRect;
    QPoint m_scrollPosition;
    QString m_caption;
    QFont m_defaultFont = QFont(sansSerifFont(), 16);
    QByteArray m_defaultFontFamilyName = m_defaultFont.family().toUtf8();
    bool m_fontAntialiasing = true;
    bool m_shapeAntialiasing = true;
    Selection m_selection;
    DocumentContainer::CursorCallback m_cursorCallback;
    DocumentContainer::LinkCallback m_linkCallback;
    DocumentContainer::PaletteCallback m_paletteCallback;
    DocumentContainer::ClipboardCallback m_clipboardCallback;
    bool m_blockLinks = false;
    QString m_masterCss;

    QPixmap m_placeholderImage;
    QPixmap m_placeholderImageError;
    QHash<QUrl, QVariant> m_dataCache;
    bool m_loadExternalResources;

    Downloader* m_downloader;

    QTimer m_timerRerender;
    QTimer m_timerForPendingExternalResources;
    QList<QUrl> m_pendingExternalResources;
};
