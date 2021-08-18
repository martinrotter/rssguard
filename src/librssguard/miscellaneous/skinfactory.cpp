// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/skinfactory.h"

#include "miscellaneous/application.h"

#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QStyleFactory>

SkinFactory::SkinFactory(QObject* parent) : QObject(parent) {}

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
      qDebugNN << LOGSEC_GUI << "Skin" << QUOTE_W_SPACE(skin_name) << "loaded.";
      return;
    }
    else {
      qWarningNN << LOGSEC_GUI << "Failed to load skin" << QUOTE_W_SPACE_DOT(skin_name);
    }
  }

  qCriticalNN << LOGSEC_GUI << "Failed to load selected or default skin. Quitting!";
}

void SkinFactory::loadSkinFromData(const Skin& skin) {
  if (!skin.m_rawData.isEmpty()) {
    if (qApp->styleSheet().simplified().isEmpty()) {
      qApp->setStyleSheet(skin.m_rawData);
    }
    else {
      qCriticalNN << LOGSEC_GUI
                  << "Skipped setting of application style and skin because there is already some style set.";
    }
  }

  qApp->setStyle(qApp->settings()->value(GROUP(GUI), SETTING(GUI::Style)).toString());
}

void SkinFactory::setCurrentSkinName(const QString& skin_name) {
  qApp->settings()->setValue(GROUP(GUI), GUI::Skin, skin_name);
}

QString SkinFactory::customSkinBaseFolder() const {
  return qApp->userDataFolder() + QDir::separator() + APP_SKIN_USER_FOLDER;
}

QString SkinFactory::selectedSkinName() const {
  return qApp->settings()->value(GROUP(GUI), SETTING(GUI::Skin)).toString();
}

QString SkinFactory::adBlockedPage(const QString& url, const QString& filter) {
  const QString& adblocked = currentSkin().m_adblocked.arg(tr("This page was blocked by AdBlock"),
                                                           tr(R"(Blocked URL: "%1"<br/>Used filter: "%2")").arg(url,
                                                                                                                filter));

  return currentSkin().m_layoutMarkupWrapper.arg(tr("This page was blocked by AdBlock"), adblocked);
}

