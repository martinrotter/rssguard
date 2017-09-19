// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "miscellaneous/externaltool.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "miscellaneous/application.h"

#include <QDir>
#include <QObject>

void ExternalTool::sanitizeParameters() {
  m_executable = QDir::toNativeSeparators(m_executable);
  m_parameters.removeDuplicates();
  m_parameters.removeAll(QString());
}

ExternalTool::ExternalTool() {}

ExternalTool::ExternalTool(const ExternalTool& other) : ExternalTool(other.executable(), other.parameters()) {}

ExternalTool::ExternalTool(const QString& executable, const QStringList& parameters)
  : m_executable(executable), m_parameters(parameters) {
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
    const QString executable = outer.at(0);
    const QStringList parameters = outer.at(1).split(EXTERNAL_TOOL_PARAM_SEPARATOR);

    return ExternalTool(executable, parameters);
  }
}

QList<ExternalTool> ExternalTool::toolsFromSettings() {
  QStringList tools_encoded = qApp->settings()->value(GROUP(Browser), SETTING(Browser::ExternalTools)).toStringList();

  QList<ExternalTool> tools;

  foreach (const QString& tool_encoded, tools_encoded) {
    tools.append(ExternalTool::fromString(tool_encoded));
  }

  return tools;
}

void ExternalTool::setToolsToSettings(QList<ExternalTool>& tools) {
  QStringList encode;

  foreach (ExternalTool tool, tools) {
    encode.append(tool.toString());
  }

  qApp->settings()->setValue(GROUP(Browser), Browser::ExternalTools, encode);
}
