// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMCATEGORYDETAILS_H
#define FORMCATEGORYDETAILS_H

#include "miscellaneous/qtlinq.h"
#include "definitions/definitions.h"

#include <QDialog>

namespace Ui {
  class FormCategoryDetails;
}

class Category;
class ServiceRoot;
class FeedsModel;
class RootItem;
class QMenu;
class QAction;
class MultiFeedEditCheckBox;

class RSSGUARD_DLLSPEC FormCategoryDetails : public QDialog {
    Q_OBJECT

  public:
    explicit FormCategoryDetails(ServiceRoot* service_root,
                                 RootItem* parent_to_select = nullptr,
                                 QWidget* parent = nullptr);
    virtual ~FormCategoryDetails();

    template <class T>
    QList<T*> addEditCategory(const QList<Category*>& cats_to_edit = {});

    template <class T>
    T* category() const;

    // Returns all cats.
    template <class T>
    QList<T*> categories() const;

  protected:
    bool isChangeAllowed(MultiFeedEditCheckBox* mcb) const;
    virtual void loadCategoryData();

  protected slots:
    virtual void apply();

  private slots:

    // Trigerred when title/description changes.
    void onTitleChanged(const QString& new_title);
    void onDescriptionChanged(const QString& new_description);

    // Icon selectors.
    void onLoadIconFromFile();
    void onUseDefaultIcon();

  private:
    void createConnections();
    void initialize();

    // Loads categories into the dialog + give root "category"
    // and make sure that no childs of input category (including)
    // input category are loaded.
    void loadCategories(const QList<Category*>& categories, RootItem* root_item, Category* input_category);

  private:
    QScopedPointer<Ui::FormCategoryDetails> m_ui;
    QList<Category*> m_categories;
    ServiceRoot* m_serviceRoot;
    QMenu* m_iconMenu{};
    QAction* m_actionLoadIconFromFile{};
    QAction* m_actionUseDefaultIcon{};
    RootItem* m_parentToSelect;
    bool m_creatingNew;
    bool m_isBatchEdit;
};

template <class T>
inline QList<T*> FormCategoryDetails::addEditCategory(const QList<Category*>& cats_to_edit) {
  m_creatingNew = cats_to_edit.isEmpty();
  m_isBatchEdit = cats_to_edit.size() > 1;

  if (m_creatingNew) {
    m_categories.append(new T());
  }
  else {
    m_categories.append(cats_to_edit);
  }

  loadCategoryData();

  if (exec() == QDialog::DialogCode::Accepted) {
    return categories<T>();
  }
  else {
    return {};
  }
}

template <class T>
inline T* FormCategoryDetails::category() const {
  return qobject_cast<T*>(m_categories.first());
}

template <class T>
inline QList<T*> FormCategoryDetails::categories() const {
  std::list<T*> std_cats = qlinq::from(m_categories)
                             .select([](Category* fd) {
                               return qobject_cast<T*>(fd);
                             })
                             .toStdList();

  return FROM_STD_LIST(QList<T*>, std_cats);
}

#endif // FORMCATEGORYDETAILS_H
