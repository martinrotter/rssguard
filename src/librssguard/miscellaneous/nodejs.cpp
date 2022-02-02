// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/nodejs.h"

#include "exceptions/applicationexception.h"
#include "miscellaneous/iofactory.h"
#include "miscellaneous/settings.h"

#include <QDir>

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

QString NodeJs::packageFolder() const {
  return QDir::toNativeSeparators(m_settings->value(GROUP(Node), SETTING(Node::PackageFolder)).toString());
}

void NodeJs::setPackageFolder(const QString& path) {}

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
