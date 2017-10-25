// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMSETTINGS_H
#define FORMSETTINGS_H

#include <QDialog>

#include "ui_formsettings.h"

class Settings;
class SettingsPanel;

class FormSettings : public QDialog {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit FormSettings(QWidget& parent);
    virtual ~FormSettings();

  private slots:

    // Saves settings into global configuration.
    void saveSettings();
    void applySettings();
    void cancelSettings();

  private:
    void addSettingsPanel(SettingsPanel* panel);

    Ui::FormSettings m_ui;
    QPushButton* m_btnApply;

    QList<SettingsPanel*> m_panels;
    Settings& m_settings;
};

#endif // FORMSETTINGS_H
