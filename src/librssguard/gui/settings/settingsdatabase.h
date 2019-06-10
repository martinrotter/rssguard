// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSDATABASE_H
#define SETTINGSDATABASE_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsdatabase.h"

class SettingsDatabase : public SettingsPanel {
  Q_OBJECT

  public:
    explicit SettingsDatabase(Settings* settings, QWidget* parent = 0);
    virtual ~SettingsDatabase();

    inline QString title() const {
      return tr("Data storage");
    }

    void loadSettings();

    void saveSettings();

  private:
    void mysqlTestConnection();
    void onMysqlHostnameChanged(const QString& new_hostname);
    void onMysqlUsernameChanged(const QString& new_username);
    void onMysqlPasswordChanged(const QString& new_password);
    void onMysqlDatabaseChanged(const QString& new_database);
    void selectSqlBackend(int index);
    void switchMysqlPasswordVisiblity(bool visible);

    Ui::SettingsDatabase* m_ui;
};

#endif // SETTINGSDATABASE_H
