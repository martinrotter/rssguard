// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBVIEWER_H
#define WEBVIEWER_H

#include "core/message.h"
#include "definitions/definitions.h"

#include <QAction>
#include <QUrl>

class QMenu;
class QContextMenuEvent;

class WebBrowser;
class RootItem;

struct PreparedHtml {
    QString m_html;
    QUrl m_baseUrl;
};

struct ContextMenuData {
    QUrl m_linkUrl;
    QUrl m_mediaUrl;
};

#define ACCEPTABLE_IMAGE_PERCENTUAL_WIDTH 0.97

// Interface for web/article viewers.
class WebViewer {
  public:
    WebViewer();
    virtual ~WebViewer();

    // Performs necessary steps to make viewer work with browser.
    // NOTE: Each implementor must do this in this method:
    //   1. Initialize all WebBrowser QActions and maintain their "enabled" state all the time.
    //   2. Connect to all slots of WebBrowser to ensure updating of title/icon, notifications
    //      of loading start/progress/finish, link highlight etc.
    //   3. Viewer must set WebBrowser to be event filter at some point.
    virtual void bindToBrowser(WebBrowser* browser) = 0;

    // Perform inline search.
    // NOTE: When text is empty, cancel search.
    virtual void findText(const QString& text, bool backwards) = 0;

    // Set static HTML into the viewer.
    virtual void setHtml(const QString& html, const QUrl& url = {}) = 0;

    // Returns current static HTML loaded in the viewer.
    virtual QString html() const = 0;

    // Returns current URL.
    virtual QUrl url() const = 0;

    // Clears displayed data.
    virtual void clear() = 0;

    // Displays all messages and ensures that vertical scrollbar is set to 0 (scrolled to top).
    virtual void loadMessages(const QList<Message>& messages, RootItem* root) = 0;

    // Returns final HTML generated for the articles.
    virtual PreparedHtml htmlForMessages(const QList<Message>& messages, RootItem* root) const = 0;

    // Vertical scrollbar changer.
    virtual double verticalScrollBarPosition() const = 0;
    virtual void setVerticalScrollBarPosition(double pos) = 0;

    // Apply font.
    virtual void applyFont(const QFont& fon) = 0;

    // Zooming.
    virtual bool canZoomIn() const;
    virtual bool canZoomOut() const;
    virtual void zoomIn();
    virtual void zoomOut();
    virtual qreal zoomFactor() const = 0;
    virtual void setZoomFactor(qreal zoom_factor) = 0;

  signals:
    virtual void pageTitleChanged(const QString& new_title) = 0;
    virtual void pageUrlChanged(const QUrl& url) = 0;
    virtual void pageIconChanged(const QIcon&) = 0;
    virtual void linkMouseHighlighted(const QUrl& url) = 0;
    virtual void linkClicked(const QUrl& url) = 0;
    virtual void loadingStarted() = 0;
    virtual void loadingProgress(int progress) = 0;
    virtual void loadingFinished(bool success) = 0;

  protected:
    void processContextMenu(QMenu* specific_menu, QContextMenuEvent* event);

    virtual ContextMenuData provideContextMenuData(QContextMenuEvent* event) const = 0;

  private:
    void playClickedLinkAsMedia();
    void openClickedLinkInExternalBrowser();
    void initializeCommonMenuItems();

  private:
    QScopedPointer<QAction> m_actionOpenExternalBrowser;
    QScopedPointer<QAction> m_actionPlayLink;
    ContextMenuData m_contextMenuData;
};

Q_DECLARE_INTERFACE(WebViewer, "WebViewer")

inline void WebViewer::zoomIn() {
  setZoomFactor(zoomFactor() + double(ZOOM_FACTOR_STEP));
}

inline void WebViewer::zoomOut() {
  setZoomFactor(zoomFactor() - double(ZOOM_FACTOR_STEP));
}

inline bool WebViewer::canZoomIn() const {
  return zoomFactor() <= double(MAX_ZOOM_FACTOR) - double(ZOOM_FACTOR_STEP);
}

inline bool WebViewer::canZoomOut() const {
  return zoomFactor() >= double(MIN_ZOOM_FACTOR) + double(ZOOM_FACTOR_STEP);
}

#endif // WEBVIEWER_H
