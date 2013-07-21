#ifndef BASEWEBVIEW_H
#define BASEWEBVIEW_H

#include <QWebView>
#include <QAction>


class QPaintEvent;
class BaseWebPage;

class BaseWebView : public QWebView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit BaseWebView(QWidget *parent = 0);
    virtual ~BaseWebView();

    void setupIcons();

  protected slots:
    // Executes if loading of any page is done.
    void onLoadFinished(bool ok);

  protected:
    void initializeActions();

    // Creates necessary connections.
    void createConnections();

    // Displays custom error page.
    void displayErrorPage();

    // Does additional painting.
    void paintEvent(QPaintEvent *event);

    // Provides custom context menu.
    void contextMenuEvent(QContextMenuEvent *event);

  private:
    BaseWebPage *m_page;

    QAction *m_actionReload;
    QAction *m_actionCopyLink;
    QAction *m_actionCopyImage;
    QAction *m_actionCopyImageUrl;
};

#endif // BASEWEBVIEW_H
