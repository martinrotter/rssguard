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
    // in parent tab widget.
    inline virtual int index() const {
      return m_index;
    }

    inline virtual void setIndex(int index) {
      m_index = index;
    }

    // Obtains instance contained in this TabContent or nullptr.
    // This can be used for obtaining the menu from the instance and so on.
    virtual WebBrowser *webBrowser() = 0;

  protected:
    int m_index;
};

#endif // TABCONTENT_H
