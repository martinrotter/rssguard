// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMTTRSSFEEDDETAILS_H
#define FORMTTRSSFEEDDETAILS_H

#include "services/abstract/gui/formfeeddetails.h"

class TtRssFeed;
class TtRssFeedDetails;
class AuthenticationDetails;

class FormTtRssFeedDetails : public FormFeedDetails {
  public:
    explicit FormTtRssFeedDetails(ServiceRoot* service_root, RootItem* parent_to_select = nullptr,
                                  const QString& url = QString(), QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  private:
    virtual void loadFeedData();

  private:
    TtRssFeedDetails* m_feedDetails;
    AuthenticationDetails* m_authDetails;
    RootItem* m_parentToSelect;
    QString m_urlToProcess;
};

#endif // FORMTTRSSFEEDDETAILS_H
