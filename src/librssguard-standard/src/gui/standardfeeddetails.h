// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDFEEDDETAILS_H
#define STANDARDFEEDDETAILS_H

#include "src/standardfeed.h"

#include "ui_standardfeeddetails.h"

#include <QNetworkProxy>
#include <QWidget>

class Category;
class RootItem;

class StandardFeedDetails : public QWidget {
    Q_OBJECT

    friend class FormStandardFeedDetails;

  public:
    explicit StandardFeedDetails(QWidget* parent = nullptr);

    StandardFeed::SourceType sourceType() const;

  private slots:
    void guessIconOnly(StandardFeed::SourceType source_type,
                       const QString& source,
                       const QString& post_process_script,
                       StandardServiceRoot* account,
                       NetworkFactory::NetworkAuthentication protection,
                       const QString& username,
                       const QString& password,
                       const QList<QPair<QByteArray, QByteArray>>& headers = {},
                       const QNetworkProxy& custom_proxy = QNetworkProxy::ProxyType::DefaultProxy);

    void guessFeed(StandardFeed::SourceType source_type,
                   const QString& source,
                   const QString& post_process_script,
                   StandardServiceRoot* account,
                   NetworkFactory::NetworkAuthentication protection,
                   const QString& username,
                   const QString& password,
                   const QList<QPair<QByteArray, QByteArray>>& headers = {},
                   const QNetworkProxy& custom_proxy = QNetworkProxy::ProxyType::DefaultProxy);

    void onTitleChanged(const QString& new_title);
    void onDescriptionChanged(const QString& new_description);
    void onUrlChanged(const QString& new_url);
    void onPostProcessScriptChanged(const QString& new_pp);
    void onLoadIconFromFile();
    void onUseDefaultIcon();

  private:
    void prepareForNewFeed(RootItem* parent_to_select, const QString& url);
    void setExistingFeed(StandardFeed* feed);
    void loadCategories(const QList<Category*>& categories, RootItem* root_item);

  private:
    Ui::StandardFeedDetails m_ui;
    QMenu* m_iconMenu{};
    QAction* m_actionLoadIconFromFile{};
    QAction* m_actionUseDefaultIcon{};
    QAction* m_actionFetchIcon{};
};

#endif // STANDARDFEEDDETAILS_H
