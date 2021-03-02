// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMCATEGORYDETAILS_H
#define FORMCATEGORYDETAILS_H

#include "ui_formcategorydetails.h"

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

class FormCategoryDetails : public QDialog {
  Q_OBJECT

  public:
    explicit FormCategoryDetails(ServiceRoot* service_root, RootItem* parent_to_select = nullptr, QWidget* parent = nullptr);
    virtual ~FormCategoryDetails();

    template<class T>
    T* addEditCategory(T* category_to_edit = nullptr);

    template<class T>
    T* category() const;

  protected:
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
    Category* m_category;
    ServiceRoot* m_serviceRoot;
    QMenu* m_iconMenu{};
    QAction* m_actionLoadIconFromFile{};
    QAction* m_actionUseDefaultIcon{};
    RootItem* m_parentToSelect;
    bool m_creatingNew;
};

template<class T>
inline T* FormCategoryDetails::addEditCategory(T* category_to_edit) {
  m_creatingNew = category_to_edit == nullptr;

  if (m_creatingNew) {
    m_category = new T();
  }
  else {
    m_category = category_to_edit;
  }

  loadCategoryData();

  if (exec() == QDialog::DialogCode::Accepted) {
    return category<T>();
  }
  else {
    return nullptr;
  }
}

template<class T>
inline T* FormCategoryDetails::category() const {
  return qobject_cast<T*>(m_category);
}

#endif // FORMCATEGORYDETAILS_H
