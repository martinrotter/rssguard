// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMMESSAGEFILTERSMANAGER_H
#define FORMMESSAGEFILTERSMANAGER_H

#include <QDialog>

#include "ui_formmessagefiltersmanager.h"

class FormMessageFiltersManager : public QDialog {
    Q_OBJECT

  public:
    explicit FormMessageFiltersManager(QWidget *parent = nullptr);

  private:
    Ui::FormMessageFiltersManager m_ui;
};

#endif // FORMMESSAGEFILTERSMANAGER_H
