// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef LOCATIONLINEEDIT_H
#define LOCATIONLINEEDIT_H

#include "gui/reusable/baselineedit.h"

class WebBrowser;
class GoogleSuggest;

class LocationLineEdit : public BaseLineEdit {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit LocationLineEdit(QWidget* parent = nullptr);
    virtual ~LocationLineEdit();

  protected:
    void focusOutEvent(QFocusEvent* event);
    void mousePressEvent(QMouseEvent* event);

  private:
    bool m_mouseSelectsAllText;
    GoogleSuggest* m_googleSuggest;
};

#endif // LOCATIONLINEEDIT_H
