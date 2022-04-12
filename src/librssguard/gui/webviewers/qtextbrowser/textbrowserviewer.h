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

class TextBrowserViewer : public QTextBrowser, public WebViewer {
    Q_OBJECT
    Q_INTERFACES(WebViewer)

  public:
    explicit TextBrowserViewer(QWidget* parent = nullptr);

    virtual QVariant loadResource(int type, const QUrl& name);
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
    BlockingResult blockedWithAdblock(const QUrl& url);
    QScopedPointer<Downloader> m_downloader;
    QPair<QString, QUrl> prepareHtmlForMessage(const QList<Message>& messages, RootItem* selected_item) const;

  private:
    QUrl m_currentUrl;
    QPointer<RootItem> m_root;
    qreal m_zoomFactor = 1.0;
};

#endif // TEXTBROWSERVIEWER_H
