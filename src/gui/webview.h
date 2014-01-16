#ifndef BASEWEBVIEW_H
#define BASEWEBVIEW_H

#include <QWebView>


class QAction;
class QPaintEvent;
class WebPage;

class WebView : public QWebView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit WebView(QWidget *parent = 0);
    virtual ~WebView();

    // Page accessor.
    inline WebPage *page() const {
      return m_page;
    }

    void setupIcons();

  signals:
    // Is emitted if user wants to open some hyperlink in new
    // web browser tab.
    void linkMiddleClicked(const QUrl &link_url);

    // User wants to open new empty web browser tab.
    void newTabRequested();

    // Emitted if user changes zoom factor via CTRL + mouse wheel combo.
    void zoomFactorChanged();

  public slots:
    // Page zoom modifiers.
    bool increaseWebPageZoom();
    bool decreaseWebPageZoom();
    bool resetWebPageZoom();

    // Executes if loading of any page is done.
    void onLoadFinished(bool ok);

    void openLinkInNewTab();
    void openImageInNewTab();

    // Provides custom context menu.
    void popupContextMenu(const QPoint &pos);

  protected:
    // Initializes all actions.
    void initializeActions();

    // Creates necessary connections.
    void createConnections();

    // Displays custom error page.
    void displayErrorPage();

    // Customize mouse wheeling.
    void wheelEvent(QWheelEvent *event);

    // Does additional painting.
    void paintEvent(QPaintEvent *event);

    // Provides custom mouse actions.
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

  private:
    WebPage *m_page;

    QAction *m_actionReload;
    QAction *m_actionCopyLink;
    QAction *m_actionCopyImage;

#if QT_VERSION >= 0x040800
    QAction *m_actionCopyImageUrl;
#endif

    QAction *m_actionOpenLinkThisTab;
    QAction *m_actionOpenLinkNewTab;
    QAction *m_actionOpenImageNewTab;

    QPoint m_gestureOrigin;
    QUrl m_contextLinkUrl;
    QUrl m_contextImageUrl;
};

#endif // BASEWEBVIEW_H
