// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMBACKUPDATABASECONFIG_H
#define FORMBACKUPDATABASECONFIG_H

#include <QDialog>

#include "ui_formbackupdatabasesettings.h"

class FormBackupDatabaseSettings : public QDialog {
  Q_OBJECT

  public:
    explicit FormBackupDatabaseSettings(QWidget* parent = nullptr);
    virtual ~FormBackupDatabaseSettings();

  protected:
    virtual void hideEvent(QHideEvent* event);

  private slots:
    void performBackup();
    void selectFolderInitial();
    void selectFolder(QString path = QString());
    void checkBackupNames(const QString& name);
    void checkOkButton();

  private:
    QScopedPointer<Ui::FormBackupDatabaseSettings> m_ui;
};

#endif // FORMBACKUPDATABASECONFIG_H
