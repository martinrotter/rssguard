// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITNEXTCLOUDACCOUNT_H
#define FORMEDITNEXTCLOUDACCOUNT_H

#include <librssguard/services/abstract/gui/formaccountdetails.h>

class NextcloudAccountDetails;
class NextcloudServiceRoot;

class FormEditNextcloudAccount : public FormAccountDetails {
    Q_OBJECT

  public:
    explicit FormEditNextcloudAccount(QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  protected:
    virtual void loadAccountData();

  private slots:
    void performTest();

  private:
    NextcloudAccountDetails* m_details;
};

#endif // FORMEDITNEXTCLOUDACCOUNT_H
