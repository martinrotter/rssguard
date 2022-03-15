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

    template<class T>
    T* addEditFeed(T* feed_to_edit = nullptr);

    template<class T>
    T* feed() const;

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
    virtual void loadFeedData();

  private slots:
    void acceptIfPossible();
    void onAutoUpdateTypeChanged(int new_index);

  private:
    void createConnections();
    void initialize();

  protected:
    QScopedPointer<Ui::FormFeedDetails> m_ui;
    Feed* m_feed;
    ServiceRoot* m_serviceRoot;
    bool m_creatingNew;
};

template<class T>
inline T* FormFeedDetails::addEditFeed(T* feed_to_edit) {
  m_creatingNew = feed_to_edit == nullptr;

  if (m_creatingNew) {
    m_feed = new T();
  }
  else {
    m_feed = feed_to_edit;
  }

  // Load custom logic for feed data loading.
  loadFeedData();

  if (exec() == QDialog::DialogCode::Accepted) {
    return feed<T>();
  }
  else {
    return nullptr;
  }
}

template<class T>
inline T* FormFeedDetails::feed() const {
  return qobject_cast<T*>(m_feed);
}

#endif // FORMFEEDDETAILS_H
