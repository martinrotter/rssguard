// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMABOUT_H
#define FORMABOUT_H

#include <QDialog>

#include "ui_formabout.h"

class RSSGUARD_DLLSPEC FormAbout : public QDialog {
  Q_OBJECT

  public:
    explicit FormAbout(QWidget* parent);
    virtual ~FormAbout();

  private:
    void loadLicenseAndInformation();
    void loadSettingsAndPaths();

    Ui::FormAbout m_ui;
};

#endif // FORMABOUT_H
