#ifndef WEBBROWSER_H
#define WEBBROWSER_H

#include <QWidget>
#include <QPointer>
#include <QUrl>

#include "core/messagesmodel.h"
#include "gui/tabcontent.h"


class QToolBar;
class QToolButton;
class QWidgetAction;
class QVBoxLayout;
class LocationLineEdit;
class BaseWebView;
class WebBrowserNetworkAccessManager;
class QMenu;
class TabWidget;

class WebBrowser : public TabContent {
    Q_OBJECT
    
  public:
    // Constructors and destructors.
    explicit WebBrowser(QWidget *parent = 0);
    ~WebBrowser();

    // Reloads icons for all buttons.
    void setupIcons();

    // Returns icon associated with currently loaded website.
    QIcon icon();

    // Sets this WebBrowser instance as focused.
    void setFocus(Qt::FocusReason reason);

    // Returns this instance.
    // NOTE: This is needed due to TabContent interface.
    WebBrowser *webBrowser();

    // Returns global menu for this web browser.
    virtual QList<QAction*> globalMenu();

    // Returns pointer to global network access manager
    // for web browsers.
    // NOTE: All web browsers use shared network access manager,
    // which makes setting of custom network settings easy.
    static WebBrowserNetworkAccessManager *globalNetworkManager();

    // Returns list of all running web browsers.
    static QList<WebBrowser*> runningWebBrowsers();

  public slots:
    // Switches visibility of navigation bar.
    void setNavigationBarVisible(bool visible);

    // Loads new url into the web browser.
    void navigateToUrl(const QString &url);
    void navigateToUrl(const QUrl &url);

    // Navigates to message.
    void navigateToMessage();

    // Zoom manipulators.
    void increaseZoom();
    void decreaseZoom();
    void resetZoom();

  protected:
    // Creates necessary connections.
    void createConnections();

    // Initializes all buttons and widgets, which are needed for "Zoom" menu item.
    void initializeZoomWidget();

  protected slots:
    // Updates zoom-related gui.
    void updateZoomGui();

    // Updates url (for example on location text box).
    void updateUrl(const QUrl &url);

    void onTitleChanged(const QString &new_title);
    void onIconChanged();

  signals:
    void newTabRequested();
    void linkMiddleClicked(const QUrl &link_url);

    void iconChanged(int index, const QIcon &icon);
    void titleChanged(int index, const QString &title);

  private:
    QVBoxLayout *m_layout;
    QToolBar *m_toolBar;
    BaseWebView *m_webView;
    LocationLineEdit *m_txtLocation;
    QWidget *m_zoomButtons;
    QToolButton *m_btnResetZoom;

    QWidgetAction *m_actionZoom;
    QAction *m_actionBack;
    QAction *m_actionForward;
    QAction *m_actionReload;
    QAction *m_actionStop;

    static QPointer<WebBrowserNetworkAccessManager> m_networkManager;
    static QList<WebBrowser*> m_runningWebBrowsers;
};

#endif // WEBBROWSER_H
