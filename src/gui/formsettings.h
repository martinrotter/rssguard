#ifndef FORMSETTINGS_H
#define FORMSETTINGS_H

#include <QDialog>

#include "ui_formsettings.h"


namespace Ui {
  class FormSettings;
}

// Structure holding some initial values.
struct TemporarySettings {
    QColor m_webBrowserProgress;
};

class FormSettings : public QDialog {
    Q_OBJECT
    
  public:
    explicit FormSettings(QWidget *parent = 0);
    virtual ~FormSettings();

  protected:
    bool doSaveCheck();

  protected slots:
    // Saves settings into global configuration.
    void saveSettings();

    void loadInterface();
    void saveInterface();
    void changeBrowserProgressColor();
    void onSkinSelected(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    void loadGeneral();
    void saveGeneral();

    void loadLanguage();
    void saveLanguage();

    void loadShortcuts();
    void saveShortcuts();

    void loadBrowser();
    void saveBrowser();

    void loadProxy();
    void saveProxy();
    void displayProxyPassword(int state);

    void onProxyTypeChanged(int index);
    
  private:
    Ui::FormSettings *m_ui;
    TemporarySettings m_initialSettings;
};

#endif // FORMSETTINGS_H
