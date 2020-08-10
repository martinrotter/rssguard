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
  m_parameters.removeDuplicates();
  m_parameters.removeAll(QString());
}

ExternalTool::ExternalTool(const ExternalTool& other) : ExternalTool(other.executable(), other.parameters()) {}

ExternalTool::ExternalTool(QString executable, QStringList parameters)
  : m_executable(std::move(executable)), m_parameters(std::move(parameters)) {
  sanitizeParameters();
}

QString ExternalTool::toString() {
  sanitizeParameters();
  return m_executable + EXTERNAL_TOOL_SEPARATOR + m_parameters.join(EXTERNAL_TOOL_PARAM_SEPARATOR);
}

QString ExternalTool::executable() const {
  return m_executable;
}

QStringList ExternalTool::parameters() const {
  return m_parameters;
}

ExternalTool ExternalTool::fromString(const QString& str) {
  QStringList outer = str.split(EXTERNAL_TOOL_SEPARATOR);

  if (outer.size() != 2) {
    throw ApplicationException(QObject::tr("Passed external tool representation is not valid."));
  }
  else {
    const QString& executable = outer.at(0);
    const QStringList parameters = outer.at(1).split(EXTERNAL_TOOL_PARAM_SEPARATOR);

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

void ExternalTool::setToolsToSettings(QList<ExternalTool>& tools) {
  QStringList encode;

  for (ExternalTool tool : tools) {
    encode.append(tool.toString());
  }

  qApp->settings()->setValue(GROUP(Browser), Browser::ExternalTools, encode);
}
