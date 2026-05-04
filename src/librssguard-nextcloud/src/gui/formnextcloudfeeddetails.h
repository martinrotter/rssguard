// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMNEXTCLOUDFEEDDETAILS_H
#define FORMNEXTCLOUDFEEDDETAILS_H

#include <librssguard/services/abstract/gui/formfeeddetails.h>

class NextcloudFeed;
class NextcloudFeedDetails;
class AuthenticationDetails;

class FormNextcloudFeedDetails : public FormFeedDetails {
  public:
    explicit FormNextcloudFeedDetails(ServiceRoot* service_root,
                                      RootItem* parent_to_select = nullptr,
                                      const QString& url = QString(),
                                      QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  private:
    virtual void loadFeedData();

  private:
    NextcloudFeedDetails* m_feedDetails;
    RootItem* m_parentToSelect;
    QString m_urlToProcess;
};

#endif // FORMNEXTCLOUDFEEDDETAILS_H
