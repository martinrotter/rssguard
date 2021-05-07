// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ADBLOCKDIALOG_H
#define ADBLOCKDIALOG_H

#include <QDialog>

#include "ui_adblockdialog.h"

class AdBlockManager;

class AdBlockDialog : public QDialog {
  Q_OBJECT

  public:
    explicit AdBlockDialog(QWidget* parent = nullptr);

  private slots:
    void saveAndClose();
    void enableAdBlock(bool enable);
    void learnAboutAdblock();

  private:
    void load();

  private:
    AdBlockManager* m_manager;
    bool m_loaded;

    Ui::AdBlockDialog m_ui;
};

#endif // ADBLOCKDIALOG_H
