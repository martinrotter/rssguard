// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMRESTOREDATABASESETTINGS_H
#define FORMRESTOREDATABASESETTINGS_H

#include <QDialog>

#include "ui_formrestoredatabasesettings.h"

class FormRestoreDatabaseSettings : public QDialog {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit FormRestoreDatabaseSettings(QWidget& parent);
    virtual ~FormRestoreDatabaseSettings();

    bool shouldRestart() const {
      return m_shouldRestart;
    }

  private slots:
    void performRestoration();
    void checkOkButton();
    void selectFolderWithGui();
    void selectFolder(QString folder = QString());

  private:
    Ui::FormRestoreDatabaseSettings m_ui;
    QPushButton* m_btnRestart;
    bool m_shouldRestart;
};

#endif // FORMRESTOREDATABASESETTINGS_H
