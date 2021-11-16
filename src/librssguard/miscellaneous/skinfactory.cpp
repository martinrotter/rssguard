// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/skinfactory.h"

#include "exceptions/ioexception.h"
#include "miscellaneous/application.h"

#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QStyleFactory>
#include <QToolTip>

SkinFactory::SkinFactory(QObject* parent) : QObject(parent) {}

void SkinFactory::loadCurrentSkin() {
  QList<QString> skin_names_to_try;

  skin_names_to_try.append(selectedSkinName());
  skin_names_to_try.append(QSL(APP_SKIN_DEFAULT));
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

bool SkinFactory::isStyleGoodForDarkVariant(const QString& style_name) const {
  return QRegularExpression("^(fusion)|(qt5ct-style)$").match(style_name.toLower()).hasMatch();
}

void SkinFactory::loadSkinFromData(const Skin& skin) {
  QString style_name = qApp->settings()->value(GROUP(GUI), SETTING(GUI::Style)).toString();

  qApp->setStyle(style_name);

  if (isStyleGoodForDarkVariant(style_name) &&
      qApp->settings()->value(GROUP(GUI), SETTING(GUI::ForceDarkFusion)).toBool()) {
    qDebugNN << LOGSEC_GUI << "Activating dark palette for Fusion style.";

    QPalette fusion_palette = qApp->palette();
    QColor clr_bg(QSL("#2D2F32"));
    QColor clr_altbg(QSL("#323437"));
    QColor clr_selbg(QSL("#8291AD"));
    QColor clr_fg(QSL("#D8D8D8"));
    QColor clr_brdr(QSL("#585C65"));
    QColor clr_tooltip_brdr(QSL("#707580"));
    QColor clr_link(QSL("#a1acc1"));
    QColor clr_dis_fg(QSL("#727272"));

    //
    // Normal state.
    //

    // Backgrounds & bases.
    fusion_palette.setColor(QPalette::ColorRole::Window, clr_bg);
    fusion_palette.setColor(QPalette::ColorRole::Base, clr_bg);
    fusion_palette.setColor(QPalette::ColorRole::Dark, clr_bg);
    fusion_palette.setColor(QPalette::ColorRole::AlternateBase, clr_altbg);
    fusion_palette.setColor(QPalette::ColorRole::Button, clr_altbg);
    fusion_palette.setColor(QPalette::ColorRole::Highlight, clr_selbg);

    // Texts.
    fusion_palette.setColor(QPalette::ColorRole::WindowText, clr_fg);
    fusion_palette.setColor(QPalette::ColorRole::ButtonText, clr_fg);
    fusion_palette.setColor(QPalette::ColorRole::BrightText, clr_fg);
    fusion_palette.setColor(QPalette::ColorRole::Text, clr_fg);
    fusion_palette.setColor(QPalette::ColorRole::PlaceholderText, clr_fg);
    fusion_palette.setColor(QPalette::ColorRole::Link, clr_link);
    fusion_palette.setColor(QPalette::ColorRole::LinkVisited, clr_link);
    fusion_palette.setColor(QPalette::ColorRole::HighlightedText, clr_fg);

    //
    // Inactive state.
    //

    // Backgrounds & bases.

    // Texts.

    //
    // Disabled state.
    //

    // Backgrounds & bases.
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Window, clr_altbg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Base, clr_altbg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Dark, clr_altbg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::AlternateBase, clr_altbg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, Qt::GlobalColor::red);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Highlight, clr_selbg);

    // Texts.
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::WindowText, clr_dis_fg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::ButtonText, clr_dis_fg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::BrightText, clr_fg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Text, clr_dis_fg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::PlaceholderText, clr_fg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Link, clr_link);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::LinkVisited, clr_link);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::HighlightedText, clr_fg);

    //
    // Tooltips.
    //

    fusion_palette.setColor(QPalette::ColorGroup::All, QPalette::ColorRole::ToolTipBase, clr_bg);
    fusion_palette.setColor(QPalette::ColorGroup::All, QPalette::ColorRole::ToolTipText, clr_fg);

    QToolTip::setPalette(fusion_palette);
    qApp->setPalette(fusion_palette);
  }

  if (!skin.m_rawData.isEmpty()) {
    if (qApp->styleSheet().simplified().isEmpty()) {
      qApp->setStyleSheet(skin.m_rawData);
    }
    else {
      qCriticalNN << LOGSEC_GUI
                  << "Skipped setting of application style and skin because there is already some style set.";
    }
  }
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
    const QString skin_parent = base_skin_folders.takeAt(0).replace(QDir::separator(), QL1C('/')) + QL1C('/');
    const QString skin_folder_no_sep = skin_parent + skin_name;
    const QString skin_folder = skin_folder_no_sep + QDir::separator();
    const QString metadata_file = skin_folder + APP_SKIN_METADATA_FILE;

    if (QFile::exists(metadata_file)) {
      QFile skin_file(metadata_file);
      QDomDocument document;

      if (!skin_file.open(QIODevice::OpenModeFlag::Text | QIODevice::OpenModeFlag::ReadOnly) ||
          !document.setContent(&skin_file, true)) {
        if (ok != nullptr) {
          *ok = false;
        }

        return skin;
      }

      const QDomNode skin_node = document.namedItem(QSL("skin"));
      QString base_skin_folder = skin_node.toElement().attribute(QSL("base"));

      if (!base_skin_folder.isEmpty()) {
        base_skin_folder = skin_parent + base_skin_folder;
      }

      // Obtain visible skin name.
      skin.m_visibleName = skin_name;

      // Obtain author.
      skin.m_author = skin_node.namedItem(QSL("author")).namedItem(QSL("name")).toElement().text();

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
      //
      // %style% placeholder is used in main wrapper HTML file to be replaced with custom skin-wide CSS.
      skin.m_layoutMarkupWrapper = loadSkinFile(skin_folder_no_sep, QSL("html_wrapper.html"), base_skin_folder);

      try {
        auto custom_css = loadSkinFile(skin_folder_no_sep, QSL("html_style.css"), base_skin_folder);

        skin.m_layoutMarkupWrapper = skin.m_layoutMarkupWrapper.replace(QSL(SKIN_STYLE_PLACEHOLDER), custom_css);
      }
      catch (...) {
        qWarningNN << "Skin"
                   << QUOTE_W_SPACE(skin_name)
                   << "does not support separated custom CSS.";
      }

      skin.m_enclosureImageMarkup = loadSkinFile(skin_folder_no_sep, QSL("html_enclosure_image.html"), base_skin_folder);
      skin.m_layoutMarkup = loadSkinFile(skin_folder_no_sep, QSL("html_single_message.html"), base_skin_folder);
      skin.m_enclosureMarkup = loadSkinFile(skin_folder_no_sep, QSL("html_enclosure_every.html"), base_skin_folder);
      skin.m_rawData = loadSkinFile(skin_folder_no_sep, QSL("qt_style.qss"), base_skin_folder);
      skin.m_adblocked = loadSkinFile(skin_folder_no_sep, QSL("html_adblocked.html"), base_skin_folder);

      if (ok != nullptr) {
        *ok = !skin.m_author.isEmpty() && !skin.m_version.isEmpty() &&
              !skin.m_baseName.isEmpty() &&
              !skin.m_layoutMarkup.isEmpty();
      }

      return skin;
    }
  }

  if (ok != nullptr) {
    *ok = false;
  }

  return skin;
}

