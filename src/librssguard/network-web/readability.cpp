// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/readability.h"

#include "3rd-party/boolinq/boolinq.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"

#define READABILITY_PACKAGE "@mozilla/readability"
#define READABILITY_VERSION "0.4.2"

Readability::Readability(QObject* parent) : QObject{parent}, m_modulesInstalling(false), m_modulesInstalled(false) {
  connect(qApp->nodejs(), &NodeJs::packageInstalledUpdated, this, &Readability::onPackageReady);
  connect(qApp->nodejs(), &NodeJs::packageError, this, &Readability::onPackageError);
}

void Readability::onPackageReady(const QList<NodeJs::PackageMetadata>& pkgs, bool already_up_to_date) {
  Q_UNUSED(already_up_to_date)

  bool concerns_readability = boolinq::from(pkgs).any([](const NodeJs::PackageMetadata& pkg) {
    return pkg.m_name == QSL(READABILITY_PACKAGE);
  });

  if (!concerns_readability) {
    return;
  }

  m_modulesInstalled = true;
  m_modulesInstalling = false;

  qApp->showGuiMessage(Notification::Event::NodePackageUpdated,
                       { tr("Packages for reader mode are installed"),
                         tr("You can now use reader mode!"),
                         QSystemTrayIcon::MessageIcon::Information },
                       { true, true, false });

  // Emit this just to allow readability again for user.
  emit htmlReadabled({});
}

void Readability::onPackageError(const QList<NodeJs::PackageMetadata>& pkgs, const QString& error) {
  bool concerns_readability = boolinq::from(pkgs).any([](const NodeJs::PackageMetadata& pkg) {
    return pkg.m_name == QSL(READABILITY_PACKAGE);
  });

  if (!concerns_readability) {
    return;
  }

  m_modulesInstalled = m_modulesInstalling = false;

  qApp->showGuiMessage(Notification::Event::NodePackageUpdated,
                       { tr("Packages for reader mode are NOT installed"),
                         tr("There is error: %1").arg(error),
                         QSystemTrayIcon::MessageIcon::Critical },
                       { true, true, false });

  // Emit this just to allow readability again for user.
  emit htmlReadabled({});
}

void Readability::makeHtmlReadable(const QString& html, const QString& base_url) {
  if (!m_modulesInstalled) {
    NodeJs::PackageStatus st = qApp->nodejs()->packageStatus({ QSL(READABILITY_PACKAGE), QSL(READABILITY_VERSION) });

    if (st != NodeJs::PackageStatus::UpToDate) {
      if (!m_modulesInstalling) {
        // We make sure to update modules.
        m_modulesInstalling = true;

        qApp->showGuiMessage(Notification::Event::NodePackageUpdated,
                             { tr("Node.js libraries not installed"),
                               tr("%1 will now install some needed libraries, this will take only a few seconds. "
                                  "You will be notified when installation is complete.").arg(QSL(APP_NAME)),
                               QSystemTrayIcon::MessageIcon::Information },
                             { true, true, false });
        qApp->nodejs()->installUpdatePackages({ { QSL(READABILITY_PACKAGE), QSL(READABILITY_VERSION) } });
      }

      return;
    }
    else {
      m_modulesInstalled = true;
    }
  }

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
