// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/filedialog.h"

#include "miscellaneous/settings.h"

#include <QFileInfo>

QString FileDialog::existingDirectory(QWidget* parent,
                                      const QString& caption,
                                      const QString& dir,
                                      const QString& id,
                                      QFileDialog::Options options) {
  QString initial_dir = storedFolder(id, dir);
  QFileInfo initial_dir_info(initial_dir);
  QString fldr = QFileDialog::getExistingDirectory(parent,
                                                   caption.isEmpty() ? QObject::tr("Select existing folder") : caption,
                                                   initial_dir_info.absolutePath(),
                                                   options);

  if (!fldr.isEmpty() && !id.isEmpty()) {
    qApp->settings()->setValue(GROUP(FileDialogPaths), id, fldr);
  }

  return fldr;
}

QString FileDialog::storedFolder(const QString& id, const QString& dir) {
  return id.isEmpty() ? dir : qApp->settings()->value(GROUP(FileDialogPaths), id, dir).toString();
}

QString FileDialog::saveFileName(QWidget* parent,
                                 const QString& caption,
                                 const QString& dir,
                                 const QString& file_name,
                                 const QString& filter,
                                 QString* selected_filter,
                                 const QString& id,
                                 QFileDialog::Options options) {
  QString initial_dir = storedFolder(id, dir);
  QString file =
    QFileDialog::getSaveFileName(parent,
                                 caption.isEmpty() ? QObject::tr("Save file") : caption,
                                 file_name.isEmpty() ? initial_dir : initial_dir + QDir::separator() + file_name,
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
                                 const QString& file_name,
                                 const QString& filter,
                                 QString* selected_filter,
                                 const QString& id,
                                 QFileDialog::Options options) {
  QString initial_dir = storedFolder(id, dir);
  QString file =
    QFileDialog::getOpenFileName(parent,
                                 caption.isEmpty() ? QObject::tr("Select existing file") : caption,
                                 file_name.isEmpty() ? initial_dir : initial_dir + QDir::separator() + file_name,
                                 filter,
                                 selected_filter,
                                 options);

  if (!file.isEmpty() && !id.isEmpty()) {
    auto pth = QFileInfo(file).absolutePath();
    qApp->settings()->setValue(GROUP(FileDialogPaths), id, pth);
  }

  return file;
}
