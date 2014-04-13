#include "updater/formupdater.h"

#include "definitions/definitions.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QProcess>


FormUpdater::FormUpdater(QWidget *parent)
  : QMainWindow(parent),
    m_state(NoState),
    m_txtOutput(new QTextEdit(this)) {

  m_txtOutput->setReadOnly(true);
  m_txtOutput->setFocusPolicy(Qt::NoFocus);

  setCentralWidget(m_txtOutput);
  setWindowTitle("RSS Guard updater");
  setWindowIcon(QIcon(APP_ICON_PATH));
  moveToCenterAndResize();
}

FormUpdater::~FormUpdater() {
}

void FormUpdater::startUpgrade() {
  m_txtOutput->append("Welcome to RSS Guard updater.");

  if (QApplication::arguments().size() != 5) {
    m_txtOutput->append("Insufficient arguments passed. Update process cannot proceed.");
    m_txtOutput->append("Press any key to exit updater...");
    m_state = ExitError;
    // Ted je nastavenej state a pri keyPressEvent se appka ukonci
  }

  // do datovejch memberu teto tridy ulozit argumenty a pokracovat
}

void FormUpdater::keyPressEvent(QKeyEvent* event) {
  event->ignore();

  switch (m_state) {
    case NoState:
      break;

    case ExitNormal:
      break;

    case ExitError:
      qApp->quit();
      break;

    default:
      break;
  }
}

void FormUpdater::moveToCenterAndResize() {
  resize(500, 400);
  move(qApp->desktop()->screenGeometry().center() - rect().center());
}

bool FormUpdater::removeDirectory(const QString& directory_name,
                                  const QStringList& exception_file_list,
                                  const QStringList& exception_folder_list) {
  bool result = true;
  QDir dir(directory_name);

  if (dir.exists(directory_name)) {
    foreach (QFileInfo info,
             dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
      if (info.isDir()) {
        if (!exception_folder_list.contains(info.fileName())) {
          result &= removeDirectory(info.absoluteFilePath(), exception_file_list);
        }
      }
      else if (!exception_file_list.contains(info.fileName())) {
        result &= QFile::remove(info.absoluteFilePath());
      }
    }

    result &= dir.rmdir(directory_name);
  }

  return result;
}

bool FormUpdater::copyDirectory(QString source, QString destination) {
  QDir dir(source);

  if (! dir.exists()) {
    return false;
  }

  foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    QString dst_path = destination + QDir::separator() + d;
    dir.mkpath(dst_path);
    copyDirectory(source + QDir::separator() + d, dst_path);
  }

  foreach (QString f, dir.entryList(QDir::Files)) {
    QString original_file = source + QDir::separator() + f;
    QString destination_file = destination + QDir::separator() + f;

    if (!QFile::exists(destination_file) || QFile::remove(destination_file)) {
      if (QFile::copy(original_file, destination_file)) {
        qDebug("Copied file %s", qPrintable(f));
      }
      else {
        qDebug("Failed to copy file %s", qPrintable(original_file));
      }
    }
    else {
      qDebug("Failed to remove file %s", qPrintable(original_file));
    }
  }

  return true;
}
