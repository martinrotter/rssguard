// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMFEEDDETAILS_H
#define FORMFEEDDETAILS_H

#include <QDialog>

#include "ui_formfeeddetails.h"

namespace Ui {
  class FormFeedDetails;
}

class ServiceRoot;
class Feed;
class Category;
class RootItem;

class FormFeedDetails : public QDialog {
  Q_OBJECT

  public:
    explicit FormFeedDetails(ServiceRoot* service_root, QWidget* parent = nullptr);
    virtual ~FormFeedDetails() = default;

  public slots:
    int editBaseFeed(Feed* input_feed);

  protected slots:
    void activateTab(int index);
    void clearTabs();

    // Applies changes.
    // NOTE: This must be reimplemented in subclasses. Also this
    // base implementation must be called first.
    virtual void apply();

  protected:
    void insertCustomTab(QWidget* custom_tab, const QString& title, int index);

    // Sets the feed which will be edited.
    // NOTE: This must be reimplemented in subclasses. Also this
    // base implementation must be called first.
    void virtual setEditableFeed(Feed* editable_feed);

  private slots:
    void onAutoUpdateTypeChanged(int new_index);

  private:
    void createConnections();
    void initialize();

  protected:
    QScopedPointer<Ui::FormFeedDetails> m_ui;
    Feed* m_editableFeed;
    ServiceRoot* m_serviceRoot;
};

#endif // FORMFEEDDETAILS_H
