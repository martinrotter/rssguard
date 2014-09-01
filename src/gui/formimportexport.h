// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef FORMEXPORT_H
#define FORMEXPORT_H

#include <QDialog>

#include "ui_formimportexport.h"
#include "core/feedsimportexportmodel.h"


namespace Ui {
  class FormExport;
}

class FormImportExport : public QDialog {
    Q_OBJECT

  public:
    enum ConversionType {
      OPML20 = 0
    };

    explicit FormImportExport(QWidget *parent = 0);
    virtual ~FormImportExport();

    void setMode(const FeedsImportExportModel::Mode &mode);

  private slots:
    void performAction();
    void selectFile();

  private:
    void selectExportFile();
    void selectImportFile();
    void parseImportFile(const QString &file_name);

    void exportFeeds();
    void importFeeds();

    Ui::FormImportExport *m_ui;
    ConversionType m_conversionType;
    FeedsImportExportModel *m_model;
};

#endif // FORMEXPORT_H
