// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TEXTBROWSERVIEWER_H
#define TEXTBROWSERVIEWER_H

#include "gui/webviewers/webviewer.h"

#include <QTextBrowser>

#include "network-web/adblock/adblockmanager.h"

#include <QPointer>

class QContextMenuEvent;
class QResizeEvent;
class WebBrowser;
class Downloader;

class TextBrowserDocument : public QTextDocument {
    Q_OBJECT

    friend class TextBrowserViewer;

  public:
    explicit TextBrowserDocument(QObject* parent = nullptr);

  protected:
    virtual QVariant loadResource(int type, const QUrl& name);

  private:
    bool m_reloadingWithResources;
    QList<QUrl> m_neededResourcesForHtml;
    QMap<QUrl, QByteArray> m_loadedResources;
};

class TextBrowserViewer : public QTextBrowser, public WebViewer {
    Q_OBJECT
    Q_INTERFACES(WebViewer)

  public:
    explicit TextBrowserViewer(QWidget* parent = nullptr);

    virtual QSize sizeHint() const;

  public:
    virtual void bindToBrowser(WebBrowser* browser);
    virtual void findText(const QString& text, bool backwards);
    virtual void setUrl(const QUrl& url);
    virtual void setHtml(const QString& html, const QUrl& base_url = {});
    virtual QString html() const;
    virtual QUrl url() const;
    virtual void clear();
    virtual void loadMessages(const QList<Message>& messages, RootItem* root);
    virtual double verticalScrollBarPosition() const;
    virtual void setVerticalScrollBarPosition(double pos);
    virtual void applyFont(const QFont& fon);
    virtual qreal zoomFactor() const;
    virtual void setZoomFactor(qreal zoom_factor);

  protected:
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

  private slots:
    void reloadWithImages();
    void openLinkInExternalBrowser();
    void downloadLink();
    void onAnchorClicked(const QUrl& url);

  signals:
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
    void setHtmlPrivate(const QString& html, const QUrl& base_url);

    BlockingResult blockedWithAdblock(const QUrl& url);
    QScopedPointer<Downloader> m_downloader;
    QPair<QString, QUrl> prepareHtmlForMessage(const QList<Message>& messages, RootItem* selected_item) const;

  private:
    QUrl m_currentUrl;
    QPointer<RootItem> m_root;
    QFont m_baseFont;
    qreal m_zoomFactor = 1.0;
    QScopedPointer<QAction> m_actionReloadWithImages;
    QScopedPointer<QAction> m_actionOpenExternalBrowser;
    QScopedPointer<QAction> m_actionDownloadLink;
    QScopedPointer<TextBrowserDocument> m_document;
    QPoint m_lastContextMenuPos;
};

#endif // TEXTBROWSERVIEWER_H
