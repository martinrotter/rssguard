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

#ifndef FORMDATABASECLEANUP_H
#define FORMDATABASECLEANUP_H

#include <QDialog>

#include "ui_formdatabasecleanup.h"

#include "miscellaneous/databasecleaner.h"


class FormDatabaseCleanup : public QDialog {
		Q_OBJECT

	public:
		// Constructors.
		explicit FormDatabaseCleanup(QWidget* parent = 0);
		virtual ~FormDatabaseCleanup();

		void setCleaner(DatabaseCleaner* cleaner);

	protected:
		void closeEvent(QCloseEvent* event);
		void keyPressEvent(QKeyEvent* event);

	private slots:
		void updateDaysSuffix(int number);
		void startPurging();
		void onPurgeStarted();
		void onPurgeProgress(int progress, const QString& description);
		void onPurgeFinished(bool finished);

	signals:
		void purgeRequested(const CleanerOrders& which_data);

	private:
		void loadDatabaseInfo();

	private:
		QScopedPointer<Ui::FormDatabaseCleanup> m_ui;
		DatabaseCleaner* m_cleaner;
};

#endif // FORMDATABASECLEANUP_H
