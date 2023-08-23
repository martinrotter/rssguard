// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMEDITNEWSBLURACCOUNT_H
#define FORMEDITNEWSBLURACCOUNT_H

#include "services/abstract/gui/formaccountdetails.h"

class NewsBlurAccountDetails;
class NewsBlurServiceRoot;

class FormEditNewsBlurAccount : public FormAccountDetails {
  Q_OBJECT

  public:
    explicit FormEditNewsBlurAccount(QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  protected:
    virtual void loadAccountData();

  private slots:
    void performTest();

  private:
    NewsBlurAccountDetails* m_details;
};

#endif // FORMEDITNEWSBLURACCOUNT_H
