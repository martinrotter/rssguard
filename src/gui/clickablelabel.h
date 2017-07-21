// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2010-2014 by David Rosca <nowrep@gmail.com>
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

#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>
#include <QIcon>


class QMouseEvent;

class ClickableLabel : public QLabel {
		Q_OBJECT
		Q_PROPERTY(QSize fixedsize READ size WRITE setFixedSize)
		Q_PROPERTY(int fixedwidth READ width WRITE setFixedWidth)
		Q_PROPERTY(int fixedheight READ height WRITE setFixedHeight)
		Q_PROPERTY(QString themeIcon READ themeIcon WRITE setThemeIcon)
		Q_PROPERTY(QIcon fallbackIcon READ fallbackIcon WRITE setFallbackIcon)

	public:
		explicit ClickableLabel(QWidget* parent = 0);

		QString themeIcon() const;
		void setThemeIcon(const QString& name);

		QIcon fallbackIcon() const;
		void setFallbackIcon(const QIcon& fallbackIcon);

	signals:
		void clicked(QPoint);
		void middleClicked(QPoint);

	private:
		void updateIcon();

		void resizeEvent(QResizeEvent* ev);
		void mouseReleaseEvent(QMouseEvent* ev);

		QString m_themeIcon;
		QIcon m_fallbackIcon;

};

#endif // CLICKABLELABEL_H
