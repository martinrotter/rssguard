#ifndef WEBVIEWER_H
#define WEBVIEWER_H

#include "core/message.h"

#include <QUrl>

class WebBrowser;
class RootItem;

// Interface for web/article viewers.
class WebViewer {
  public:
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

    // Loads URL into the viewer.
    virtual void setUrl(const QUrl& url) = 0;

    // Set static HTML into the viewer.
    virtual void setHtml(const QString& html, const QUrl& base_url = {}) = 0;

    // Returns current static HTML loaded in the viewer.
    virtual QString html() const = 0;

    // Returns current URL.
    virtual QUrl url() const = 0;

    // Clears displayed URL.
    virtual void clear() = 0;

    // Displays all messages and ensures that vertical scrollbar is set to 0 (scrolled to top).
    virtual void loadMessages(const QList<Message>& messages, RootItem* root) = 0;

    // Vertical scrollbar changer.
    virtual double verticalScrollBarPosition() const = 0;
    virtual void setVerticalScrollBarPosition(double pos) = 0;

    // Apply font.
    virtual void applyFont(const QFont& fon) = 0;

    // Zooming.
    virtual bool canZoomIn() const = 0;
    virtual bool canZoomOut() const = 0;
    virtual void zoomIn() = 0;
    virtual void zoomOut() = 0;
    virtual qreal zoomFactor() const = 0;
    virtual void setZoomFactor(qreal zoom_factor) = 0;
};

inline WebViewer::~WebViewer() {}

#endif // WEBVIEWER_H
