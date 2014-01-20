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

    // TODO: napsat metodu setStatus(enum-statusu, qstring)
    // kde enum-statusu bude Ok, Warning, Error a qstring bude text kerej se objevi jako
    // tooltip na toolbuttonu
    // pak bude proste navazani na textEdited() a tam si bude uzivatel
    // widgetu nastavovat pres to setStatus co chce on

  private:
    BaseLineEdit *m_txtInput;
    PlainToolButton *m_btnStatus;
    QHBoxLayout *m_layout;

};

#endif // LINEEDITWITHSTATUS_H
