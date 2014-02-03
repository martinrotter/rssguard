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
    // Executes add/edit standard feed dialog.
    int exec(FeedsModelStandardFeed *input_feed);

  protected slots:
    // Applies changes.
    void apply();

    // Trigerred when title/description/url/username/password changes.
    void onTitleChanged(const QString &new_title);
    void onDescriptionChanged(const QString &new_description);
    void onUrlChanged(const QString &new_url);
    void onUsernameChanged(const QString &new_username);
    void onPasswordChanged(const QString &new_password);
    void onAuthenticationSwitched();
    void onAutoUpdateTypeChanged(int new_index);

    // Check if "OK" button can be enabled or not.
    void checkOkButtonEnabled();

    // Icon selectors.
    void onNoIconSelected();
    void onLoadIconFromFile();
    void onUseDefaultIcon();

  protected:
    // Creates needed connections.
    void createConnections();

    // Sets the feed which will be edited.
    void setEditableFeed(FeedsModelStandardFeed *editable_feed);

    // Initializes the dialog.
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
    QPushButton *m_btnLoadDataFromInternet;
};

#endif // FORMSTANDARDFEEDDETAILS_H
