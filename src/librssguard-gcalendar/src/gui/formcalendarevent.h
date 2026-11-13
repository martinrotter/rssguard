// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMCALENDAREVENT_H
#define FORMCALENDAREVENT_H

#include "src/calendareventdraft.h"

#include "ui_formcalendarevent.h"

#include <optional>

#include <QDialog>
#include <QPointer>

class GCalendarServiceRoot;

class FormCalendarEvent : public QDialog {
    Q_OBJECT

  public:
    explicit FormCalendarEvent(GCalendarServiceRoot* account,
                               const Message* message = nullptr,
                               const QString& calendar_id = QString(),
                               QWidget* parent = nullptr);

  public slots:
    virtual void reject() override;

  signals:
    void eventStored(int message_id);

  protected:
    virtual void closeEvent(QCloseEvent* event) override;

  private:
    enum class Pending {
      None,
      Save,
      Series,
      Reload,
      Reapply
    };
    CalendarEventDraft& draft();
    void loadForm();
    void setStatus(WidgetWithStatus::StatusType status, const QString& text);
    void readForm();
    void updateDateControls(bool loading = false);
    void setBusy(bool busy);
    void save();
    void selectTarget(int target);
    void reload(bool reapply);
    void finish(quint64 id,
                const QJsonObject& resource,
                int status,
                const QString& error,
                bool confirmed,
                int message_id,
                bool recovered_creation);

    Ui::FormCalendarEvent m_ui;
    QPointer<GCalendarServiceRoot> m_account;
    CalendarEventDraft m_occurrence;
    std::optional<CalendarEventDraft> m_seriesDraft;
    std::optional<CalendarEventDraft> m_creationAttempt;
    QString m_calendarId;
    QString m_seriesId;
    int m_messageId = 0;
    int m_loadedRecurrence = 0;
    int m_loadedCount = 0;
    QJsonArray m_loadedRules;
    quint64 m_request = 0;
    Pending m_pending = Pending::None;
    bool m_series = false;
    bool m_loading = false;
    bool m_busy = false;
    bool m_creationCalendarLocked = false;
    bool m_creationUncertain = false;
};

#endif // FORMCALENDAREVENT_H
