// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSNODEJS_H
#define SETTINGSNODEJS_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsnodejs.h"

class SettingsNodejs : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsNodejs(Settings* settings, QWidget* parent = nullptr);

    virtual QIcon icon() const;
    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();

  private slots:
    void testNodejs();
    void testNpm();
    void testPackageFolder();

    void changeFileFolder(LineEditWithStatus* tb, bool directory_select, const QString& file_filter = {});

  private:
    Ui::SettingsNodejs m_ui;
};

#endif // SETTINGSNODEJS_H
