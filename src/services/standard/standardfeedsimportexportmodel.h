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

#ifndef STANDARDFEEDSIMPORTEXPORTMODEL_H
#define STANDARDFEEDSIMPORTEXPORTMODEL_H

#include "services/abstract/accountcheckmodel.h"


class FeedsImportExportModel : public AccountCheckModel {
    Q_OBJECT

  public:
    enum Mode {
      Import,
      Export
    };

    // Constructors and destructors.
    explicit FeedsImportExportModel(QObject *parent = 0);
    virtual ~FeedsImportExportModel();

    // Exports to OPML 2.0
    // NOTE: http://dev.opml.org/spec2.html
    bool exportToOMPL20(QByteArray &result);
    void importAsOPML20(const QByteArray &data, bool fetch_metadata_online);

    // Exports to plain text format
    // where there is one feed URL per line.
    bool exportToTxtURLPerLine(QByteArray &result);
    void importAsTxtURLPerLine(const QByteArray &data, bool fetch_metadata_online);

    Mode mode() const;
    void setMode(const Mode &mode);

  signals:
    // These signals are emitted when user selects some data
    // to be imported/parsed into the model.
    void parsingStarted();
    void parsingProgress(int completed, int total);
    void parsingFinished(int count_failed, int count_succeeded, bool parsing_error);

  private:
    Mode m_mode;
};

#endif // STANDARDFEEDSIMPORTEXPORTMODEL_H
