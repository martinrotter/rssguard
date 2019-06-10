// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TOOLBAREDITOR_H
#define TOOLBAREDITOR_H

#include <QWidget>

#include "ui_toolbareditor.h"

namespace Ui {
  class ToolBarEditor;
}

class BaseBar;

class ToolBarEditor : public QWidget {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit ToolBarEditor(QWidget* parent = 0);
    virtual ~ToolBarEditor();

    // Toolbar operations.
    void loadFromToolBar(BaseBar* tool_bar);
    void saveToolBar();

    inline QListWidget* activeItemsWidget() const {
      return m_ui->m_listActivatedActions;
    }

    inline QListWidget* availableItemsWidget() const {
      return m_ui->m_listAvailableActions;
    }

  protected:
    bool eventFilter(QObject* object, QEvent* event);

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
    void loadEditor(const QList<QAction*> activated_actions, const QList<QAction*> available_actions);

    QScopedPointer<Ui::ToolBarEditor> m_ui;
    BaseBar* m_toolBar;
};

#endif // TOOLBAREDITOR_H
