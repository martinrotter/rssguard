// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TABCONTENT_H
#define TABCONTENT_H

#include <QWidget>

#if defined(USE_WEBENGINE)
class WebBrowser;
#endif

// Base class for all widgets which are placed inside tabs of TabWidget
class TabContent : public QWidget {
  Q_OBJECT

  public:
    explicit TabContent(QWidget* parent = nullptr);

    // Gets/sets current index of this TabContent.
    // NOTE: This is the index under which this object lies
    // in parent tab widget.
    virtual int index() const;
    virtual void setIndex(int index);

#if defined(USE_WEBENGINE)
    // Obtains instance contained in this TabContent or nullptr.
    // This can be used for obtaining the menu from the instance and so on.
    virtual WebBrowser* webBrowser() const = 0;
#endif

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
