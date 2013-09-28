#ifndef BASEWEBVIEW_H
#define BASEWEBVIEW_H

#include <QWebView>


class QAction;
class QPaintEvent;
class BaseWebPage;

class BaseWebView : public QWebView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit BaseWebView(QWidget *parent = 0);
    virtual ~BaseWebView();

    void setupIcons();

  signals:
    // Is emitted if user wants to open some hyperlink in new
    // web browser tab.
    void linkMiddleClicked(const QUrl &link_url);

    // User wants to open new empty web browser tab.
    void newTabRequested();

  public slots:
    void increaseWebPageZoom();
    void decreaseWebPageZoom();
    void resetWebPageZoom();

  protected slots:
    // Executes if loading of any page is done.
    void onLoadFinished(bool ok);

    void openLinkInNewTab();
    void openImageInNewTab();

    // Provides custom context menu.
    void popupContextMenu(const QPoint &pos);

  protected:
    void initializeActions();

    // Creates necessary connections.
    void createConnections();

    // Displays custom error page.
    void displayErrorPage();

    // Does additional painting.
    void paintEvent(QPaintEvent *event);

    // Provides custom mouse actions.
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

  private:
    BaseWebPage *m_page;

    QAction *m_actionReload;
    QAction *m_actionCopyLink;
    QAction *m_actionCopyImage;
    QAction *m_actionCopyImageUrl;
    QAction *m_actionOpenLinkThisTab;
    QAction *m_actionOpenLinkNewTab;
    QAction *m_actionOpenImageNewTab;

    QPoint m_gestureOrigin;
    QUrl m_contextLinkUrl;
    QUrl m_contextImageUrl;
};

#endif // BASEWEBVIEW_H
