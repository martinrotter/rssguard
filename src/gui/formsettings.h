#ifndef FORMSETTINGS_H
#define FORMSETTINGS_H

#include <QDialog>

#include "ui_formsettings.h"


namespace Ui {
  class FormSettings;
}

// Structure holding some initial values.
struct TemporarySettings {

  public:
    TemporarySettings()
      : m_webBrowserProgress(QColor()),
        m_skin(QString()) {
    }

    QColor m_webBrowserProgress;
    QString m_skin;


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
    QStringList m_changedDataTexts;
};

#endif // FORMSETTINGS_H
