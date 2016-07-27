// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "miscellaneous/skinfactory.h"

#include "miscellaneous/application.h"

#include <QDir>
#include <QStyleFactory>
#include <QDomDocument>
#include <QDomElement>


SkinFactory::SkinFactory(QObject *parent) : QObject(parent) {
}

SkinFactory::~SkinFactory() {
}

void SkinFactory::loadCurrentSkin() {
  QList<QString> skin_names_to_try;

  skin_names_to_try.append(selectedSkinName());
  skin_names_to_try.append(APP_SKIN_DEFAULT);

  bool skin_parsed;
  Skin skin_data;
  QString skin_name;

  while (!skin_names_to_try.isEmpty()) {
    skin_name = skin_names_to_try.takeFirst();
    skin_data = skinInfo(skin_name, &skin_parsed);

    if (skin_parsed) {
      loadSkinFromData(skin_data);

      // Set this 'Skin' object as active one.
      m_currentSkin = skin_data;

      qDebug("Skin '%s' loaded.", qPrintable(skin_name));
      return;
    }
    else {
      qWarning("Failed to load skin '%s'.", qPrintable(skin_name));
    }
  }

  qFatal("Failed to load selected or default skin. Quitting!");
}

void SkinFactory::loadSkinFromData(const Skin &skin) {
  // Iterate supported styles and load one.
  foreach (const QString &style, skin.m_stylesNames) {
    if (qApp->setStyle(style) != 0) {
      qDebug("Style '%s' loaded.", qPrintable(style));
      break;
    }
  }

  if (!skin.m_rawData.isEmpty()) {
    qApp->setStyleSheet(skin.m_rawData);
  }
}

void SkinFactory::setCurrentSkinName(const QString &skin_name) {
  qApp->settings()->setValue(GROUP(GUI), GUI::Skin, skin_name);
}

QString SkinFactory::selectedSkinName() const {
  return qApp->settings()->value(GROUP(GUI), SETTING(GUI::Skin)).toString();
}

Skin SkinFactory::skinInfo(const QString &skin_name, bool *ok) const {
  Skin skin;
  QString styles;
  QFile skin_file(APP_SKIN_PATH + QDir::separator() + skin_name);
  QDomDocument dokument;

  if (!skin_file.open(QIODevice::Text | QIODevice::ReadOnly) || !dokument.setContent(&skin_file, true)) {
    if (ok) {
      *ok = false;
    }

    return skin;
  }

  const QDomNode skin_node = dokument.namedItem(QSL("skin"));

  // Obtain visible skin name.
  skin.m_visibleName = skin_node.namedItem(QSL("name")).toElement().text();

  // Obtain style name.
  styles = skin_node.namedItem(QSL("style")).toElement().text();
  skin.m_stylesNames = styles.split(',', QString::SkipEmptyParts);

  // Obtain author.
  skin.m_author = skin_node.namedItem(QSL("author")).namedItem(QSL("name")).toElement().text();

  // Obtain email.
  skin.m_email = skin_node.namedItem(QSL("author")).namedItem(QSL("email")).toElement().text();

  // Obtain version.
  skin.m_version = skin_node.attributes().namedItem(QSL("version")).toAttr().value();

  // Obtain other information.
  skin.m_baseName = QString(skin_name).replace(QDir::separator(), '/');

  // Obtain base folder.
  const QString base_folder = skin.m_baseName.split(QL1C('/'), QString::SkipEmptyParts).at(0);

  // Here we use "/" instead of QDir::separator() because CSS2.1 url field
  // accepts '/' as path elements separator.
  //
  // "##" is placeholder for the actual path to skin file. This is needed for using
  // images within the QSS file.
  // So if one uses "##/images/border.png" in QSS then it is
  // replaced by fully absolute path and target file can
  // be safely loaded.

  // Obtain layout markup wrapper.
  skin.m_layoutMarkupWrapper = skin_node.namedItem(QSL("markup_wrapper")).toElement().text();
  skin.m_layoutMarkupWrapper = QByteArray::fromBase64(skin.m_layoutMarkupWrapper.toLocal8Bit());
  skin.m_layoutMarkupWrapper = skin.m_layoutMarkupWrapper.replace(QSL("##"), APP_SKIN_PATH + QL1S("/") + base_folder);

  // Obtain enclosure image layout
  skin.m_enclosureImageMarkup = skin_node.namedItem(QSL("enclosure_image")).toElement().text();
  skin.m_enclosureImageMarkup = QByteArray::fromBase64(skin.m_enclosureImageMarkup.toLocal8Bit());
  skin.m_enclosureImageMarkup = skin.m_enclosureImageMarkup.replace(QSL("##"), APP_SKIN_PATH + QL1S("/") + base_folder);

  // Obtain layout markup.
  skin.m_layoutMarkup = skin_node.namedItem(QSL("markup")).toElement().text();
  skin.m_layoutMarkup = QByteArray::fromBase64(skin.m_layoutMarkup.toLocal8Bit());
  skin.m_layoutMarkup = skin.m_layoutMarkup.replace(QSL("##"), APP_SKIN_PATH + QL1S("/") + base_folder);

  // Obtain enclosure hyperlink wrapper.
  skin.m_enclosureMarkup = skin_node.namedItem(QSL("markup_enclosure")).toElement().text();
  skin.m_enclosureMarkup = QByteArray::fromBase64(skin.m_enclosureMarkup.toLocal8Bit());
  skin.m_enclosureMarkup = skin.m_enclosureMarkup.replace(QSL("##"), APP_SKIN_PATH + QL1S("/") + base_folder);

  // Obtain skin raw data.
  skin.m_rawData = skin_node.namedItem(QSL("data")).toElement().text();
  skin.m_rawData = QByteArray::fromBase64(skin.m_rawData.toLocal8Bit());
  skin.m_rawData = skin.m_rawData.replace(QSL("##"), APP_SKIN_PATH + QL1S("/") + base_folder);

  // Free resources.
  skin_file.close();
  skin_file.deleteLater();

  if (ok != nullptr) {
    *ok = !skin.m_author.isEmpty() && !skin.m_version.isEmpty() &&
          !skin.m_baseName.isEmpty() && !skin.m_email.isEmpty() &&
          !skin.m_layoutMarkup.isEmpty();
  }

  return skin;
}

QList<Skin> SkinFactory::installedSkins() const {
  QList<Skin> skins;
  bool skin_load_ok;
  const QStringList skin_directories = QDir(APP_SKIN_PATH).entryList(QDir::Dirs |
                                                                     QDir::NoDotAndDotDot |
                                                                     QDir::NoSymLinks |
                                                                     QDir::Readable);

  foreach (const QString &base_directory, skin_directories) {
    // Check skins installed in this base directory.
    const QStringList skin_files = QDir(APP_SKIN_PATH + QDir::separator() + base_directory).entryList(QStringList() << QSL("*.xml"),
                                                                                                      QDir::Files | QDir::Readable | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    foreach (const QString &skin_file, skin_files) {
      // Check if skin file is valid and add it if it is valid.
      const Skin skin_info = skinInfo(base_directory + QDir::separator() + skin_file, &skin_load_ok);

      if (skin_load_ok) {
        skins.append(skin_info);
      }
    }
  }

  return skins;
}
