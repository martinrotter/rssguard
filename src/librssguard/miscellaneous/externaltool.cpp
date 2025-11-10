// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/externaltool.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <utility>

#include <QDir>
#include <QJsonObject>
#include <QObject>

void ExternalTool::sanitizeParameters() {
  m_executable = QDir::toNativeSeparators(m_executable);
}

ExternalTool::ExternalTool(const ExternalTool& other)
  : ExternalTool(other.name(), other.executable(), other.parameters()) {}

ExternalTool::ExternalTool(QString name, QString executable, QString parameters)
  : m_name(std::move(name)), m_executable(std::move(executable)), m_parameters(std::move(parameters)) {
  sanitizeParameters();
}

QByteArray ExternalTool::toString() {
  QJsonObject obj;

  obj[QSL("name")] = name();
  obj[QSL("exe")] = executable();
  obj[QSL("params")] = parameters();

  return QJsonDocument(obj).toJson(QJsonDocument::JsonFormat::Compact);
}

QString ExternalTool::executable() const {
  return m_executable;
}

QString ExternalTool::parameters() const {
  return m_parameters;
}

ExternalTool ExternalTool::fromString(const QByteArray& str) {
  auto json = QJsonDocument::fromJson(str);
  auto obj = json.object();
  ExternalTool tool(obj[QSL("name")].toString(), obj[QSL("exe")].toString(), obj[QSL("params")].toString());

  return tool;
}

QList<ExternalTool> ExternalTool::toolsFromSettings() {
  QStringList keys = qApp->settings()->allKeys(GROUP(ExternalTools));
  QList<ExternalTool> tools;

  for (const QString& key : keys) {
    auto data = qApp->settings()->value(GROUP(ExternalTools), key).toByteArray();

    if (data.isEmpty()) {
      continue;
    }

    tools.append(ExternalTool::fromString(data));
  }

  return tools;
}

void ExternalTool::setToolsToSettings(QVector<ExternalTool>& tools) {
  int i = 0;

  for (ExternalTool tool : tools) {
    auto data = tool.toString();
    qApp->settings()->setValue(GROUP(ExternalTools), QString::number(i++), data);
  }
}

QString ExternalTool::name() const {
  return m_name;
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
      pars += QSL(" '%1'").arg(target);
    }

    auto params = TextFactory::tokenizeProcessArguments(pars);

    return IOFactory::startProcessDetached(executable(), params);
  }
}
