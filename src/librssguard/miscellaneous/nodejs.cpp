// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/nodejs.h"

#include "exceptions/applicationexception.h"
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

  return {};
}

void NodeJs::installUpdatePackage(const PackageMetadata& pkg) {
  auto pkg_status = packageStatus(pkg);

  switch (pkg_status) {
    case PackageStatus::NotInstalled:
      break;

    case PackageStatus::OutOfDate:
      break;

    case PackageStatus::UpToDate:
      break;
  }
}

void NodeJs::installPackage(const PackageMetadata& pkg) {
  // npm install --prefix "." @cliqz/adblocker@">=1.0.0 <2.0.0" --production --save-exact
  //https://docs.npmjs.com/cli/v8/commands/npm-install

}

void NodeJs::updatePackage(const PackageMetadata& pkg)
{
  //  npm update --prefix "." @cliqz/adblocker@">=1.0.0 <2.0.0" --production --save-exact
  //https://docs.npmjs.com/cli/v8/commands/npm-update
}
