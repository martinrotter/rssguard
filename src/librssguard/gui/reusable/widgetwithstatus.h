// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef WIDGETWITHSTATUS_H
#define WIDGETWITHSTATUS_H

#include <QIcon>
#include <QWidget>

class PlainToolButton;
class QHBoxLayout;

class RSSGUARD_DLLSPEC WidgetWithStatus : public QWidget {
    Q_OBJECT

  public:
    enum class StatusType {
      Information,
      Warning,
      Error,
      Ok,
      Progress,
      Question
    };

    explicit WidgetWithStatus(QWidget* parent);

    void setStatus(StatusType status, const QString& tooltip_text);
    StatusType status() const;

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
    QIcon m_iconQuestion;
};

inline WidgetWithStatus::StatusType WidgetWithStatus::status() const {
  return m_status;
}

#endif // WIDGETWITHSTATUS_H
