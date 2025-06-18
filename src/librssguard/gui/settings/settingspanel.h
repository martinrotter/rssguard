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
    virtual QIcon icon() const = 0;
    virtual void loadSettings() = 0;
    virtual void saveSettings() = 0;
    virtual void loadUi();

    bool requiresRestart() const;
    bool isDirty() const;

    void setIsDirty(bool is_dirty);
    void setRequiresRestart(bool requiresRestart);

    bool isLoaded() const;
    bool uiLoaded() const;

    int numberOfMatches() const;
    void setNumberOfMatches(int nmbr);
    void incrementNumberOfMatches();

  protected:
    void onBeginLoadSettings();
    void onEndLoadSettings();
    void onBeginSaveSettings();
    void onEndSaveSettings();

    // Settings to use to save/load.
    Settings* settings() const;

  protected slots:
    void dirtifySettings();
    void requireRestart();

  signals:
    void settingsChanged();

  private:
    bool m_requiresRestart;
    bool m_isDirty;
    bool m_isLoading;
    bool m_isLoaded;
    bool m_uiLoaded;
    Settings* m_settings;
    int m_numberOfMatches;
};

#endif // SETTINGSPANEL_H
