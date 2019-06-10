// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SETTINGSPANEL_H
#define SETTINGSPANEL_H

#include <QWidget>

class Settings;

class SettingsPanel : public QWidget {
  Q_OBJECT

  public:
    explicit SettingsPanel(Settings* settings, QWidget* parent = nullptr);

    virtual QString title() const = 0;
    virtual void loadSettings() = 0;
    virtual void saveSettings() = 0;

    bool requiresRestart() const;
    bool isDirty() const;

    void setIsDirty(bool is_dirty);
    void setRequiresRestart(bool requiresRestart);

  protected:
    void onBeginLoadSettings();
    void onEndLoadSettings();
    void onBeginSaveSettings();
    void onEndSaveSettings();

    // Settings to use to save/load.
    Settings* settings() const;

  protected slots:

    // Sets this settings panel as dirty (some settings are changed) and emits the signal.
    // NOTE: This will be probably called by subclasses when user changes some stuff.
    void dirtifySettings();

    void requireRestart();

  signals:
    void settingsChanged();

  private:
    bool m_requiresRestart;
    bool m_isDirty;
    bool m_isLoading;
    Settings* m_settings;
};

#endif // SETTINGSPANEL_H
