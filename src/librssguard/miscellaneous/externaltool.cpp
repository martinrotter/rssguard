// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/externaltool.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"

#include <QDir>
#include <QObject>
#include <utility>

void ExternalTool::sanitizeParameters() {
  m_executable = QDir::toNativeSeparators(m_executable);
}

ExternalTool::ExternalTool(const ExternalTool& other) : ExternalTool(other.executable(), other.parameters()) {}

ExternalTool::ExternalTool(QString executable, QString parameters)
  : m_executable(std::move(executable)), m_parameters(std::move(parameters)) {
  sanitizeParameters();
}

QString ExternalTool::toString() {
  sanitizeParameters();
  return m_executable + EXTERNAL_TOOL_SEPARATOR + m_parameters;
}

QString ExternalTool::executable() const {
  return m_executable;
}

QString ExternalTool::parameters() const {
  return m_parameters;
}

ExternalTool ExternalTool::fromString(const QString& str) {
  QStringList outer = str.split(QSL(EXTERNAL_TOOL_SEPARATOR));

  if (outer.size() != 2) {
    throw ApplicationException(QObject::tr("Passed external tool representation is not valid."));
  }
  else {
    const QString& executable = outer.at(0);
    const QString& parameters = outer.at(1);

    return ExternalTool(executable, parameters);
  }
}

QList<ExternalTool> ExternalTool::toolsFromSettings() {
  QStringList tools_encoded = qApp->settings()->value(GROUP(Browser), SETTING(Browser::ExternalTools)).toStringList();
  QList<ExternalTool> tools;

  for (const QString& tool_encoded : tools_encoded) {
    tools.append(ExternalTool::fromString(tool_encoded));
  }

  return tools;
}

void ExternalTool::setToolsToSettings(QVector<ExternalTool>& tools) {
  QStringList encode;

  for (ExternalTool tool : tools) {
    encode.append(tool.toString());
  }

  qApp->settings()->setValue(GROUP(Browser), Browser::ExternalTools, encode);
}

bool ExternalTool::run(const QString& target) {
  if (parameters().isEmpty()) {
    return IOFactory::startProcessDetached(executable(), {target});
  }
  else {
    auto pars = parameters();

    if (pars.contains(QSL("%1"))) {
      // We replace existing target placeholder.
      pars = pars.replace(QSL("%1"), target);
    }
    else {
      pars += QSL(" \"%1\"").arg(target);
    }

    auto params = TextFactory::tokenizeProcessArguments(pars);

    return IOFactory::startProcessDetached(executable(), params);
  }
}
