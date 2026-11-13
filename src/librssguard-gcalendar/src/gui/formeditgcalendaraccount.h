// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITGCALENDARACCOUNT_H
#define FORMEDITGCALENDARACCOUNT_H

#include <librssguard/services/abstract/gui/formaccountdetails.h>

class GCalendarAccountDetails;
class GCalendarReminderDetails;

class FormEditGCalendarAccount : public FormAccountDetails {
    Q_OBJECT

  public:
    explicit FormEditGCalendarAccount(QWidget* parent = nullptr);
    virtual ~FormEditGCalendarAccount() override;

  protected:
    virtual bool loadAccountData() override;
    virtual void apply() override;

  private:
    GCalendarAccountDetails* m_details;
    GCalendarReminderDetails* m_remindersDetails;
};

#endif // FORMEDITGCALENDARACCOUNT_H
