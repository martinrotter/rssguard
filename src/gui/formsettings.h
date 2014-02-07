#ifndef FORMSETTINGS_H
#define FORMSETTINGS_H

#include "ui_formsettings.h"

#include <QDialog>


namespace Ui {
  class FormSettings;
}

// Structure holding some initial values.
struct TemporarySettings {

  public:
    TemporarySettings()
      : m_webBrowserProgress(QColor()) {
    }

    QColor m_webBrowserProgress;
};

class FormSettings : public QDialog {
    Q_OBJECT
    
  public:
    // Constructors and destructors.
    explicit FormSettings(QWidget *parent = 0);
    virtual ~FormSettings();

  protected:
    // Does check of controls before dialog can be submitted.
    bool doSaveCheck();

  protected slots:
    // Displays "restart" dialog if some critical settings changed.
    void promptForRestart();

    // Saves settings into global configuration.
    void saveSettings();

    void loadInterface();
    void saveInterface();
    void changeBrowserProgressColor();

    // Loads QColor instance into given button.
    void loadWebBrowserColor(const QColor &color);

    void onSkinSelected(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    void loadGeneral();
    void saveGeneral();

    void loadDataStorage();
    void saveDataStorage();

    void loadLanguage();
    void saveLanguage();

    void loadShortcuts();
    void saveShortcuts();

    void loadBrowser();
    void saveBrowser();
    void changeDefaultBrowserArguments(int index);
    void selectBrowserExecutable();

    void loadProxy();
    void saveProxy();
    void displayProxyPassword(int state);
    void onProxyTypeChanged(int index);

    void loadFeedsMessages();
    void saveFeedsMessages();
    
  private:
    Ui::FormSettings *m_ui;
    TemporarySettings m_initialSettings;
    QStringList m_changedDataTexts;
};

#endif // FORMSETTINGS_H
