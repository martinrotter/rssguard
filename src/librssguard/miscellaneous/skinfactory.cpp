// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/skinfactory.h"

#include "exceptions/ioexception.h"
#include "miscellaneous/application.h"

#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QMetaEnum>
#include <QMetaObject>
#include <QProcessEnvironment>
#include <QStyleFactory>
#include <QToolTip>

SkinFactory::SkinFactory(QObject* parent) : QObject(parent), m_styleIsFrozen(false) {}

void SkinFactory::loadCurrentSkin() {
  QList<QString> skin_names_to_try = { selectedSkinName(), QSL(APP_SKIN_DEFAULT) };
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
  static QRegularExpression re = QRegularExpression("^(fusion)|(qt[56]ct-style)$");

  return re.match(style_name.toLower()).hasMatch();
}

void SkinFactory::loadSkinFromData(const Skin& skin) {
  QString style_name = qApp->settings()->value(GROUP(GUI), SETTING(GUI::Style)).toString();
  auto env = QProcessEnvironment::systemEnvironment();
  const QString env_forced_style = env.value(QSL("QT_STYLE_OVERRIDE"));
  const QString cli_forced_style = qApp->cmdParser()->value(QSL(CLI_STYLE_SHORT));

  if (env_forced_style.isEmpty() && cli_forced_style.isEmpty()) {
    qApp->setStyle(style_name);
    m_styleIsFrozen = false;

    qDebugNN << LOGSEC_GUI << "Setting style:" << QUOTE_W_SPACE_DOT(style_name);
  }
  else {
    m_styleIsFrozen = true;
    qWarningNN << LOGSEC_GUI << "Respecting forced style(s):\n"
               << "  QT_STYLE_OVERRIDE: " QUOTE_NO_SPACE(env_forced_style) << "\n"
               << "  CLI (-style): " QUOTE_NO_SPACE(cli_forced_style);
  }

  if (isStyleGoodForDarkVariant(style_name) &&
      qApp->settings()->value(GROUP(GUI), SETTING(GUI::ForceDarkFusion)).toBool()) {
    qDebugNN << LOGSEC_GUI << "Activating dark palette for Fusion style.";

    QPalette fusion_palette = qApp->palette();
    QColor clr_maibg(QSL("#2D2F32"));
    QColor clr_basbg(QSL("#373A3D"));
    QColor clr_altbg(QSL("#323437"));
    QColor clr_selbg(QSL("#8291AD"));
    QColor clr_selfg(QSL("#FFFFFF"));
    QColor clr_btnfg(QSL("#E7E7E7"));
    QColor clr_dibfg(QSL("#A7A7A7"));
    QColor clr_winfg(QSL("#D8D8D8"));
    QColor clr_diwfg(QSL("#999999"));
    QColor clr_brdbg(QSL("#202224")); // Use colour picker on dark brdr under list header for this one
    QColor clr_wlink(QSL("#a1acc1"));

    //
    // Normal state.
    //

    // Backgrounds & bases.
    fusion_palette.setColor(QPalette::ColorRole::Window, clr_maibg);
    fusion_palette.setColor(QPalette::ColorRole::Base, clr_basbg);
    fusion_palette.setColor(QPalette::ColorRole::Dark, clr_brdbg);
    fusion_palette.setColor(QPalette::ColorRole::AlternateBase, clr_altbg);
    fusion_palette.setColor(QPalette::ColorRole::Button, clr_altbg);
    fusion_palette.setColor(QPalette::ColorRole::Light, clr_altbg);
    fusion_palette.setColor(QPalette::ColorRole::Highlight, clr_selbg);

    // Texts.
    fusion_palette.setColor(QPalette::ColorRole::ButtonText, clr_btnfg);
    fusion_palette.setColor(QPalette::ColorRole::WindowText, clr_winfg);
    fusion_palette.setColor(QPalette::ColorRole::BrightText, clr_basbg);
    fusion_palette.setColor(QPalette::ColorRole::Text, clr_winfg);
    fusion_palette.setColor(QPalette::ColorRole::PlaceholderText, clr_dibfg);
    fusion_palette.setColor(QPalette::ColorRole::Link, clr_wlink);
    fusion_palette.setColor(QPalette::ColorRole::LinkVisited, clr_wlink);
    fusion_palette.setColor(QPalette::ColorRole::HighlightedText, clr_selfg);

    //
    // Inactive state.
    //

    // Backgrounds & bases.

    // Texts.

    //
    // Disabled state.
    //

    // Backgrounds & bases.
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Window, clr_maibg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Base, clr_basbg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Dark, clr_brdbg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::AlternateBase, clr_altbg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Button, clr_altbg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Light, clr_altbg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Highlight, clr_selbg);

    // Texts.
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::ButtonText, clr_dibfg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::WindowText, clr_diwfg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::BrightText, clr_basbg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Text, clr_diwfg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::PlaceholderText, clr_dibfg);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Link, clr_wlink);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::LinkVisited, clr_wlink);
    fusion_palette.setColor(QPalette::ColorGroup::Disabled, QPalette::ColorRole::HighlightedText, clr_selfg);

    //
    // Tooltips.
    //

    fusion_palette.setColor(QPalette::ColorGroup::All, QPalette::ColorRole::ToolTipBase, clr_maibg);
    fusion_palette.setColor(QPalette::ColorGroup::All, QPalette::ColorRole::ToolTipText, clr_winfg);

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
  const QStringList skins_root_folders = {
    APP_SKIN_PATH,
    customSkinBaseFolder()
  };

  for (const QString& skins_root_folder : skins_root_folders) {
    const QString skin_parent = QString(skins_root_folder).replace(QDir::separator(), QL1C('/')) + QL1C('/');
    const QString skin_folder_no_sep = skin_parent + skin_name;
    const QString skin_folder_with_sep = skin_folder_no_sep + QDir::separator();
    const QString metadata_file = skin_folder_with_sep + APP_SKIN_METADATA_FILE;

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
      const QString base_skin_name = skin_node.toElement().attribute(QSL("base"));;
      QString real_base_skin_folder;

      if (!base_skin_name.isEmpty()) {
        // We now find folder of "base" skin.
        for (const QString& parent_for_base : skins_root_folders) {
          QString full_base = parent_for_base + QDir::separator() + base_skin_name;

          if (QDir().exists(full_base)) {
            real_base_skin_folder = full_base;
            real_base_skin_folder = real_base_skin_folder.replace(QDir::separator(), QL1C('/'));

            qDebugNN << LOGSEC_GUI << "Found path of base skin:"
                     << QUOTE_W_SPACE_DOT(QDir::toNativeSeparators(real_base_skin_folder));
            break;
          }
        }

        if (real_base_skin_folder.isEmpty()) {
          // Base skin not found.
          if (ok != nullptr) {
            *ok = false;
          }

          qCriticalNN << LOGSEC_GUI << "Base skin" << QUOTE_W_SPACE(base_skin_name) << "not found.";
          return skin;
        }
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
      QHash<SkinEnums::PaletteColors, QColor> palette;
      QDomNodeList colors_of_palette = skin_node.namedItem(QSL("palette")).toElement().elementsByTagName(QSL("color"));
      const QMetaObject& mo = SkinEnums::staticMetaObject;
      QMetaEnum enumer = mo.enumerator(mo.indexOfEnumerator(QSL("PaletteColors").toLocal8Bit().constData()));

      for (int i = 0; i < colors_of_palette.size(); i++) {
        QDomElement elem_clr = colors_of_palette.item(i).toElement();
        QString en_val = elem_clr.attribute(QSL("key"));
        SkinEnums::PaletteColors key = SkinEnums::PaletteColors(enumer.keyToValue(en_val.toLatin1()));
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
      skin.m_layoutMarkupWrapper = loadSkinFile(skin_folder_no_sep, QSL("html_wrapper.html"), real_base_skin_folder);

      try {
        auto custom_css = loadSkinFile(skin_folder_no_sep, QSL("html_style.css"), real_base_skin_folder);

        skin.m_layoutMarkupWrapper = skin.m_layoutMarkupWrapper.replace(QSL(SKIN_STYLE_PLACEHOLDER), custom_css);
      }
      catch (...) {
        qWarningNN << "Skin"
                   << QUOTE_W_SPACE(skin_name)
                   << "does not support separated custom CSS.";
      }

      skin.m_enclosureImageMarkup = loadSkinFile(skin_folder_no_sep, QSL("html_enclosure_image.html"), real_base_skin_folder);
      skin.m_layoutMarkup = loadSkinFile(skin_folder_no_sep, QSL("html_single_message.html"), real_base_skin_folder);
      skin.m_enclosureMarkup = loadSkinFile(skin_folder_no_sep, QSL("html_enclosure_every.html"), real_base_skin_folder);
      skin.m_rawData = loadSkinFile(skin_folder_no_sep, QSL("qt_style.qss"), real_base_skin_folder);
      skin.m_adblocked = loadSkinFile(skin_folder_no_sep, QSL("html_adblocked.html"), real_base_skin_folder);

      if (ok != nullptr) {
        *ok = !skin.m_author.isEmpty() &&
              !skin.m_version.isEmpty() &&
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
    return data.replace(QSL(USER_DATA_PLACEHOLDER), skin_folder);
  }
  else {
    qDebugNN << LOGSEC_GUI << "Trying to load base file" << QUOTE_W_SPACE(base_file) << "for the skin.";
    data = QString::fromUtf8(IOFactory::readFile(base_file));
    return data.replace(QSL(USER_DATA_PLACEHOLDER), base_folder);
  }
}

bool SkinFactory::styleIsFrozen() const {
  return m_styleIsFrozen;
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

uint qHash(const SkinEnums::PaletteColors& key) {
  return uint(key);
}

QVariant Skin::colorForModel(SkinEnums::PaletteColors type, bool ignore_custom_colors) const {
  if (!ignore_custom_colors) {
    bool enabled = qApp->settings()->value(GROUP(CustomSkinColors),
                                           SETTING(CustomSkinColors::Enabled)).toBool();

    if (enabled) {
      const QMetaObject& mo = SkinEnums::staticMetaObject;
      QMetaEnum enumer = mo.enumerator(mo.indexOfEnumerator(QSL("PaletteColors").toLocal8Bit().constData()));
      QColor custom_clr = qApp->settings()->value(GROUP(CustomSkinColors), enumer.valueToKey(int(type))).toString();

      if (custom_clr.isValid()) {
        return custom_clr;
      }
    }
  }

  return m_colorPalette.contains(type)
      ? m_colorPalette[type]
      : QVariant();
}

QString SkinEnums::palleteColorText(PaletteColors col) {
  switch (col) {
    case SkinEnums::PaletteColors::FgInteresting:
      return QObject::tr("interesting stuff");

    case SkinEnums::PaletteColors::FgSelectedInteresting:
      return QObject::tr("interesting stuff (highlighted)");;

    case SkinEnums::PaletteColors::FgError:
      return QObject::tr("errored items");

    case SkinEnums::PaletteColors::FgSelectedError:
      return QObject::tr("errored items (highlighted)");

    case SkinEnums::PaletteColors::Allright:
      return QObject::tr("OK-ish color");

    default:
      return {};
  }
}
