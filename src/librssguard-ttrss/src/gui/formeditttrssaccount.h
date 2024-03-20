// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITACCOUNT_H
#define FORMEDITACCOUNT_H

#include <librssguard/services/abstract/gui/formaccountdetails.h>

class RootItem;
class TtRssServiceRoot;
class TtRssAccountDetails;

class FormEditTtRssAccount : public FormAccountDetails {
    Q_OBJECT

  public:
    explicit FormEditTtRssAccount(QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  protected:
    virtual void loadAccountData();

  private slots:
    void performTest();

  private:
    TtRssAccountDetails* m_details;
};

#endif // FORMEDITACCOUNT_H
