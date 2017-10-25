// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WIDGETWITHSTATUS_H
#define WIDGETWITHSTATUS_H

#include <QIcon>
#include <QWidget>

class PlainToolButton;
class QHBoxLayout;

class WidgetWithStatus : public QWidget {
  Q_OBJECT

  public:
    enum StatusType {
      Information,
      Warning,
      Error,
      Ok,
      Progress
    };

    // Constructors and destructors.
    explicit WidgetWithStatus(QWidget* parent);
    virtual ~WidgetWithStatus();

    // Sets custom status for this control.
    void setStatus(StatusType status, const QString& tooltip_text);

    inline StatusType status() const {
      return m_status;
    }

  protected:
    StatusType m_status;
    QWidget* m_wdgInput;
    PlainToolButton* m_btnStatus;
    QHBoxLayout* m_layout;
    QIcon m_iconProgress;
    QIcon m_iconInformation;
    QIcon m_iconWarning;
    QIcon m_iconError;
    QIcon m_iconOk;
};

#endif // WIDGETWITHSTATUS_H
