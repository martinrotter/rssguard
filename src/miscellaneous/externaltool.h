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

#ifndef EXTERNALTOOL_H
#define EXTERNALTOOL_H

#include <QStringList>
#include <QMetaType>


class ExternalTool {
  public:
    explicit ExternalTool();
    ExternalTool(const ExternalTool& other);
    explicit ExternalTool(const QString& executable, const QStringList& parameters);

    QString toString();
    QString executable() const;
    QStringList parameters() const;

    static ExternalTool fromString(const QString& str);
    static QList<ExternalTool> toolsFromSettings();
    static void setToolsToSettings(QList<ExternalTool>& tools);

  private:
    QString m_executable;
    QStringList m_parameters;
    void sanitizeParameters();
};

Q_DECLARE_METATYPE(ExternalTool)

#endif // EXTERNALTOOL_H
