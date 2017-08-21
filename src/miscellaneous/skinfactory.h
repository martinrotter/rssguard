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

#ifndef SKINFACTORY_H
#define SKINFACTORY_H

#include <QObject>

#include <QStringList>
#include <QMetaType>


struct Skin {
	QString m_baseName;
	QString m_visibleName;
	QString m_author;
	QString m_email;
	QString m_version;
	QString m_rawData;
	QString m_adblocked;
	QString m_layoutMarkupWrapper;
	QString m_enclosureImageMarkup;
	QString m_layoutMarkup;
	QString m_enclosureMarkup;
};

Q_DECLARE_METATYPE(Skin)

class SkinFactory : public QObject {
		Q_OBJECT

	public:
		// Constructor.
		explicit SkinFactory(QObject* parent = 0);

		// Destructor.
		virtual ~SkinFactory();

		// Loads skin name from settings and sets it as active.
		void loadCurrentSkin();

		inline Skin currentSkin() const {
			return m_currentSkin;
		}

		// Returns the name of the skin, that should be activated
		// after application restart.
		QString selectedSkinName() const;

		QString adBlockedPage(const QString& subscription, const QString& rule);

		// Gets skin about a particular skin.
		Skin skinInfo(const QString& skin_name, bool* ok = nullptr) const;

		// Returns list of installed skins.
		QList<Skin> installedSkins() const;

		// Sets the desired skin as the active one if it exists.
		void setCurrentSkinName(const QString& skin_name);

		QString customSkinBaseFolder() const;

	private:

		// Loads the skin from give skin_data.
		void loadSkinFromData(const Skin& skin);

		// Holds name of the current skin.
		Skin m_currentSkin;
};

#endif // SKINFACTORY_H
