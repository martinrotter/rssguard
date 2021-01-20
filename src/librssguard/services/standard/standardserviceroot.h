// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDSERVICEROOT_H
#define STANDARDSERVICEROOT_H

#include "services/abstract/serviceroot.h"

#include "services/standard/standardfeed.h"

#include <QCoreApplication>
#include <QPair>

class StandardCategory;
class FeedsImportExportModel;
class QMenu;

class StandardServiceRoot : public ServiceRoot {
  Q_OBJECT

  public:
    explicit StandardServiceRoot(RootItem* parent = nullptr);
    virtual ~StandardServiceRoot();

    // Start/stop root.
    void start(bool freshly_activated);
    void stop();

    QString code() const;

    bool canBeEdited() const;
    bool canBeDeleted() const;
    bool editViaGui();
    bool deleteViaGui();
    bool supportsFeedAdding() const;
    bool supportsCategoryAdding() const;

    Qt::ItemFlags additionalFlags() const;

    // Returns menu to be shown in "Services -> service" menu.
    QList<QAction*> serviceMenu();

    // Returns context specific menu actions for given feed.
    QList<QAction*> getContextMenuForFeed(StandardFeed* feed);

    // Takes structure residing under given root item and adds feeds/categories from
    // it to active structure.
    // NOTE: This is used for import/export of the model.
    bool mergeImportExportModel(FeedsImportExportModel* model, RootItem* target_root_node, QString& output_message);

    QString processFeedUrl(const QString& feed_url);
    void loadFromDatabase();
    void checkArgumentForFeedAdding(const QString& argument);

  public slots:
    void addNewFeed(RootItem* selected_item, const QString& url = QString());
    void addNewCategory(RootItem* selected_item);
    void importFeeds();
    void exportFeeds();

  private:
    void checkArgumentsForFeedAdding();

    QPointer<StandardFeed> m_feedForMetadata = {};
    QList<QAction*> m_feedContextMenu = {};
};

#endif // STANDARDSERVICEROOT_H
