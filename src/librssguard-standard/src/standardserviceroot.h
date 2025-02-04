// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDSERVICEROOT_H
#define STANDARDSERVICEROOT_H

#include "src/standardfeed.h"

#include <librssguard/services/abstract/serviceroot.h>

#include <QCoreApplication>
#include <QPair>

class StandardCategory;
class FeedsImportExportModel;
class QMenu;

class StandardServiceRoot : public ServiceRoot {
    Q_OBJECT

    friend class FormStandardFeedDetails;
    friend class FormStandardImportExport;

  public:
    explicit StandardServiceRoot(RootItem* parent = nullptr);
    virtual ~StandardServiceRoot();

    virtual FormAccountDetails* accountSetupDialog() const;
    virtual void onDatabaseCleanup();
    virtual void start(bool freshly_activated);
    virtual void stop();
    virtual QString code() const;
    virtual bool canBeEdited() const;
    virtual void editItems(const QList<RootItem*>& items);
    virtual bool supportsFeedAdding() const;
    virtual bool supportsCategoryAdding() const;
    virtual Qt::ItemFlags additionalFlags() const;
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual QList<Message> obtainNewMessages(Feed* feed,
                                             const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages);

    QList<QAction*> serviceMenu();
    QList<QAction*> getContextMenuForFeed(StandardFeed* feed);

    static QString defaultTitle();

  public slots:
    void addNewFeed(RootItem* selected_item, const QString& url = QString());
    void addNewCategory(RootItem* selected_item);

  private slots:
    void importFeeds();
    void exportFeeds();

  private:
    // Takes structure residing under given root item and adds feeds/categories from
    // it to active structure.
    // NOTE: This is used for import/export of the model.
    bool mergeImportExportModel(FeedsImportExportModel* model, RootItem* target_root_node, QString& output_message);

    QPointer<StandardFeed> m_feedForMetadata = {};
    QList<QAction*> m_feedContextMenu = {};
};

#endif // STANDARDSERVICEROOT_H
