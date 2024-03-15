// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TEXTBROWSERVIEWER_H
#define TEXTBROWSERVIEWER_H

#include "gui/webviewers/webviewer.h"

#include <QTextBrowser>

#include "network-web/adblock/adblockmanager.h"

#include <QNetworkReply>
#include <QPixmap>
#include <QPointer>
#include <QTimer>

class QContextMenuEvent;
class QResizeEvent;
class WebBrowser;
class Downloader;

class TextBrowserViewer;

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

  public:
    explicit TextBrowserViewer(QWidget* parent = nullptr);
    virtual ~TextBrowserViewer();

    QVariant loadOneResource(int type, const QUrl& name);

    virtual QSize sizeHint() const;
    virtual void bindToBrowser(WebBrowser* browser);
    virtual void findText(const QString& text, bool backwards);
    virtual void setUrl(const QUrl& url);
    virtual void setHtml(const QString& html, const QUrl& base_url = {});
    virtual void setReadabledHtml(const QString& html, const QUrl& base_url = {});
    virtual QString html() const;
    virtual QUrl url() const;
    virtual void clear();
    virtual void loadMessages(const QList<Message>& messages, RootItem* root);
    virtual double verticalScrollBarPosition() const;
    virtual void setVerticalScrollBarPosition(double pos);
    virtual void applyFont(const QFont& fon);
    virtual qreal zoomFactor() const;
    virtual void setZoomFactor(qreal zoom_factor);

    bool resourcesEnabled() const;
    void setResourcesEnabled(bool enabled);

  protected:
    virtual ContextMenuData provideContextMenuData(QContextMenuEvent* event) const;

    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

  private slots:
    void enableResources(bool enable);
    void downloadLink();
    void onAnchorClicked(const QUrl& url);
    void reloadHtmlDelayed();
    void downloadNextNeededResource();
    void resourceDownloaded(const QUrl& url,
                            QNetworkReply::NetworkError status,
                            int http_code,
                            const QByteArray& contents = QByteArray());

  signals:
    void reloadDocument();
    void pageTitleChanged(const QString& new_title);
    void pageUrlChanged(const QUrl& url);
    void pageIconChanged(const QIcon&);
    void linkMouseHighlighted(const QUrl& url);
    void loadingStarted();
    void loadingProgress(int progress);
    void loadingFinished(bool success);
    void newWindowRequested(WebViewer* viewer);
    void closeWindowRequested();

  private:
    PreparedHtml prepareLegacyHtmlForMessage(const QList<Message>& messages, RootItem* selected_item) const;

    void setHtmlPrivate(const QString& html, const QUrl& base_url);
    BlockingResult blockedWithAdblock(const QUrl& url);

    QString decodeHtmlData(const QByteArray& data, const QString& content_type) const;

  private:
    QScopedPointer<Downloader> m_downloader;
    bool m_resourcesEnabled;
    QList<QUrl> m_neededResources; // All URLs here must be resolved.
    Downloader* m_resourceDownloader;
    QThread* m_resourceDownloaderThread;
    QMap<QUrl, QByteArray> m_loadedResources; // All URLs here must be resolved.
    QPixmap m_placeholderImage;
    QPixmap m_placeholderImageError;
    QUrl m_currentUrl;
    QString m_currentHtml;

    QPointer<RootItem> m_root;
    QFont m_baseFont;
    qreal m_zoomFactor = 1.0;
    QScopedPointer<QAction> m_actionEnableResources;
    QScopedPointer<QAction> m_actionDownloadLink;
    QScopedPointer<TextBrowserDocument> m_document;
    QPoint m_lastContextMenuPos;
};

#endif // TEXTBROWSERVIEWER_H
