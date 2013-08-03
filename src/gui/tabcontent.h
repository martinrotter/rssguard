#ifndef TABCONTENT_H
#define TABCONTENT_H

#include <QWidget>


class WebBrowser;

// Base class for all widgets which are placed inside tabs of TabWidget
class TabContent : public QWidget {
    Q_OBJECT

  public:
    TabContent(QWidget *parent = 0);
    virtual ~TabContent();

    virtual int index() const;
    virtual void setIndex(int index);

    // Obtains instance contained in this TabContent or nullptr.
    // This is used for obtaining the menu from the instance and so on.
    virtual WebBrowser *webBrowser() = 0;

  private:
    int m_index;
};

#endif // TABCONTENT_H
