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

#ifndef SKINFACTORY_H
#define SKINFACTORY_H

#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QMetaType>


struct Skin {
    QString m_baseName;
    QString m_visibleName;
    QStringList m_stylesNames;
    QString m_author;
    QString m_email;
    QString m_version;
    QString m_rawData;
    QString m_layoutMarkupWrapper;
    QString m_layoutMarkup;
};

Q_DECLARE_METATYPE(Skin)

class SkinFactory : public QObject {
    Q_OBJECT

  private:
    // Constructor.
    explicit SkinFactory(QObject *parent = 0);

    // Loads the skin from give skin_data.
    bool loadSkinFromData(const Skin &skin);

  public:
    // Destructor.
    virtual ~SkinFactory();

    // Loads skin name from settings and sets it as active.
    void loadCurrentSkin();

    // Returns contents of current layout markup.
    inline QString currentMarkup() {
      return m_currentSkin.m_layoutMarkup;
    }

    inline QString currentMarkupLayout() {
      return m_currentSkin.m_layoutMarkupWrapper;
    }

    // Returns the name of the skin, that should be activated
    // after application restart.
    QString selectedSkinName();

    // Gets skin about a particular skin.
    Skin skinInfo(const QString &skin_name, bool *ok = NULL);

    // Returns list of installed skins.
    QList<Skin> installedSkins();

    // Sets the desired skin as the active one if it exists.
    void setCurrentSkinName(const QString &skin_name);

    // Singleton getter.
    static SkinFactory *instance();

  private:
    // Holds name of the current skin.
    Skin m_currentSkin;

    // Singleton.
    static QPointer<SkinFactory> s_instance;
};

#endif // SKINFACTORY_H
