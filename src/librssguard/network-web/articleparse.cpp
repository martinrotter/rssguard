// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/articleparse.h"

#include "3rd-party/boolinq/boolinq.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"

#include <QDir>

#define EXTRACTOR_PACKAGE "@extractus/article-extractor"
#define EXTRACTOR_VERSION "8.0.7"

#define FETCH_PACKAGE "fetch-charset-detection"
#define FETCH_VERSION "1.0.1"

ArticleParse::ArticleParse(QObject* parent) : QObject{parent}, m_modulesInstalling(false), m_modulesInstalled(false) {
  connect(qApp->nodejs(), &NodeJs::packageInstalledUpdated, this, &ArticleParse::onPackageReady);
  connect(qApp->nodejs(), &NodeJs::packageError, this, &ArticleParse::onPackageError);
}

void ArticleParse::onPackageReady(const QList<NodeJs::PackageMetadata>& pkgs, bool already_up_to_date) {
  Q_UNUSED(already_up_to_date)

  bool concerns_extractor = boolinq::from(pkgs).any([](const NodeJs::PackageMetadata& pkg) {
    return pkg.m_name == QSL(EXTRACTOR_PACKAGE);
  });

  if (!concerns_extractor) {
    return;
  }

  m_modulesInstalled = true;
  m_modulesInstalling = false;

  qApp->showGuiMessage(Notification::Event::NodePackageUpdated,
                       {tr("Packages for article-extractor are installed"),
                        tr("Reload your website or article and you can test it then!"),
                        QSystemTrayIcon::MessageIcon::Information},
                       {true, true, false});

  // Emit this just to allow the action again for user.
  emit articleParsed(nullptr, tr("Packages for article-extractor are installed. You can now use this feature!"));
}

void ArticleParse::onPackageError(const QList<NodeJs::PackageMetadata>& pkgs, const QString& error) {
  bool concerns_extractor = boolinq::from(pkgs).any([](const NodeJs::PackageMetadata& pkg) {
    return pkg.m_name == QSL(EXTRACTOR_PACKAGE);
  });

  if (!concerns_extractor) {
    return;
  }

  m_modulesInstalled = m_modulesInstalling = false;

  qApp->showGuiMessage(Notification::Event::NodePackageUpdated,
                       {tr("Packages for article-extractor are NOT installed"),
                        tr("There is error: %1").arg(error),
                        QSystemTrayIcon::MessageIcon::Critical},
                       {true, true, false});

  // Emit this just to allow readability again for user.
  emit articleParsed(nullptr, tr("Packages for article-extractor are NOT installed. There is error: %1").arg(error));
}

void ArticleParse::parseArticle(QObject* sndr, const QString& url) {
  if (!m_modulesInstalled) {
    // NOTE: Here we use MJS file directly placed in its NODE package folder
    // because NODE_PATH is not supported for MJS files.
    m_scriptFilename = qApp->nodejs()->processedPackageFolder() + QDir::separator() + QSL("extract-article.mjs");

    if (!IOFactory::copyFile(QSL(":/scripts/article-extractor/extract-article.mjs"), m_scriptFilename)) {
      qCriticalNN << LOGSEC_ADBLOCK << "Failed to copy article-extractor script to TEMP.";
    }

    try {
      NodeJs::PackageStatus st_extractor =
        qApp->nodejs()->packageStatus({QSL(EXTRACTOR_PACKAGE), QSL(EXTRACTOR_VERSION)});
      NodeJs::PackageStatus st_fetch = qApp->nodejs()->packageStatus({QSL(FETCH_PACKAGE), QSL(FETCH_VERSION)});

      if (st_extractor != NodeJs::PackageStatus::UpToDate || st_fetch != NodeJs::PackageStatus::UpToDate) {
        if (!m_modulesInstalling) {
          // We make sure to update modules.
          m_modulesInstalling = true;

          qApp->showGuiMessage(Notification::Event::NodePackageUpdated,
                               {tr("Node.js libraries not installed"),
                                tr("%1 will now install some needed libraries, this will take only a few seconds. "
                                   "You will be notified when installation is complete.")
                                  .arg(QSL(APP_NAME)),
                                QSystemTrayIcon::MessageIcon::Warning},
                               {true, true, false});
          qApp->nodejs()->installUpdatePackages({{QSL(EXTRACTOR_PACKAGE), QSL(EXTRACTOR_VERSION)},
                                                 {QSL(FETCH_PACKAGE), QSL(FETCH_VERSION)}});
        }

        return;
      }
      else {
        m_modulesInstalled = true;
      }
    }
    catch (const ApplicationException& ex) {
      qApp->showGuiMessage(Notification::Event::NodePackageUpdated,
                           {tr("Node.js libraries not installed"),
                            tr("Node.js is not configured properly. Go to \"Settings\" -> \"Node.js\" and check "
                               "if your Node.js is properly configured."),
                            QSystemTrayIcon::MessageIcon::Critical},
                           {true, true, false});

      qCriticalNN << LOGSEC_CORE << "Failed to check for Node.js package status:" << QUOTE_W_SPACE_DOT(ex.message());

      // Emit this just to allow readability again for user.
      emit articleParsed(sndr,
                         tr("Node.js is not configured properly. Go to \"Settings\" -> \"Node.js\" and check "
                            "if your Node.js is properly configured."));
    }
  }

  QProcess* proc = new QProcess(this);

  connect(proc,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this,
          [=](int exit_code, QProcess::ExitStatus exit_status) {
            onParsingFinished(sndr, exit_code, exit_status);
          });

  qApp->nodejs()->runScript(proc, m_scriptFilename, {url});
}

void ArticleParse::onParsingFinished(QObject* sndr, int exit_code, QProcess::ExitStatus exit_status) {
  QProcess* proc = qobject_cast<QProcess*>(sender());

  if (exit_status == QProcess::ExitStatus::NormalExit && exit_code == EXIT_SUCCESS) {
    emit articleParsed(sndr, QString::fromUtf8(proc->readAllStandardOutput()));
  }
  else {
    QString err = QString::fromUtf8(proc->readAllStandardError());
    emit errorOnArticlePArsing(sndr, err);
  }

  proc->deleteLater();
}
