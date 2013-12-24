#include "gui/skinfactory.h"

#include "core/defs.h"
#include "core/settings.h"

#include <QApplication>
#include <QDomDocument>
#include <QDir>
#include <QStyleFactory>
#include <QDomDocument>
#include <QDomElement>


QPointer<SkinFactory> SkinFactory::s_instance;

SkinFactory::SkinFactory(QObject *parent) : QObject(parent) {
}

SkinFactory::~SkinFactory() {
  qDebug("Destroying SkinFactory instance.");
}

SkinFactory *SkinFactory::getInstance() {
  if (s_instance.isNull()) {
    s_instance = new SkinFactory(qApp);
  }

  return s_instance;
}

void SkinFactory::loadCurrentSkin() {
  QString skin_name_from_settings = getSelectedSkinName();
  bool skin_parsed;
  Skin skin_data = getSkinInfo(skin_name_from_settings, &skin_parsed);

  if (skin_parsed) {
    loadSkinFromData(skin_data);

    // Set this 'Skin' object as active one.
    m_currentSkin = skin_data;

    qDebug("Skin '%s' loaded.", qPrintable(skin_name_from_settings));
  }
  else {
    // TODO: Change this to qFatal once code is stable.
    qWarning("Skin '%s' not loaded because its data are corrupted. No skin is loaded now!",
             qPrintable(skin_name_from_settings));
  }
}

bool SkinFactory::loadSkinFromData(const Skin &skin) {
  QStringList skin_parts = skin.m_baseName.split('/', QString::SkipEmptyParts);

  // Skin does not contain leading folder name or the actual skin file name.
  if (skin_parts.size() != 2) {
    qDebug("Loading of sking '%s' failed because skin name does not contain "
           "base folder name or the actual skin name.",
           qPrintable(skin.m_baseName));
    return false;
  }
  else {
    qDebug("Loading skin '%s'.", qPrintable(skin.m_baseName));
  }

  // Create needed variables and create QFile object representing skin contents.
  QString skin_folder = skin_parts.at(0);

  // Here we use "/" instead of QDir::separator() because CSS2.1 url field
  // accepts '/' as path elements separator.
  //
  // "##" is placeholder for the actual path to skin file. This is needed for using
  // images within the QSS file.
  QString raw_data = skin.m_rawData;

  if (!raw_data.isEmpty()) {
    QString parsed_data = raw_data.replace("##",
                                           APP_SKIN_PATH + '/' +
                                           skin_folder + "/images");
    qApp->setStyleSheet(parsed_data);
  }

  // Iterate supported styles and load one.
  foreach (const QString &style, skin.m_stylesNames) {
    if (qApp->setStyle(style) != 0) {
      qDebug("Style '%s' loaded.", qPrintable(style));
      break;
    }
  }

  return true;
}

void SkinFactory::setCurrentSkinName(const QString &skin_name) {
  Settings::getInstance()->setValue(APP_CFG_GUI, "skin", skin_name);
}

QString SkinFactory::getSelectedSkinName() {
  return Settings::getInstance()->value(APP_CFG_GUI,
                                        "skin",
                                        APP_SKIN_DEFAULT).toString();
}

QString SkinFactory::getCurrentMarkup() {
  return m_currentSkin.m_layoutMarkup;
}

Skin SkinFactory::getSkinInfo(const QString &skin_name, bool *ok) {
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

  QDomNode skin_node = dokument.namedItem("skin");

  // Obtain visible skin name.
  skin.m_visibleName = skin_node.namedItem("name").toElement().text();

  // Obtain skin raw data.
  skin.m_rawData = skin_node.namedItem("data").toElement().text();
  skin.m_rawData = QByteArray::fromBase64(skin.m_rawData.toLocal8Bit());

  // Obtain style name.
  styles = skin_node.namedItem("style").toElement().text();
  skin.m_stylesNames = styles.split(',', QString::SkipEmptyParts);

  // Obtain author.
  skin.m_author = skin_node.namedItem("author").namedItem("name").toElement().text();

  // Obtain email.
  skin.m_email = skin_node.namedItem("author").namedItem("email").toElement().text();

  // Obtain version.
  skin.m_version = skin_node.attributes().namedItem("version").toAttr().value();

  // Obtain layout markup.
  skin.m_layoutMarkup = skin_node.namedItem("markup").toElement().text();
  skin.m_layoutMarkup = QByteArray::fromBase64(skin.m_layoutMarkup.toLocal8Bit());

  // Obtain other information.
  // NOTE: Probably fixed bug with "active skin" on Windows.
  skin.m_baseName = QString(skin_name).replace(QDir::separator(), '/');

  // Free resources.
  skin_file.close();
  skin_file.deleteLater();

  if (ok) {
    *ok = !skin.m_author.isEmpty() && !skin.m_version.isEmpty() &&
          !skin.m_baseName.isEmpty() && !skin.m_email.isEmpty() &&
          !skin.m_layoutMarkup.isEmpty();
  }

  return skin;
}

QList<Skin> SkinFactory::getInstalledSkins() {
  QList<Skin> skins;
  bool skin_load_ok;
  QStringList skin_directories = QDir(APP_SKIN_PATH).entryList(QDir::Dirs |
                                                               QDir::NoDotAndDotDot |
                                                               QDir::NoSymLinks |
                                                               QDir::Readable);

  foreach (const QString &base_directory, skin_directories) {
    // Check skins installed in this base directory.
    QStringList skin_files = QDir(APP_SKIN_PATH + QDir::separator() + base_directory).entryList(QStringList() << "*.xml",
                                                                                                QDir::Files | QDir::Readable | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    foreach (const QString &skin_file, skin_files) {
      // Check if skin file is valid and add it if it is valid.
      Skin skin_info = getSkinInfo(base_directory + QDir::separator() + skin_file,
                                   &skin_load_ok);

      if (skin_load_ok) {
        skins.append(skin_info);
      }
    }
  }

  return skins;
}
