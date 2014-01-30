#ifndef LINEEDITWITHSTATUS_H
#define LINEEDITWITHSTATUS_H

#include <QWidget>
#include <QIcon>


class BaseLineEdit;
class PlainToolButton;
class QHBoxLayout;

class LineEditWithStatus : public QWidget {
    Q_OBJECT

  public:
    enum StatusType {
      Information,
      Warning,
      Error,
      Ok
    };

    // Constructors and destructors.
    explicit LineEditWithStatus(QWidget *parent = 0);
    virtual ~LineEditWithStatus();

    // TODO: napsat metodu setStatus(enum-statusu, qstring)
    // kde enum-statusu bude Ok, Warning, Error a qstring bude text kerej se objevi jako
    // tooltip na toolbuttonu
    // pak bude proste navazani na textEdited() a tam si bude uzivatel
    // widgetu nastavovat pres to setStatus co chce on

    // Sets custom status for this control.
    void setStatus(StatusType status, const QString &tooltip_text);

    inline StatusType status() const {
      return m_status;
    }

    // Access to line edit.
    inline BaseLineEdit *lineEdit() const {
      return m_txtInput;
    }

  private:
    StatusType m_status;
    BaseLineEdit *m_txtInput;
    PlainToolButton *m_btnStatus;
    QHBoxLayout *m_layout;

    QIcon m_iconInformation;
    QIcon m_iconWarning;
    QIcon m_iconError;
    QIcon m_iconOk;
};

#endif // LINEEDITWITHSTATUS_H
