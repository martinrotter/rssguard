#ifndef WEBBROWSER_H
#define WEBBROWSER_H

#include <QWidget>


class QToolBar;
class QVBoxLayout;
class LocationLineEdit;
class BaseWebView;
class WebBrowserNetworkAccessManager;

class WebBrowser : public QWidget {
    Q_OBJECT
    
  public:
    // Constructors and destructors.
    explicit WebBrowser(QWidget *parent = 0);
    ~WebBrowser();

    // Reloads icons for all buttons.
    void setupIcons();

    // Returns pointer to global network access manager
    // for web browsers.
    // NOTE: All web browsers use shared network access manager,
    // which makes setting of custom network settings easy.
    static WebBrowserNetworkAccessManager *globalNetworkManager();

    // Returns list of all running web browsers.
    static QList<WebBrowser*> runningWebBrowsers();

  protected:
    // Creates necessary connections.
    void createConnections();

  protected slots:
    // Updates url (for example on location text box).
    void updateUrl(const QUrl &url);

    // Loads new url into the web browser.
    void navigateToUrl(const QString &url);

  private:
    QVBoxLayout *m_layout;
    QToolBar *m_toolBar;
    BaseWebView *m_webView;
    LocationLineEdit *m_txtLocation;

    QAction *m_actionBack;
    QAction *m_actionForward;
    QAction *m_actionReload;
    QAction *m_actionStop;

    static QPointer<WebBrowserNetworkAccessManager> m_networkManager;
    static QList<WebBrowser*> m_runningWebBrowsers;
};

#endif // WEBBROWSER_H
