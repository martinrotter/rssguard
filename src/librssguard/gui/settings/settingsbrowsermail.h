// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSBROWSERMAIL_H
#define SETTINGSBROWSERMAIL_H

#include "gui/settings/settingspanel.h"

#include "miscellaneous/externaltool.h"

#include "ui_settingsbrowsermail.h"

class NetworkProxyDetails;

class SettingsBrowserMail : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsBrowserMail(Settings* settings, QWidget* parent = nullptr);
    virtual ~SettingsBrowserMail();

    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();

  private slots:
    void addExternalTool();
    void editSelectedExternalTool();
    void deleteSelectedExternalTool();
    void changeDefaultBrowserArguments(int index);
    void selectBrowserExecutable();
    void changeDefaultEmailArguments(int index);
    void selectEmailExecutable();

  private:
    ExternalTool tweakExternalTool(const ExternalTool& tool) const;

  private:
    QVector<ExternalTool> externalTools() const;
    void setExternalTools(const QList<ExternalTool>& list);

    NetworkProxyDetails* m_proxyDetails;
    Ui::SettingsBrowserMail* m_ui;
};

inline QString SettingsBrowserMail::title() const {
  return tr("Network & web & tools");
}

#endif // SETTINGSBROWSERMAIL_H
