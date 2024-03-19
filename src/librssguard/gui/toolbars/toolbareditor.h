// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TOOLBAREDITOR_H
#define TOOLBAREDITOR_H

#include "ui_toolbareditor.h"

#include <QWidget>

namespace Ui {
  class ToolBarEditor;
}

class BaseBar;

class ToolBarEditor : public QWidget {
    Q_OBJECT

  public:
    explicit ToolBarEditor(QWidget* parent = nullptr);

    // Toolbar operations.
    void loadFromToolBar(BaseBar* tool_bar);
    void saveToolBar();

    QListWidget* activeItemsWidget() const;
    QListWidget* availableItemsWidget() const;

  protected:
    virtual bool eventFilter(QObject* object, QEvent* event);

  private slots:
    void updateActionsAvailability();

    // Insert common controls.
    void insertSpacer();
    void insertSeparator();

    void moveActionDown();
    void moveActionUp();

    void addSelectedAction();
    void deleteSelectedAction();
    void deleteAllActions();

    void resetToolBar();

  signals:
    void setupChanged();

  private:
    void loadEditor(const QList<QAction*>& activated_actions, const QList<QAction*>& available_actions);

    QScopedPointer<Ui::ToolBarEditor> m_ui;
    BaseBar* m_toolBar;
};

inline QListWidget* ToolBarEditor::activeItemsWidget() const {
  return m_ui->m_listActivatedActions;
}

inline QListWidget* ToolBarEditor::availableItemsWidget() const {
  return m_ui->m_listAvailableActions;
}

#endif // TOOLBAREDITOR_H
