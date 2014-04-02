#ifndef TOOLBAREDITOR_H
#define TOOLBAREDITOR_H

#include <QDialog>

#include "ui_toolbareditor.h"

namespace Ui {
  class ToolBarEditor;
}

// TODO: dialog pro úpravu prirazeneho toolbaru.
class ToolBarEditor : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit ToolBarEditor(QWidget *parent = 0);
    virtual ~ToolBarEditor();

  private:
    Ui::ToolBarEditor *m_ui;
};

#endif // TOOLBAREDITOR_H
