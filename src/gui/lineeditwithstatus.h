#ifndef LINEEDITWITHSTATUS_H
#define LINEEDITWITHSTATUS_H

#include <QWidget>


class BaseLineEdit;
class PlainToolButton;
class QHBoxLayout;

class LineEditWithStatus : public QWidget {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit LineEditWithStatus(QWidget *parent = 0);
    virtual ~LineEditWithStatus();



  private:
    BaseLineEdit *m_txtInput;
    PlainToolButton *m_btnStatus;
    QHBoxLayout *m_layout;

};

#endif // LINEEDITWITHSTATUS_H
