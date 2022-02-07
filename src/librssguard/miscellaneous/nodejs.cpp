// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/nodejs.h"

#include "exceptions/applicationexception.h"
#include "exceptions/processexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/settings.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

NodeJs::NodeJs(Settings* settings, QObject* parent) : QObject(parent), m_settings(settings) {}

QString NodeJs::nodeJsExecutable() const {
  return QDir::toNativeSeparators(m_settings->value(GROUP(Node), SETTING(Node::NodeJsExecutable)).toString());
}

void NodeJs::setNodeJsExecutable(const QString& exe) const {
  m_settings->setValue(GROUP(Node), Node::NodeJsExecutable, exe);
}

QString NodeJs::npmExecutable() const {
  return QDir::toNativeSeparators(m_settings->value(GROUP(Node), SETTING(Node::NpmExecutable)).toString());
}

void NodeJs::setNpmExecutable(const QString& exe) const {
  m_settings->setValue(GROUP(Node), Node::NpmExecutable, exe);
}

void NodeJs::setPackageFolder(const QString& path) {
  m_settings->setValue(GROUP(Node), Node::PackageFolder, path);
}

QString NodeJs::packageFolder() const {
  QString path = QDir::toNativeSeparators(m_settings->value(GROUP(Node), SETTING(Node::PackageFolder)).toString());

  return path;
}

QString NodeJs::processedPackageFolder() const {
  QString path = qApp->replaceDataUserDataFolderPlaceholder(packageFolder());

  if (!QDir().mkpath(path)) {
    qCriticalNN << LOGSEC_NODEJS << "Failed to create package folder structure" << QUOTE_W_SPACE_DOT(path);
  }

  return QDir::toNativeSeparators(path);
}

QString NodeJs::nodejsVersion(const QString& nodejs_exe) const {
  if (nodejs_exe.simplified().isEmpty()) {
    throw ApplicationException(tr("file not found"));
  }

  return IOFactory::startProcessGetOutput(nodejs_exe, { QSL("--version") }).simplified();
}

QString NodeJs::npmVersion(const QString& npm_exe) const {
  if (npm_exe.simplified().isEmpty()) {
    throw ApplicationException(tr("file not found"));
  }

  return IOFactory::startProcessGetOutput(npm_exe, { QSL("--version") }).simplified();
}

NodeJs::PackageStatus NodeJs::packageStatus(const PackageMetadata& pkg) const {
  QString npm_ls = IOFactory::startProcessGetOutput(npmExecutable(),
                                                    { QSL("ls"), QSL("--unicode"), QSL("--json"), QSL("--prefix"),
                                                      processedPackageFolder() });
  QJsonDocument json = QJsonDocument::fromJson(npm_ls.toUtf8());
  QJsonObject deps = json.object()["dependencies"].toObject();

  if (deps.contains(pkg.m_name)) {
    QString vers = deps[pkg.m_name].toObject()["version"].toString();

    return vers == pkg.m_version ? PackageStatus::UpToDate : PackageStatus::OutOfDate;
  }
  else {
    return PackageStatus::NotInstalled;
  }
}

void NodeJs::installUpdatePackage(const PackageMetadata& pkg) {
  auto pkg_status = packageStatus(pkg);

  switch (pkg_status) {
    case PackageStatus::NotInstalled:
    case PackageStatus::OutOfDate:
      installPackage(pkg);
      break;

    case PackageStatus::UpToDate:
      emit packageInstalledUpdated(pkg);

      break;
  }
}

void NodeJs::installPackage(const PackageMetadata& pkg) {
  // npm install --prefix "." @cliqz/adblocker@">=1.0.0 <2.0.0" --production --save-exact
  //https://docs.npmjs.com/cli/v8/commands/npm-install
  try {
    QProcess* proc = new QProcess();

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [pkg, this](int exit_code,
                                                                                                   QProcess::ExitStatus status) {
      QProcess* sndr = qobject_cast<QProcess*>(sender());

      if (exit_code != EXIT_SUCCESS || status == QProcess::ExitStatus::CrashExit) {
        qCriticalNN << LOGSEC_NODEJS << "Error when installing package" << QUOTE_W_SPACE_DOT(pkg.m_name)
                    << " Exit code:" << QUOTE_W_SPACE_DOT(exit_code)
                    << " Message:" << QUOTE_W_SPACE_DOT(sndr->readAllStandardError());

        emit packageError(pkg, sndr->errorString());
      }
      else {
        qDebugNN << LOGSEC_NODEJS << "Installed/updated package" << QUOTE_W_SPACE(pkg.m_name)
                 << "with version" << QUOTE_W_SPACE_DOT(pkg.m_version);
        emit packageInstalledUpdated(pkg);
      }
    });
    connect(proc, &QProcess::errorOccurred, this, [pkg, this](QProcess::ProcessError error) {
      QProcess* sndr = qobject_cast<QProcess*>(sender());

      qCriticalNN << LOGSEC_NODEJS << "Error when installing package" << QUOTE_W_SPACE_DOT(pkg.m_name)
                  << " Message:" << QUOTE_W_SPACE_DOT(error);

      emit packageError(pkg, sndr->errorString());
    });

    IOFactory::startProcess(proc,
                            npmExecutable(),
                            { QSL("install"), QSL("--production"),
                              QSL("%1@%2").arg(pkg.m_name, pkg.m_version),
                              QSL("--prefix"), processedPackageFolder() });
  }
  catch (const ProcessException& ex) {
    emit packageError(pkg, ex.message());
  }
}

void NodeJs::updatePackage(const PackageMetadata& pkg) {
  //  npm update --prefix "." @cliqz/adblocker@">=1.0.0 <2.0.0" --production --save-exact
  //https://docs.npmjs.com/cli/v8/commands/npm-update
}
