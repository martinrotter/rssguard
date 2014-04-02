#ifndef TOOLBAREDITOR_H
#define TOOLBAREDITOR_H

#include <QDialog>

#include "ui_toolbareditor.h"


namespace Ui {
  class ToolBarEditor;
}

class BaseToolBar;

// TODO: dialog pro úpravu prirazeneho toolbaru.
class ToolBarEditor : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit ToolBarEditor(QWidget *parent = 0);
    virtual ~ToolBarEditor();

    // Toolbar operations.
    void loadFromToolBar(BaseToolBar *tool_bar);
    void saveToolBar();

  private:
    Ui::ToolBarEditor *m_ui;
    BaseToolBar *m_toolBar;
};

#endif // TOOLBAREDITOR_H