QString SkinFactory::loadSkinFile(const QString& skin_folder, const QString& file_name, const QString& base_folder) const {
  QString local_file = QDir::toNativeSeparators(skin_folder + QDir::separator() + file_name);
  QString base_file = QDir::toNativeSeparators(base_folder + QDir::separator() + file_name);
  QString data;

  if (QFile::exists(local_file)) {
    qDebugNN << LOGSEC_GUI << "Local file" << QUOTE_W_SPACE(local_file) << "exists, using it for the skin.";
    data = QString::fromUtf8(IOFactory::readFile(local_file));
  }
  else {
    qDebugNN << LOGSEC_GUI << "Trying to load base file" << QUOTE_W_SPACE(base_file) << "for the skin.";
    data = QString::fromUtf8(IOFactory::readFile(base_file));
  }

  return data.replace(QSL(USER_DATA_PLACEHOLDER), skin_folder);
}

QList<Skin> SkinFactory::installedSkins() const {
  QList<Skin> skins;
  bool skin_load_ok;
  QStringList skin_directories = QDir(APP_SKIN_PATH).entryList(QDir::Filter::Dirs |
                                                               QDir::Filter::NoDotAndDotDot |
                                                               QDir::Filter::Readable);

  skin_directories.append(QDir(customSkinBaseFolder()).entryList(QDir::Filter::Dirs |
                                                                 QDir::Filter::NoDotAndDotDot |
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
