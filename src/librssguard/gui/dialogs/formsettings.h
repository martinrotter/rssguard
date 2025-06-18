// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMSETTINGS_H
#define FORMSETTINGS_H

#include "ui_formsettings.h"

#include <QDialog>

class Settings;
class SettingsPanel;

class FormSettings : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormSettings(QWidget& parent);
    virtual ~FormSettings();

  public slots:
    void reject();

  private slots:
    void findInSettings(const QString& find);
    void openSettingsCategory(int category);

    // Saves settings into global configuration.
    void saveSettings();
    void applySettings();
    void cancelSettings();

  private:
    SettingsPanel* getSettingsPanel(QWidget* child) const;
    void addSettingsPanel(SettingsPanel* panel);

    void redrawPanelsList();

    Ui::FormSettings m_ui;
    QPushButton* m_btnApply;
    QList<SettingsPanel*> m_panels;
    Settings& m_settings;
};

#endif // FORMSETTINGS_H
