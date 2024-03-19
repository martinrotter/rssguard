// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMABOUT_H
#define FORMABOUT_H

#include "ui_formabout.h"

#include <QDialog>

class RSSGUARD_DLLSPEC FormAbout : public QDialog {
    Q_OBJECT

  public:
    explicit FormAbout(bool go_to_changelog, QWidget* parent);
    virtual ~FormAbout();

  private slots:
    void displayLicense();

  private:
    void loadLicenseAndInformation();
    void loadSettingsAndPaths();

    Ui::FormAbout m_ui;
};

#endif // FORMABOUT_H
