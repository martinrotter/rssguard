// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMRESTOREDATABASESETTINGS_H
#define FORMRESTOREDATABASESETTINGS_H

#include "ui_formrestoredatabasesettings.h"

#include <QDialog>

class FormRestoreDatabaseSettings : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormRestoreDatabaseSettings(QWidget& parent);
    virtual ~FormRestoreDatabaseSettings();

  private slots:
    void performRestoration();
    void checkOkButton();
    void selectFolderWithGui();
    void selectFolder(QString folder = QString());

  private:
    Ui::FormRestoreDatabaseSettings m_ui;
};

#endif // FORMRESTOREDATABASESETTINGS_H
