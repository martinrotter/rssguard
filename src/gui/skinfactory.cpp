#include <QApplication>
#include <QDomDocument>
#include <QDir>
#include <QXmlQuery>
#include <QStyleFactory>

#include "core/defs.h"
#include "core/settings.h"
#include "gui/skinfactory.h"


QPointer<SkinFactory> SkinFactory::s_instance;

SkinFactory::SkinFactory(QObject *parent)
  : QObject(parent), m_currentSkin(APP_THEME_SYSTEM) {
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
  bool loaded = false;
  Skin skin_data = getSkinInfo(skin_name_from_settings, &loaded);

  if (skin_name_from_settings == APP_THEME_SYSTEM) {
    // NOTE: No need to call qApp->setStylesheet(QString()) here.
    qDebug("'Default system skin' loaded.");
  }
  else if (loaded) {
    loadSkinFromData(skin_data.m_rawData, skin_name_from_settings);

    foreach (QString style, skin_data.m_stylesNames) {
      if (qApp->setStyle(style) != 0) {
        qDebug("Style '%s' loaded.", qPrintable(style));
        break;
      }
    }

    m_currentSkin = skin_name_from_settings;

    qDebug("Skin '%s' loaded.", qPrintable(skin_name_from_settings));
  }
  else {
    qDebug("Skin '%s' not loaded because style name is not specified or skin raw data is missing. Default skin loaded.",
           qPrintable(skin_name_from_settings));
  }
}

bool SkinFactory::loadSkinFromData(QString skin_data, const QString &skin_path) {
  QStringList skin_parts = skin_path.split('/', QString::SkipEmptyParts);

  // Skin does not contain leading folder name or the actual skin file name.
  if (skin_parts.size() != 2) {
    qDebug("Loading of sking %s failed because skin name does not contain "
           "base folder name or the actual skin name.",
           qPrintable(skin_path));
    return false;
  }
  else {
    qDebug("Loading skin '%s'.", qPrintable(skin_path));
  }

  // Create needed variables and create QFile object representing skin contents.
  QString skin_folder = skin_parts.at(0);

  // Here we use "/" instead of QDir::separator() because CSS2.1 url field
  // accepts '/' as path elements separator.
  //
  // "##" is placeholder for the actual path to skin file. This is needed for using
  // images within the QSS file.
  QString parsed_data = skin_data.replace("##",
                                          APP_SKIN_PATH + "/" + skin_folder + "/images");
  qApp->setStyleSheet(parsed_data);
  return true;
}

void SkinFactory::setCurrentSkinName(const QString &skin_name) {
  Settings::getInstance()->setValue(APP_CFG_GUI, "skin", skin_name);
}

QString SkinFactory::getSelectedSkinName() {
  return Settings::getInstance()->value(APP_CFG_GUI,
                                        "skin",
                                        APP_THEME_SYSTEM).toString();
}

QString SkinFactory::getCurrentSkinName() {
  return m_currentSkin;
}

Skin SkinFactory::getSkinInfo(const QString &skin_name, bool *ok) {
  Skin skin;

  if (skin_name == APP_THEME_SYSTEM) {
    skin.m_author = "-";
    skin.m_baseName = APP_THEME_SYSTEM;
    skin.m_email = "-";
    skin.m_version = "-";
    skin.m_visibleName = tr("default system skin");

    if (ok != NULL) {
      *ok = true;
    }

    return skin;
  }

  QXmlQuery query;
  QFile skin_file(APP_SKIN_PATH + QDir::separator() + skin_name);

  if (!skin_file.open(QIODevice::ReadOnly) || !query.setFocus(&skin_file)) {
    if (ok) {
      *ok = false;
    }
    return skin;
  }

  // Obtain visible skin name.
  query.setQuery("string(skin/name)");
  query.evaluateTo(&skin.m_visibleName);
  skin.m_visibleName = skin.m_visibleName.remove("\n");

  // Obtain skin raw data.
  query.setQuery("string(skin/data)");
  query.evaluateTo(&skin.m_rawData);

  // Obtain style name.
  query.setQuery("string(/skin/style)");
  QString styles;
  query.evaluateTo(&styles);
  skin.m_stylesNames = styles.remove("\n").split(",");

  // Obtain author.
  query.setQuery("string(/skin/author/name)");
  query.evaluateTo(&skin.m_author);
  skin.m_author = skin.m_author.remove("\n");

  // Obtain email.
  query.setQuery("string(/skin/author/email)");
  query.evaluateTo(&skin.m_email);
  skin.m_email = skin.m_email.remove("\n");

  // Obtain version.
  query.setQuery("string(/skin/@version)");
  query.evaluateTo(&skin.m_version);
  skin.m_version = skin.m_version.remove("\n");

  // Obtain other information.
  skin.m_baseName = skin_name;

  // Free resources.
  skin_file.close();
  skin_file.deleteLater();

  if (ok) {
    *ok = !skin.m_author.isEmpty() && !skin.m_version.isEmpty() &&
          !skin.m_baseName.isEmpty() && !skin.m_email.isEmpty() &&
          !skin.m_rawData.isEmpty() && !skin.m_stylesNames.isEmpty();
  }

  return skin;
}

QList<Skin> SkinFactory::getInstalledSkins() {
  QList<Skin> skins;

  skins.append(getSkinInfo(APP_THEME_SYSTEM));

  bool skin_load_ok;
  QStringList skin_directories = QDir(APP_SKIN_PATH).entryList(QDir::Dirs |
                                                               QDir::NoDotAndDotDot |
                                                               QDir::NoSymLinks |
                                                               QDir::Readable);

  foreach (QString base_directory, skin_directories) {
    // Check skins installed in this base directory.
    QStringList skin_files = QDir(APP_SKIN_PATH + QDir::separator() + base_directory).entryList(QStringList() << "*.xml",
                                                                                                QDir::Files | QDir::Readable | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    foreach (QString skin_file, skin_files) {
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
