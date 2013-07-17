#ifndef LOCATIONLINEEDIT_H
#define LOCATIONLINEEDIT_H

#include "gui/baselineedit.h"


class WebBrowser;

class LocationLineEdit : public BaseLineEdit {
  public:
    explicit LocationLineEdit(QWidget *parent = 0);
    virtual ~LocationLineEdit();

  public slots:
    // Sets percentual value of web page loading action.
    // NOTE: Number ranging from 0 to 100 is expected.
    void setProgress(int progress);

  protected:
    void focusOutEvent(QFocusEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);

  private:
    int m_progress;
    bool m_mouseSelectsAllText;
};

#endif // LOCATIONLINEEDIT_H