Skin SkinFactory::skinInfo(const QString& skin_name, bool* ok) const {
  Skin skin;
  QStringList base_skin_folders;

  base_skin_folders.append(APP_SKIN_PATH);
  base_skin_folders.append(customSkinBaseFolder());

  while (!base_skin_folders.isEmpty()) {
    const QString skin_folder_no_sep = base_skin_folders.takeAt(0).replace(QDir::separator(),
                                                                           QL1C('/')) + QL1C('/') + skin_name;
    const QString skin_folder = skin_folder_no_sep + QDir::separator();
    const QString metadata_file = skin_folder + APP_SKIN_METADATA_FILE;

    if (QFile::exists(metadata_file)) {
      QFile skin_file(metadata_file);
      QDomDocument dokument;

      if (!skin_file.open(QIODevice::OpenModeFlag::Text | QIODevice::OpenModeFlag::ReadOnly) ||
          !dokument.setContent(&skin_file, true)) {
        if (ok != nullptr) {
          *ok = false;
        }

        return skin;
      }

      const QDomNode skin_node = dokument.namedItem(QSL("skin"));

      // Obtain visible skin name.
      skin.m_visibleName = skin_name;

      // Obtain author.
      skin.m_author = skin_node.namedItem(QSL("author")).namedItem(QSL("name")).toElement().text();

      // Obtain email.
      skin.m_email = skin_node.namedItem(QSL("author")).namedItem(QSL("email")).toElement().text();

      // Obtain version.
      skin.m_version = skin_node.attributes().namedItem(QSL("version")).toAttr().value();

      // Obtain other information.
      skin.m_baseName = skin_name;

      // Obtain color palette.
      QHash<Skin::PaletteColors, QColor> palette;
      QDomNodeList colors_of_palette = skin_node.namedItem(QSL("palette")).toElement().elementsByTagName(QSL("color"));

      for (int i = 0; i < colors_of_palette.size(); i++) {
        QDomElement elem_clr = colors_of_palette.item(i).toElement();

        Skin::PaletteColors key = Skin::PaletteColors(elem_clr.attribute(QSL("key")).toInt());
        QColor value = elem_clr.text();

        if (value.isValid()) {
          palette.insert(key, value);
        }
      }

      skin.m_colorPalette = palette;

      // Free resources.
      skin_file.close();
      skin_file.deleteLater();

      // Here we use "/" instead of QDir::separator() because CSS2.1 url field
      // accepts '/' as path elements separator.
      //
      // USER_DATA_PLACEHOLDER is placeholder for the actual path to skin folder. This is needed for using
      // images within the QSS file.
      // So if one uses "%data%/images/border.png" in QSS then it is
      // replaced by fully absolute path and target file can
      // be safely loaded.
      skin.m_layoutMarkupWrapper = QString::fromUtf8(IOFactory::readFile(skin_folder + QL1S("html_wrapper.html")));
      skin.m_layoutMarkupWrapper = skin.m_layoutMarkupWrapper.replace(QSL(USER_DATA_PLACEHOLDER),
                                                                      skin_folder_no_sep);
      skin.m_enclosureImageMarkup = QString::fromUtf8(IOFactory::readFile(skin_folder + QL1S("html_enclosure_image.html")));
      skin.m_enclosureImageMarkup = skin.m_enclosureImageMarkup.replace(QSL(USER_DATA_PLACEHOLDER),
                                                                        skin_folder_no_sep);
      skin.m_layoutMarkup = QString::fromUtf8(IOFactory::readFile(skin_folder + QL1S("html_single_message.html")));
      skin.m_layoutMarkup = skin.m_layoutMarkup.replace(QSL(USER_DATA_PLACEHOLDER),
                                                        skin_folder_no_sep);
      skin.m_enclosureMarkup = QString::fromUtf8(IOFactory::readFile(skin_folder + QL1S("html_enclosure_every.html")));
      skin.m_enclosureMarkup = skin.m_enclosureMarkup.replace(QSL(USER_DATA_PLACEHOLDER),
                                                              skin_folder_no_sep);
      skin.m_rawData = QString::fromUtf8(IOFactory::readFile(skin_folder + QL1S("theme.css")));
      skin.m_rawData = skin.m_rawData.replace(QSL(USER_DATA_PLACEHOLDER),
                                              skin_folder_no_sep);
      skin.m_adblocked = QString::fromUtf8(IOFactory::readFile(skin_folder + QL1S("html_adblocked.html")));
      skin.m_adblocked = skin.m_adblocked.replace(QSL(USER_DATA_PLACEHOLDER),
                                                  skin_folder_no_sep);

      if (ok != nullptr) {
        *ok = !skin.m_author.isEmpty() && !skin.m_version.isEmpty() &&
              !skin.m_baseName.isEmpty() && !skin.m_email.isEmpty() &&
              !skin.m_layoutMarkup.isEmpty();
      }

      break;
    }
  }

  return skin;
}

QList<Skin> SkinFactory::installedSkins() const {
  QList<Skin> skins;
  bool skin_load_ok;
  QStringList skin_directories = QDir(APP_SKIN_PATH).entryList(QDir::Filter::Dirs |
                                                               QDir::Filter::NoDotAndDotDot |
                                                               QDir::Filter::NoSymLinks |
                                                               QDir::Filter::Readable);

  skin_directories.append(QDir(customSkinBaseFolder()).entryList(QDir::Filter::Dirs |
                                                                 QDir::Filter::NoDotAndDotDot |
                                                                 QDir::Filter::NoSymLinks |
                                                                 QDir::Filter::Readable));

  for (const QString& base_directory : skin_directories) {
    const Skin skin_info = skinInfo(base_directory, &skin_load_ok);

    if (skin_load_ok) {
      skins.append(skin_info);
    }
  }

  return skins;
}

uint qHash(const Skin::PaletteColors& key) {
  return uint(key);
}
