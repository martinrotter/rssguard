// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMFEEDDETAILS_H
#define FORMFEEDDETAILS_H

#include <QDialog>

#include "ui_formfeeddetails.h"

#include "3rd-party/boolinq/boolinq.h"
#include "definitions/definitions.h"

namespace Ui {
  class FormFeedDetails;
}

class ServiceRoot;
class Feed;
class Category;
class RootItem;

class RSSGUARD_DLLSPEC FormFeedDetails : public QDialog {
    Q_OBJECT

  public:
    explicit FormFeedDetails(ServiceRoot* service_root, QWidget* parent = nullptr);
    virtual ~FormFeedDetails() = default;

    template <class T>
    QList<T*> addEditFeed(const QList<Feed*>& feeds_to_edit = {});

    // Returns first feed.
    template <class T>
    T* feed() const;

    // Returns all feeds.
    template <class T>
    QList<T*> feeds() const;

  protected slots:
    void activateTab(int index);
    void clearTabs();

    // Applies changes.
    // NOTE: This must be reimplemented in subclasses. Also this
    // base implementation must be called first.
    virtual void apply();

  protected:
    bool isChangeAllowed(MultiFeedEditCheckBox* mcb) const;
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
    Ui::FormFeedDetails m_ui;
    QList<Feed*> m_feeds;
    ServiceRoot* m_serviceRoot;
    bool m_creatingNew;
    bool m_isBatchEdit;
};

template <class T>
inline QList<T*> FormFeedDetails::addEditFeed(const QList<Feed*>& feeds_to_edit) {
  m_creatingNew = feeds_to_edit.isEmpty();
  m_isBatchEdit = feeds_to_edit.size() > 1;

  if (m_creatingNew) {
    m_feeds.append(new T());
  }
  else {
    m_feeds.append(feeds_to_edit);
  }

  loadFeedData();

  if (exec() == QDialog::DialogCode::Accepted) {
    return feeds<T>();
  }
  else {
    return {};
  }
}

template <class T>
inline T* FormFeedDetails::feed() const {
  return qobject_cast<T*>(m_feeds.first());
}

template <class T>
inline QList<T*> FormFeedDetails::feeds() const {
  std::list<T*> std_fds = boolinq::from(m_feeds)
                            .select([](Feed* fd) {
                              return qobject_cast<T*>(fd);
                            })
                            .toStdList();

  return FROM_STD_LIST(QList<T*>, std_fds);
}

#endif // FORMFEEDDETAILS_H
