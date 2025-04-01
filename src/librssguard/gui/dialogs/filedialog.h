// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILEDIALOG_H
#define FILEDIALOG_H

#include <QFileDialog>
#include <QString>

#define GENERAL_REMEMBERED_PATH QSL("general")

class QWidget;

class RSSGUARD_DLLSPEC FileDialog {
  public:
    static QString storedFolder(const QString& id, const QString& dir = QString());

    static QString saveFileName(QWidget* parent = nullptr,
                                const QString& caption = QString(),
                                const QString& dir = QString(),
                                const QString& filter = QString(),
                                QString* selected_filter = nullptr,
                                const QString& id = QString(),
                                QFileDialog::Options options = QFileDialog::Options());

    static QString existingDirectory(QWidget* parent = nullptr,
                                     const QString& caption = QString(),
                                     const QString& dir = QString(),
                                     const QString& id = QString(),
                                     QFileDialog::Options options = QFileDialog::Option::ShowDirsOnly);

    static QString openFileName(QWidget* parent = nullptr,
                                const QString& caption = QString(),
                                const QString& dir = QString(),
                                const QString& filter = QString(),
                                QString* selected_filter = nullptr,
                                const QString& id = QString(),
                                QFileDialog::Options options = QFileDialog::Options());

  private:
    FileDialog() {}
};

#endif // FILEDIALOG_H
