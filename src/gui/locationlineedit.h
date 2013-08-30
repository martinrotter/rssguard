#ifndef LOCATIONLINEEDIT_H
#define LOCATIONLINEEDIT_H

#include "gui/baselineedit.h"


class WebBrowser;

class LocationLineEdit : public BaseLineEdit {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit LocationLineEdit(QWidget *parent = 0);
    virtual ~LocationLineEdit();

  public slots:
    // Sets percentual value of web page loading action.
    // NOTE: Number ranging from 0 to 100 is expected.
    void setProgress(int progress);
    void clearProgress();

  protected:
    void focusOutEvent(QFocusEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);

  private:
    int m_progress;
    QPalette m_defaultPalette;
    bool m_mouseSelectsAllText;
};

#endif // LOCATIONLINEEDIT_H
