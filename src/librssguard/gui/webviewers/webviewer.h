// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WEBVIEWER_H
#define WEBVIEWER_H

#include "core/message.h"
#include "definitions/definitions.h"

#include <QAction>
#include <QPrinter>
#include <QUrl>

class QPrinter;
class QMenu;
class QContextMenuEvent;

class WebBrowser;
class RootItem;

struct ContextMenuData {
    QUrl m_linkUrl;
    QUrl m_imgLinkUrl;
    QString m_selectedText;
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

    // Called when network settings are changed app-wide.
    virtual void reloadNetworkSettings() = 0;

    // Perform inline search.
    // NOTE: When text is empty, cancel search.
    virtual void findText(const QString& text, bool backwards) = 0;

    // Asynchronously loads url.
    // If URL is non-valid, clears the view.
    virtual void loadUrl(const QUrl& url) = 0;

    // Set static HTML into the viewer.
    virtual void setHtml(const QString& html, const QUrl& url = {}, RootItem* root = nullptr) = 0;

    // Returns current static HTML (or plain text) loaded in the viewer.
    virtual QString html() const = 0;
    virtual QString plainText() const = 0;

    virtual bool supportsNavigation() const = 0;
    virtual bool supportImagesLoading() const = 0;

    // Reloads page if there is some URL set.
    virtual void reloadPage() = 0;

    virtual void goBack() = 0;
    virtual void goForward() = 0;

    // Returns current URL.
    virtual QUrl url() const = 0;

    // Clears displayed data.
    virtual void clear() = 0;

    virtual void cleanupCache() = 0;

    // Printing.
    virtual void printToPrinter(QPrinter* printer) = 0;

    // Displays message and ensure that vertical scrollbar is set to 0 (scrolled to top).
    virtual void loadMessage(const Message& message, RootItem* root) = 0;

    // Specifies or tweaks the recommended URL of the message.
    virtual QUrl urlForMessage(const Message& message, RootItem* root) const;

    // Returns final HTML generated for the articles.
    virtual QString htmlForMessage(const Message& messages, RootItem* root) const = 0;

    // Enables/disables loading of remote resources like images etc.
    virtual bool loadExternalResources() const;
    virtual void setLoadExternalResources(bool load_resources);

    // Vertical scrollbar changer.
    virtual double verticalScrollBarPosition() const = 0;
    virtual void setVerticalScrollBarPosition(double pos) = 0;

    // Called after printing is finished, must be reimplemented by view
    // and this super-implementation called.
    virtual void onPrintingFinished(bool success);

    // Apply font.
    virtual void applyFont(const QFont& fon) = 0;

    // Appends specific items to context menu.
    virtual void processContextMenu(QMenu* specific_menu, QContextMenuEvent* event);

    // Zooming.
    virtual bool canZoomIn() const;
    virtual bool canZoomOut() const;
    virtual void zoomIn();
    virtual void zoomOut();
    virtual qreal zoomFactor() const = 0;
    virtual void setZoomFactor(qreal zoom_factor) = 0;

  signals:
    virtual void reloadPageEnabledChanged(bool can_go_forward) = 0;
    virtual void goBackEnabledChanged(bool can_go_back) = 0;
    virtual void goForwardEnabledChanged(bool can_go_forward) = 0;
    virtual void pageTitleChanged(const QString& new_title) = 0;
    virtual void pageUrlChanged(const QUrl& url) = 0;
    virtual void pageIconChanged(const QIcon&) = 0;
    virtual void linkMouseHighlighted(const QUrl& url) = 0;
    virtual void linkMouseClicked(const QUrl& url) = 0;
    virtual void loadingStarted() = 0;
    virtual void loadingProgress(int progress) = 0;
    virtual void loadingFinished(bool success) = 0;
    virtual void openUrlInNewTab(bool open_externally, const QUrl& url) = 0;
    virtual void openViewerInNewTab(WebViewer* viewer) = 0;

  protected:
    virtual ContextMenuData provideContextMenuData(QContextMenuEvent* event) = 0;

  private:
    void copySelectedText();
    void copySelectedLink();
    void copySelectedImage();
    void copySelectedImageLink();
    void saveImageAs();
    void saveHtmlAs();
    void playClickedLinkAsMedia();
    void openClickedLinkInExternalBrowser();
    void openClickedLinkInNewTab();
    void printContents();
    void initializeCommonMenuItems();

  protected:
    ContextMenuData m_contextMenuData;
    QPixmap m_placeholderImage;
    QPixmap m_placeholderImageError;

  private:
    bool m_loadExternalResources;

    QScopedPointer<QAction> m_actionExternalResources;
    QScopedPointer<QAction> m_actionPrint;
    QScopedPointer<QAction> m_actionSaveHtml;
    QScopedPointer<QAction> m_actionOpenNewTab;
    QScopedPointer<QAction> m_actionOpenExternalBrowser;
    QScopedPointer<QAction> m_actionPlayLink;
    QScopedPointer<QAction> m_actionCopyText;
    QScopedPointer<QAction> m_actionCopyLink;
    QScopedPointer<QAction> m_actionCopyImage;
    QScopedPointer<QAction> m_actionSaveImage;
    QScopedPointer<QAction> m_actionCopyImageLink;

    QScopedPointer<QPrinter> m_printer;
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
