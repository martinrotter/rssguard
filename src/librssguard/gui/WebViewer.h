#ifndef WEBVIEWER_H
#define WEBVIEWER_H

#include "core/message.h"

class WebBrowser;
class RootItem;

// Abstract class to define interface for web viewers.
class WebViewer {
  public:

    // Performs necessary steps to make viewer work with browser.
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

    // Displays all messages;
    virtual void loadMessages(const QList<Message>& messages, RootItem* root) = 0;

    // Vertical scrollbar changer.
    virtual double verticalScrollBarPosition() const = 0;
    virtual void setVerticalScrollBarPosition(double pos) = 0;

    // Apply font.
    virtual void reloadFontSettings(const QFont& fon) = 0;

    // Zooming.
    virtual bool canZoomIn() const = 0;
    virtual bool canZoomOut() const = 0;
    virtual qreal zoomFactor() const = 0;
    virtual void zoomIn() = 0;
    virtual void zoomOut() = 0;
    virtual void setZoomFactor(qreal zoom_factor) = 0;
};

#endif // WEBVIEWER_H
