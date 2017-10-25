// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMDOWNLOADATTACHMENT_H
#define FORMDOWNLOADATTACHMENT_H

#include <QDialog>

#include "ui_formdownloadattachment.h"

namespace Ui {
  class FormDownloadAttachment;
}

class Downloader;

class FormDownloadAttachment : public QDialog {
  Q_OBJECT

  public:
    explicit FormDownloadAttachment(const QString& target_file, Downloader* downloader, QWidget* parent = nullptr);

  private:
    Ui::FormDownloadAttachment m_ui;
};

#endif // FORMDOWNLOADATTACHMENT_H
