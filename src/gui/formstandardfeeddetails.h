#ifndef FORMSTANDARDFEEDDETAILS_H
#define FORMSTANDARDFEEDDETAILS_H

#include <QDialog>

#include "ui_formstandardfeeddetails.h"


namespace Ui {
  class FormStandardFeedDetails;
}

class FeedsModel;
class FeedsModelStandardFeed;
class FeedsModelCategory;
class FeedsModelRootItem;

class FormStandardFeedDetails : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormStandardFeedDetails(FeedsModel *model, QWidget *parent = 0);
    virtual ~FormStandardFeedDetails();

  public slots:
    int exec(FeedsModelStandardFeed *input_feed);

  protected slots:
    // Trigerred when title/description changes.
    void onTitleChanged(const QString &new_title);
    void onDescriptionChanged(const QString &new_description);
    void onUrlChanged(const QString &new_url);

  protected:
    void createConnections();
    void setEditableFeed(FeedsModelStandardFeed *editable_feed);
    void initialize();

    // Loads categories into the dialog from the model.
    void loadCategories(const QList<FeedsModelCategory*> categories,
                        FeedsModelRootItem *root_item,
                        FeedsModelStandardFeed *input_feed);

  private:
    Ui::FormStandardFeedDetails *m_ui;
    FeedsModelStandardFeed *m_editableFeed;
    FeedsModel *m_feedsModel;

    QMenu *m_iconMenu;
    QAction *m_actionLoadIconFromFile;
    QAction *m_actionUseDefaultIcon;
    QAction *m_actionNoIcon;
};

#endif // FORMSTANDARDFEEDDETAILS_H
