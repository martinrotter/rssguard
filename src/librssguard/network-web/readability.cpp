// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/readability.h"

#include "miscellaneous/application.h"
#include "miscellaneous/nodejs.h"

Readability::Readability(QObject* parent) : QObject{parent} {}

void Readability::makeHtmlReadable(const QString& html, const QString& base_url) {
  QProcess* proc = new QProcess(this);

  connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Readability::onReadabilityFinished);

  qApp->nodejs()->runScript(proc,
                            QSL("c:\\Projekty\\Moje\\build-rssguard-Desktop_Qt_6_2_3_MSVC2017_64bit-Debug\\"
                                "src\\rssguard\\data4\\node-packages-windows\\article-to-readable.js"),
                            { base_url });

  proc->write(html.toUtf8());
  proc->closeWriteChannel();
}

void Readability::onReadabilityFinished(int exit_code, QProcess::ExitStatus exit_status) {
  QProcess* proc = qobject_cast<QProcess*>(sender());

  if (exit_status == QProcess::ExitStatus::NormalExit &&
      exit_code == EXIT_SUCCESS) {
    emit htmlReadabled(QString::fromUtf8(proc->readAllStandardOutput()));
  }
  else {
    QString err = QString::fromUtf8(proc->readAllStandardError());
    emit errorOnHtmlReadabiliting(err);
  }

  proc->deleteLater();
}
