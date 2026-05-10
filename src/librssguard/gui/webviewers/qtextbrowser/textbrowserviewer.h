// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TEXTBROWSERVIEWER_H
#define TEXTBROWSERVIEWER_H

#include "gui/webviewers/webviewer.h"
#include "network-web/networkfactory.h"

#include <atomic>

#include <QImage>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QPixmap>
#include <QPointer>
#include <QTextBrowser>
#include <QThread>
#include <QTimer>

class QContextMenuEvent;
class QResizeEvent;
class WebBrowser;

class TextBrowserViewer;

class RSSGUARD_DLLSPEC TextBrowserImageCache {
  public:
    static QString cacheRootFolder();
    static QString cacheFilePath(const QUrl& image_url);

    static bool loadImage(const QUrl& image_url, QImage& image);
    static bool saveImage(const QUrl& image_url, const QByteArray& image_data);

    static void cleanup();
};

class RSSGUARD_DLLSPEC TextBrowserImageDownloader : public QObject {
    Q_OBJECT

  public:
    explicit TextBrowserImageDownloader(QList<QUrl> image_urls, QNetworkProxy custom_proxy, QObject* parent = nullptr);

    void cancel();

  public slots:
    void downloadImages();

  signals:
    void imageDownloaded(const QUrl& image_url, const QImage& image);
    void downloadProgress(int progress);
    void downloadFinished(bool success);

  private:
    QList<QUrl> m_imageUrls;
    QNetworkProxy m_customProxy;
    std::atomic_bool m_cancelled = false;
};

class RSSGUARD_DLLSPEC TextBrowserDocument : public QTextDocument {
    Q_OBJECT

  public:
    explicit TextBrowserDocument(TextBrowserViewer* parent = nullptr);

  protected:
    virtual QVariant loadResource(int type, const QUrl& name);

  private:
    QPointer<TextBrowserViewer> m_viewer;
};

class RSSGUARD_DLLSPEC TextBrowserViewer : public QTextBrowser, public WebViewer {
    Q_OBJECT
    Q_INTERFACES(WebViewer)

    friend class TextBrowserDocument;

  public:
    explicit TextBrowserViewer(QWidget* parent = nullptr);
    virtual ~TextBrowserViewer();

    virtual QSize sizeHint() const;

    virtual void reloadNetworkSettings();
    virtual void bindToBrowser(WebBrowser* browser);
    virtual void findText(const QString& text, bool backwards);
    virtual void loadUrl(const QUrl& url);
    virtual void setHtml(const QString& html, const QUrl& url = {}, RootItem* root = nullptr);
    virtual QString html() const;
    virtual QString plainText() const;
    virtual QUrl url() const;
    virtual void clear();
    virtual void loadMessage(const Message& message, RootItem* root);
    virtual QString htmlForMessage(const Message& message, RootItem* root) const;
    virtual void printToPrinter(QPrinter* printer);
    virtual void cleanupCache();
    virtual double verticalScrollBarPosition() const;
    virtual void setVerticalScrollBarPosition(double pos);
    virtual void applyFont(const QFont& fon);
    virtual void reloadPage();
    virtual void goBack();
    virtual void goForward();
    virtual bool supportImagesLoading() const;
    virtual bool supportsNavigation() const;
    virtual void setLoadExternalResources(bool load_resources);

    virtual qreal zoomFactor() const;
    virtual void setZoomFactor(qreal zoom_factor);

  protected:
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual ContextMenuData provideContextMenuData(QContextMenuEvent* event);

  signals:
    void reloadPageEnabledChanged(bool can_go_forward);
    void goBackEnabledChanged(bool can_go_back);
    void goForwardEnabledChanged(bool can_go_forward);
    void pageTitleChanged(const QString& new_title);
    void pageUrlChanged(const QUrl& url);
    void pageIconChanged(const QIcon&);
    void linkMouseHighlighted(const QUrl& url);
    void linkMouseClicked(const QUrl& url, LinkNavigationHints hints = LinkNavigationHints::None);
    void loadingStarted();
    void loadingProgress(int progress);
    void loadingFinished(bool success);
    void openUrlInNewTab(bool open_externally, const QUrl& url);
    void openViewerInNewTab(WebViewer* viewer);

  private:
    void displayDownloadedPage(const QUrl& url, const QByteArray& data, const NetworkResult& res);
    bool loadStaticHtml(const QString& html, const QUrl& url = {}, RootItem* root = nullptr);
    void justSetHtml(const QString& html, const QUrl& url = {}, RootItem* root = nullptr, bool keep_scroll = false);
    QString htmlToDisplay(const QString& html) const;
    QString convertToHtmlWithoutImages(const QString& html) const;

    void abortImageDownloading();
    bool startImageDownloading();
    void reloadHtmlWithCachedImages();

    QList<QUrl> imageUrlsForHtml(const QString& html, const QUrl& base_url) const;
    QUrl imageUrlAt(const QPoint& pos) const;
    QUrl imageUrlFromCursor(const QTextCursor& cursor) const;
    QUrl resolvedResourceUrl(const QUrl& resource_url) const;
    QNetworkProxy networkProxyForCurrentRoot() const;

  private:
    WebBrowser* m_browser;
    QUrl m_currentUrl;
    QString m_currentHtml;
    QFont m_baseFont;
    qreal m_zoomFactor = 1.0;
    QScopedPointer<TextBrowserDocument> m_document;
    QPointer<RootItem> m_root;

    QPointer<TextBrowserImageDownloader> m_imageDownloader;
    QPointer<QThread> m_imageDownloadThread;
    QHash<QUrl, QImage> m_downloadedImages;
};

#endif // TEXTBROWSERVIEWER_H
