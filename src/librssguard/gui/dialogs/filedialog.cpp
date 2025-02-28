// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/filedialog.h"

#include "miscellaneous/settings.h"

#include <QFileInfo>

QString FileDialog::existingDirectory(QWidget* parent,
                                      const QString& caption,
                                      const QString& dir,
                                      const QString& id,
                                      QFileDialog::Options options) {
  QString initial_dir_file = id.isEmpty() ? dir : qApp->settings()->value(GROUP(FileDialogPaths), id, dir).toString();
  QFileInfo initial_dir_file_info(initial_dir_file);
  QString fldr = QFileDialog::getExistingDirectory(parent,
                                                   caption.isEmpty() ? QObject::tr("Select existing folder") : caption,
                                                   initial_dir_file_info.absolutePath(),
                                                   options);

  if (!fldr.isEmpty() && !id.isEmpty()) {
    qApp->settings()->setValue(GROUP(FileDialogPaths), id, fldr);
  }

  return fldr;
}

QString FileDialog::saveFileName(QWidget* parent,
                                 const QString& caption,
                                 const QString& dir,
                                 const QString& filter,
                                 QString* selected_filter,
                                 const QString& id,
                                 QFileDialog::Options options) {
  QString initial_dir_file = id.isEmpty() ? dir : qApp->settings()->value(GROUP(FileDialogPaths), id, dir).toString();
  QString file = QFileDialog::getSaveFileName(parent,
                                              caption.isEmpty() ? QObject::tr("Save file") : caption,
                                              initial_dir_file,
                                              filter,
                                              selected_filter,
                                              options);

  if (!file.isEmpty() && !id.isEmpty()) {
    qApp->settings()->setValue(GROUP(FileDialogPaths), id, QFileInfo(file).absolutePath());
  }

  return file;
}

QString FileDialog::openFileName(QWidget* parent,
                                 const QString& caption,
                                 const QString& dir,
                                 const QString& filter,
                                 QString* selected_filter,
                                 const QString& id,
                                 QFileDialog::Options options) {
  QString initial_dir_file = id.isEmpty() ? dir : qApp->settings()->value(GROUP(FileDialogPaths), id, dir).toString();
  QString file = QFileDialog::getOpenFileName(parent,
                                              caption.isEmpty() ? QObject::tr("Select existing file") : caption,
                                              initial_dir_file,
                                              filter,
                                              selected_filter,
                                              options);

  if (!file.isEmpty() && !id.isEmpty()) {
    qApp->settings()->setValue(GROUP(FileDialogPaths), id, QFileInfo(file).absolutePath());
  }

  return file;
}
