// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMGREADERFEEDDETAILS_H
#define FORMGREADERFEEDDETAILS_H

#include <librssguard/services/abstract/gui/formfeeddetails.h>

class GreaderFeed;
class GreaderFeedDetails;

class FormGreaderFeedDetails : public FormFeedDetails {
  public:
    explicit FormGreaderFeedDetails(ServiceRoot* service_root,
                                    RootItem* parent_to_select = nullptr,
                                    const QString& url = QString(),
                                    QWidget* parent = nullptr);

  protected slots:
    virtual void apply();

  private:
    virtual void loadFeedData();

  private:
    GreaderFeedDetails* m_feedDetails;
    RootItem* m_parentToSelect;
    QString m_urlToProcess;
};

#endif // FORMGREADERFEEDDETAILS_H
