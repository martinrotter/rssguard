#ifndef BASEWEBVIEW_H
#define BASEWEBVIEW_H

#include <QWebView>


class QPaintEvent;
class BaseWebPage;

class BaseWebView : public QWebView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit BaseWebView(QWidget *parent = 0);
    virtual ~BaseWebView();

  protected slots:
    // Executes if loading of any page is done.
    void onLoadFinished(bool ok);

  protected:
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
};

#endif // BASEWEBVIEW_H
