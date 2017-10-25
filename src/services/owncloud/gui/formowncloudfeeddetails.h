// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMOWNCLOUDFEEDDETAILS_H
#define FORMOWNCLOUDFEEDDETAILS_H

#include "services/abstract/gui/formfeeddetails.h"

class FormOwnCloudFeedDetails : public FormFeedDetails {
  Q_OBJECT

  public:
    explicit FormOwnCloudFeedDetails(ServiceRoot* service_root, QWidget* parent = 0);

  protected slots:
    void apply();

  protected:
    void setEditableFeed(Feed* editable_feed);
};

#endif // FORMOWNCLOUDFEEDDETAILS_H
