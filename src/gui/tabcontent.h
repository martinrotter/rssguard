#ifndef TABCONTENT_H
#define TABCONTENT_H

#include <QWidget>


class WebBrowser;

// Base class for all widgets which are placed inside tabs of TabWidget
class TabContent : public QWidget {
    Q_OBJECT

  public:
    // Contructors.
    explicit TabContent(QWidget *parent = 0);
    virtual ~TabContent();

    // Gets/sets current index of this TabContent.
    // NOTE: This is the index under which this object lies
    // in some TabWidget instance.
    virtual int index() const;
    virtual void setIndex(int index);

    // Obtains instance contained in this TabContent or nullptr.
    // This can be used for obtaining the menu from the instance and so on.
    virtual WebBrowser *webBrowser() = 0;

  private:
    int m_index;
};

#endif // TABCONTENT_H
