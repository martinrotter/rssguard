// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMSSFEEDDETAILS_H
#define FORMSSFEEDDETAILS_H

#include "src/gui/feediconselection.h"
#include "src/parsers/feedparser.h"

#include <librssguard/services/abstract/gui/formfeeddetails.h>

class StandardFeedDetails;
class StandardFeedExpDetails;
class StandardFeedNetworkDetails;
class StandardFeed;

class FormStandardFeedDetails : public FormFeedDetails {
    Q_OBJECT

  public:
    explicit FormStandardFeedDetails(ServiceRoot* service_root,
                                     RootItem* parent_to_select = nullptr,
                                     const QString& url = QString(),
                                     QWidget* parent = nullptr);

  private slots:
    void guessFeed();
    void guessIconOnly();
    void onTitleChanged(const QString& title);
    void invalidateFetchedIcons();

    virtual void apply();

  private:
    virtual void loadFeedData();
    FeedIconRequest iconRequest(StandardFeed* feed) const;
    void initializeIconFetching();

  private:
    StandardFeedDetails* m_standardFeedDetails;
    StandardFeedExpDetails* m_standardFeedExpDetails;
    StandardFeedNetworkDetails* m_networkDetails;
    RootItem* m_parentToSelect;
    QString m_urlToProcess;
    FeedIconSelection m_iconSelection;
};

#endif // FORMSSFEEDDETAILS_H
