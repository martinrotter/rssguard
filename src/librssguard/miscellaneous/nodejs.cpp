// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/nodejs.h"

#include "exceptions/applicationexception.h"
#include "exceptions/processexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/settings.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

NodeJs::NodeJs(Settings* settings, QObject* parent) : QObject(parent), m_settings(settings) {}

void NodeJs::runScript(QProcess* proc, const QString& script, const QStringList& arguments) const {
  QStringList arg = {script};
  arg.append(arguments);
  QProcessEnvironment env;
  QString node_modules = processedPackageFolder() + QDir::separator() + QSL("node_modules");

  env.insert(QSL("NODE_PATH"), node_modules);

  IOFactory::startProcess(proc, nodeJsExecutable(), arg, env);
}

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
  QString path = qApp->replaceUserDataFolderPlaceholder(packageFolder());

  if (!QDir().mkpath(path)) {
    qCriticalNN << LOGSEC_NODEJS << "Failed to create package folder structure" << QUOTE_W_SPACE_DOT(path);
  }

  if (!QDir(path).exists(QSL("package.json"))) {
    QFile fl(path + QDir::separator() + QSL("package.json"));

    fl.open(QIODevice::OpenModeFlag::WriteOnly);
    fl.write(QString("{}").toUtf8());
    fl.flush();
    fl.close();
  }

  return QDir::toNativeSeparators(path);
}

QString NodeJs::nodeJsVersion(const QString& nodejs_exe) const {
  if (nodejs_exe.simplified().isEmpty()) {
    throw ApplicationException(tr("file not found"));
  }

  return IOFactory::startProcessGetOutput(nodejs_exe, {QSL("--version")}).simplified();
}

QString NodeJs::npmVersion(const QString& npm_exe) const {
  if (npm_exe.simplified().isEmpty()) {
    throw ApplicationException(tr("file not found"));
  }

  return IOFactory::startProcessGetOutput(npm_exe, {QSL("--version")}).simplified();
}

NodeJs::PackageStatus NodeJs::packageStatus(const PackageMetadata& pkg) const {
  QString npm_ls = IOFactory::startProcessGetOutput(npmExecutable(),
                                                    {QSL("ls"),
                                                     QSL("--unicode"),
                                                     QSL("--json"),
                                                     QSL("--prefix"),
                                                     processedPackageFolder()},
                                                    {},
                                                    processedPackageFolder());
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

void NodeJs::installUpdatePackages(const QList<PackageMetadata>& pkgs) {
  QList<PackageMetadata> to_install;
  QStringList desc;

  for (const PackageMetadata& mt : pkgs) {
    try {
      auto pkg_status = packageStatus(mt);

      switch (pkg_status) {
        case PackageStatus::NotInstalled:
        case PackageStatus::OutOfDate:
          to_install.append(mt);
          break;

        default:
          desc << QSL("%1@%2").arg(mt.m_name, mt.m_version);
          break;
      }
    }
    catch (const ApplicationException& ex) {
      emit packageError(pkgs, ex.message());

      return;
    }
  }

  if (to_install.isEmpty()) {
    qDebugNN << LOGSEC_NODEJS << "Packages" << QUOTE_W_SPACE(desc.join(QL1S(", "))) << "are up-to-date.";

    emit packageInstalledUpdated(pkgs, true);
  }
  else {
    installPackages(pkgs);
  }
}

QString NodeJs::packagesToString(const QList<PackageMetadata>& pkgs) {
  QStringList desc;

  for (const PackageMetadata& mt : pkgs) {
    desc << QSL("%1@%2").arg(mt.m_name, mt.m_version);
  }

  return desc.join(QL1S(", "));
}

void NodeJs::installPackages(const QList<PackageMetadata>& pkgs) {
  QStringList to_install;

  try {
    for (const PackageMetadata& mt : pkgs) {
      to_install.append(QSL("%1@%2").arg(mt.m_name, mt.m_version));
    }

    QProcess* proc = new QProcess();

    connect(proc,
            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this,
            [pkgs, this](int exit_code, QProcess::ExitStatus status) {
              QProcess* sndr = qobject_cast<QProcess*>(sender());

              if (exit_code != EXIT_SUCCESS || status == QProcess::ExitStatus::CrashExit) {
                qCriticalNN << LOGSEC_NODEJS << "Error when installing packages"
                            << QUOTE_W_SPACE_DOT(packagesToString(pkgs))
                            << " Exit code:" << QUOTE_W_SPACE_DOT(exit_code)
                            << " Message:" << QUOTE_W_SPACE_DOT(sndr->readAllStandardError());

                emit packageError(pkgs, sndr->errorString());
              }
              else {
                qDebugNN << LOGSEC_NODEJS << "Installed/updated packages" << QUOTE_W_SPACE(packagesToString(pkgs));
                emit packageInstalledUpdated(pkgs, false);
              }
            });
    connect(proc, &QProcess::errorOccurred, this, [pkgs, this](QProcess::ProcessError error) {
      QProcess* sndr = qobject_cast<QProcess*>(sender());

      qCriticalNN << LOGSEC_NODEJS << "Error when installing packages" << QUOTE_W_SPACE_DOT(packagesToString(pkgs))
                  << " Message:" << QUOTE_W_SPACE_DOT(error);

      emit packageError(pkgs, sndr->errorString());
    });

    qDebugNN << LOGSEC_NODEJS << "Installing packages" << QUOTE_W_SPACE_DOT(packagesToString(pkgs));

    to_install.prepend(QSL("--production"));
    to_install.prepend(QSL("install"));

    // to_install.append(QSL("--prefix"));
    // to_install.append(processedPackageFolder());

    IOFactory::startProcess(proc, npmExecutable(), to_install, {}, processedPackageFolder());
  }
  catch (const ProcessException& ex) {
    qCriticalNN << LOGSEC_NODEJS << "Packages" << QUOTE_W_SPACE(to_install)
                << "were not installed, error:" << QUOTE_W_SPACE_DOT(ex.message());

    emit packageError(pkgs, ex.message());
  }
}
