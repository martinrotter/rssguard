// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TABCONTENT_H
#define TABCONTENT_H

#include <QWidget>

class WebBrowser;

// Base class for all widgets which are placed inside tabs of TabWidget
class TabContent : public QWidget {
    Q_OBJECT

  public:
    explicit TabContent(QWidget* parent = nullptr);
    virtual ~TabContent();

    // Gets/sets current index of this TabContent.
    // NOTE: This is the index under which this object lies
    // in parent tab widget.
    virtual int index() const;
    virtual void setIndex(int index);

    // Obtains instance contained in this TabContent or nullptr.
    // This can be used for obtaining the menu from the instance and so on.
    virtual WebBrowser* webBrowser() const = 0;

  protected:
    int m_index;
};

inline int TabContent::index() const {
  return m_index;
}

inline void TabContent::setIndex(int index) {
  m_index = index;
}

#endif // TABCONTENT_H
