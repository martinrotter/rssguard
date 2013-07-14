#ifndef WEBBROWSER_H
#define WEBBROWSER_H

#include <QWidget>


class QVBoxLayout;
class BaseNetworkAccessManager;

class WebBrowser : public QWidget {
    Q_OBJECT
    
  public:
    explicit WebBrowser(QWidget *parent = 0);
    ~WebBrowser();

    static BaseNetworkAccessManager *getNetworkManager();
    
  private:
    QVBoxLayout *m_layout;

    static QPointer<BaseNetworkAccessManager> m_networkManager;
};

#endif // WEBBROWSER_H
