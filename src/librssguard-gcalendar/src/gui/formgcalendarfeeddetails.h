// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMGCALENDARFEEDDETAILS_H
#define FORMGCALENDARFEEDDETAILS_H

#include <librssguard/services/abstract/gui/formfeeddetails.h>

class GCalendarReminderDetails;

class FormGCalendarFeedDetails : public FormFeedDetails {
    Q_OBJECT

  public:
    explicit FormGCalendarFeedDetails(ServiceRoot* account, QWidget* parent = nullptr);

  protected:
    virtual void loadFeedData() override;
    virtual void apply() override;

  private:
    GCalendarReminderDetails* m_remindersDetails;
};

#endif // FORMGCALENDARFEEDDETAILS_H
