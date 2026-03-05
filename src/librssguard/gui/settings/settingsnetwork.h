// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSNETWORK_H
#define SETTINGSNETWORK_H

#include "gui/settings/settingspanel.h"

#include "ui_settingsnetwork.h"

class NetworkProxyDetails;

class SettingsNetwork : public SettingsPanel {
    Q_OBJECT

  public:
    explicit SettingsNetwork(Settings* settings, QWidget* parent = nullptr);
    virtual ~SettingsNetwork();

    virtual QIcon icon() const;
    virtual QString title() const;
    virtual void loadSettings();
    virtual void saveSettings();
    virtual void loadUi();

  private:
    NetworkProxyDetails* m_proxyDetails;
    Ui::SettingsNetwork* m_ui;
};

inline QString SettingsNetwork::title() const {
  return tr("Network");
}

#endif // SETTINGSNETWORK_H
