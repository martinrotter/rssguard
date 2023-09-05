// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/skinfactory.h"

#include "miscellaneous/application.h"
#include "network-web/networkfactory.h"
#include "services/abstract/rootitem.h"

#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFontDatabase>
#include <QMetaEnum>
#include <QMetaObject>
#include <QProcessEnvironment>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTextDocument>
#include <QToolTip>

SkinFactory::SkinFactory(QObject* parent) : QObject(parent), m_styleIsFrozen(false), m_useSkinColors(false) {}

void SkinFactory::loadCurrentSkin() {
  QList<QString> skin_names_to_try = {selectedSkinName(), QSL(APP_SKIN_DEFAULT)};
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

QVariant SkinFactory::colorForModel(SkinEnums::PaletteColors type, bool ignore_custom_colors) const {
  return m_currentSkin.colorForModel(type, m_useSkinColors, ignore_custom_colors);
}

bool SkinFactory::isStyleGoodForAlternativeStylePalette(const QString& style_name) const {
  static QRegularExpression re = QRegularExpression(QSL("^(fusion|windows|qt[56]ct-style)$"));

  return re.match(style_name.toLower()).hasMatch();
}

// NOTE: Taken from "QPlatformThemePrivate" Qt class.
// This is here because in Qt 6.5.0, they hardcoded
// DARK palette if user has enabled "dark mode" in OS.
QPalette qt_fusionPalette(bool dark_appearance) {
  const QColor window_text = dark_appearance ? QColor(240, 240, 240) : Qt::black;
  const QColor background = dark_appearance ? QColor(50, 50, 50) : QColor(239, 239, 239);
  const QColor light = background.lighter(150);
  const QColor mid = (background.darker(130));
  const QColor midlight = mid.lighter(110);
  const QColor base = dark_appearance ? background.darker(140) : Qt::white;
  const QColor disabled_base(background);
  const QColor dark = background.darker(150);
  const QColor disabled_dark = QColor(209, 209, 209).darker(110);
  const QColor text = dark_appearance ? window_text : Qt::black;
  const QColor highlight = QColor(48, 140, 198);
  const QColor hightlighted_text = dark_appearance ? window_text : Qt::white;
  const QColor disabled_text = dark_appearance ? QColor(130, 130, 130) : QColor(190, 190, 190);
  const QColor button = background;
  const QColor shadow = dark.darker(135);
  const QColor disabled_shadow = shadow.lighter(150);

  QColor placeholder = text;
  placeholder.setAlpha(128);

  QPalette fusion_palette(window_text, background, light, dark, mid, text, base);
  fusion_palette.setBrush(QPalette::Midlight, midlight);
  fusion_palette.setBrush(QPalette::Button, button);
  fusion_palette.setBrush(QPalette::Shadow, shadow);
  fusion_palette.setBrush(QPalette::HighlightedText, hightlighted_text);

  fusion_palette.setBrush(QPalette::Disabled, QPalette::Text, disabled_text);
  fusion_palette.setBrush(QPalette::Disabled, QPalette::WindowText, disabled_text);
  fusion_palette.setBrush(QPalette::Disabled, QPalette::ButtonText, disabled_text);
  fusion_palette.setBrush(QPalette::Disabled, QPalette::Base, disabled_base);
  fusion_palette.setBrush(QPalette::Disabled, QPalette::Dark, disabled_dark);
  fusion_palette.setBrush(QPalette::Disabled, QPalette::Shadow, disabled_shadow);

  fusion_palette.setBrush(QPalette::Active, QPalette::Highlight, highlight);
  fusion_palette.setBrush(QPalette::Inactive, QPalette::Highlight, highlight);
  fusion_palette.setBrush(QPalette::Disabled, QPalette::Highlight, QColor(145, 145, 145));

  fusion_palette.setBrush(QPalette::PlaceholderText, placeholder);

  if (dark_appearance) {
    fusion_palette.setBrush(QPalette::Link, highlight);
  }

  return fusion_palette;
}

void SkinFactory::loadSkinFromData(const Skin& skin) {
#if QT_VERSION >= 0x060500 // Qt >= 6.5.0
  auto system_color_scheme = qApp->styleHints()->colorScheme();

  qDebugNN << LOGSEC_GUI << "OS defines color scheme:" << QUOTE_W_SPACE_DOT(system_color_scheme);
#endif

  QString style_name = qApp->settings()->value(GROUP(GUI), SETTING(GUI::Style)).toString();
  auto env = QProcessEnvironment::systemEnvironment();
  const QString env_forced_style = env.value(QSL("QT_STYLE_OVERRIDE"));
  const QString cli_forced_style = qApp->cmdParser()->value(QSL(CLI_STYLE_SHORT));

  // Load fonts.
  QDir dr_fonts(skin.m_skinFolder + QDir::separator() + QSL("fonts"));

  if (dr_fonts.exists()) {
    QStringList ttf_fonts =
      dr_fonts.entryList({QSL("*.ttf"), QSL("*.otf")}, QDir::Filter::Files | QDir::Filter::Readable);

    for (const QString& font_file : ttf_fonts) {
      bool added = QFontDatabase::addApplicationFont(dr_fonts.absoluteFilePath(font_file)) >= 0;

      if (added) {
        qDebugNN << "Adding font" << QUOTE_W_SPACE(font_file) << "to font database.";
      }
      else {
        qCriticalNN << "Font" << QUOTE_W_SPACE(font_file) << "could not be loaded.";
      }
    }
  }

  if (skin.m_defaultFont != QApplication::font()) {
    QApplication::setFont(skin.m_defaultFont);
    qDebugNN << "Activating custom application default font" << QUOTE_W_SPACE_DOT(skin.m_defaultFont.toString());
  }

  if (env_forced_style.isEmpty() && cli_forced_style.isEmpty()) {
    m_styleIsFrozen = false;

    if (!skin.m_forcedStyles.isEmpty()) {
      qDebugNN << LOGSEC_GUI << "Forcing one of skin's declared styles:" << QUOTE_W_SPACE_DOT(skin.m_forcedStyles);

      for (const QString& skin_forced_style : skin.m_forcedStyles) {
        if (qApp->setStyle(skin_forced_style) != nullptr) {
          break;
        }
      }
    }
    else {
      qDebugNN << LOGSEC_GUI << "Setting style:" << QUOTE_W_SPACE_DOT(style_name);
      qApp->setStyle(style_name);
    }
  }
  else {
    m_styleIsFrozen = true;

    qWarningNN << LOGSEC_GUI << "Respecting forced style(s):\n"
               << "  QT_STYLE_OVERRIDE: " QUOTE_NO_SPACE(env_forced_style) << "\n"
               << "  CLI (-style): " QUOTE_NO_SPACE(cli_forced_style);
  }

  // NOTE: We can do this because in Qt source code
  // they specifically set object name to style name.
  m_currentStyle = qApp->style()->objectName();
  m_useSkinColors =
    skin.m_forcedSkinColors || qApp->settings()->value(GROUP(GUI), SETTING(GUI::ForcedSkinColors)).toBool();

  if (isStyleGoodForAlternativeStylePalette(m_currentStyle)) {
    if (!skin.m_stylePalette.isEmpty() && m_useSkinColors) {
      qDebugNN << LOGSEC_GUI << "Activating alternative palette.";

      QPalette pal = skin.extractPalette();

      QToolTip::setPalette(pal);
      qApp->setPalette(pal);
    }
    // NOTE: Very hacky way of avoiding automatic "dark mode"
    // palettes in some styles. Also in light mode,
    // colors are now derived from system.
#if QT_VERSION >= 0x060500 // Qt >= 6.5.0
    else /*if (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Light)*/ {
      qApp->setPalette(qt_fusionPalette(false));
    }
#endif
  }

  if (!skin.m_rawData.isEmpty()) {
    if (qApp->styleSheet().simplified().isEmpty() && m_useSkinColors) {
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
  const QString& adblocked =
    currentSkin().m_adblocked.arg(tr("This page was blocked by AdBlock"),
                                  tr(R"(Blocked URL: "%1"<br/>Used filter: "%2")").arg(url, filter));

  return currentSkin().m_layoutMarkupWrapper.arg(tr("This page was blocked by AdBlock"), adblocked);
}

PreparedHtml SkinFactory::prepareHtml(const QString& inner_html, const QUrl& base_url) {
  return {currentSkin().m_layoutMarkupWrapper.arg(QString(), inner_html), base_url};
}

PreparedHtml SkinFactory::generateHtmlOfArticles(const QList<Message>& messages, RootItem* root) const {
  Skin skin = currentSkin();
  QString messages_layout;
  QString single_message_layout = skin.m_layoutMarkup;
  const auto forced_img_size =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::MessageHeadImageHeight)).toInt();

  auto* feed = root != nullptr
                 ? root->getParentServiceRoot()
                     ->getItemFromSubTree([messages](const RootItem* it) {
                       return it->kind() == RootItem::Kind::Feed && it->customId() == messages.at(0).m_feedId;
                     })
                     ->toFeed()
                 : nullptr;
  /*
  bool is_rtl_feed = feed != nullptr && feed->isRtl();
  */

  for (const Message& message : messages) {
    QString enclosures;
    QString enclosure_images;
    bool is_plain = !TextFactory::couldBeHtml(message.m_contents);

    if (root == nullptr || root->getParentServiceRoot()->displaysEnclosures()) {
      for (const Enclosure& enclosure : message.m_enclosures) {
        QString enc_url = QUrl::fromPercentEncoding(enclosure.m_url.toUtf8());

        enclosures += skin.m_enclosureMarkup.arg(enc_url, QSL("&#129527;"), enclosure.m_mimeType);

        if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::DisplayEnclosuresInMessage)).toBool()) {
          if (enclosure.m_mimeType.startsWith(QSL("image/")) &&
              qApp->settings()->value(GROUP(Messages), SETTING(Messages::DisplayEnclosuresInMessage)).toBool()) {
            // Add thumbnail image.
            enclosure_images +=
              skin.m_enclosureImageMarkup.arg(enclosure.m_url,
                                              enclosure.m_mimeType,
                                              forced_img_size <= 0 ? QString() : QString::number(forced_img_size));
          }
        }
      }
    }

    QString msg_date =
      qApp->settings()->value(GROUP(Messages), SETTING(Messages::UseCustomDate)).toBool()
        ? message.m_created.toLocalTime()
            .toString(qApp->settings()->value(GROUP(Messages), SETTING(Messages::CustomDateFormat)).toString())
        : qApp->localization()->loadedLocale().toString(message.m_created.toLocalTime(),
                                                        QLocale::FormatType::ShortFormat);

    messages_layout.append(single_message_layout.arg(message.m_title,
                                                     tr("Written by ") + (message.m_author.isEmpty()
                                                                            ? tr("unknown author")
                                                                            : message.m_author),
                                                     message.m_url,
                                                     is_plain
                                                       ? Qt::convertFromPlainText(message.m_contents,
                                                                                  Qt::WhiteSpaceMode::WhiteSpaceNormal)
                                                       : message.m_contents,
                                                     msg_date,
                                                     enclosures,
                                                     enclosure_images,
                                                     QString::number(message.m_id),
                                                     message.m_isRtl ? QSL("rtl") : QSL("ltr")));
  }

  QString msg_contents =
    skin.m_layoutMarkupWrapper.arg(messages.size() == 1 ? messages.at(0).m_title : tr("Newspaper view"),
                                   messages_layout);
  QString base_url;

  if (feed != nullptr) {
    QUrl url(NetworkFactory::sanitizeUrl(feed->source()));

    if (url.isValid()) {
      if (url.isLocalFile()) {
        base_url = url.scheme() + QSL("://") + url.toLocalFile();
      }
      else {
        base_url = url.scheme() + QSL("://") + url.host();
      }
    }
  }

  return {msg_contents, base_url};
}

Skin SkinFactory::skinInfo(const QString& skin_name, bool* ok) const {
  Skin skin;
  const QStringList skins_root_folders = {APP_SKIN_PATH, customSkinBaseFolder()};

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
      const QString base_skin_name = skin_node.toElement().attribute(QSL("base"));
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

      // Obtain skin data.
      skin.m_visibleName = skin_name;
      skin.m_author = skin_node.namedItem(QSL("author")).namedItem(QSL("name")).toElement().text();
      skin.m_version = skin_node.attributes().namedItem(QSL("version")).toAttr().value();
      skin.m_description = skin_node.namedItem(QSL("description")).toElement().text();
      skin.m_baseName = skin_name;

      if (!skin_node.namedItem(QSL("default-font")).isNull()) {
        skin.m_defaultFont =
          QFont(skin_node.namedItem(QSL("default-font")).namedItem(QSL("family")).toElement().text(),
                skin_node.namedItem(QSL("default-font")).namedItem(QSL("size")).toElement().text().toInt());
      }

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

      // Obtain alternative style palette.
      skin.m_forcedStyles = skin_node.namedItem(QSL("forced-styles"))
                              .toElement()
                              .text()
                              .split(',',
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                     Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                     QString::SplitBehavior::SkipEmptyParts);
#endif

      skin.m_forcedSkinColors =
        skin_node.namedItem(QSL("forced-skin-colors")).toElement().text() == QVariant(true).toString();

      QDomElement style_palette_root = skin_node.namedItem(QSL("style-palette")).toElement();

      if (!style_palette_root.isNull()) {
        QMetaEnum enumerp = QMetaEnum::fromType<QPalette::ColorGroup>();
        QMetaEnum enumerx = QMetaEnum::fromType<QPalette::ColorRole>();
        QMetaEnum enumery = QMetaEnum::fromType<Qt::BrushStyle>();

        QMultiHash<QPalette::ColorGroup, QPair<QPalette::ColorRole, QPair<QColor, Qt::BrushStyle>>> groups;

        QDomNodeList groups_of_palette = style_palette_root.elementsByTagName(QSL("group"));

        for (int i = 0; i < groups_of_palette.size(); i++) {
          const QDomNode& group_root_nd = groups_of_palette.at(i);
          QPalette::ColorGroup group =
            QPalette::ColorGroup(enumerp.keyToValue(group_root_nd.toElement().attribute(QSL("id")).toLatin1()));

          QDomNodeList colors_of_group = group_root_nd.toElement().elementsByTagName(QSL("color"));

          for (int j = 0; j < colors_of_group.size(); j++) {
            const QDomNode& color_nd = colors_of_group.at(j);

            QColor color(color_nd.toElement().text());
            QPalette::ColorRole role =
              QPalette::ColorRole(enumerx.keyToValue(color_nd.toElement().attribute(QSL("role")).toLatin1()));
            Qt::BrushStyle brush =
              Qt::BrushStyle(enumery.keyToValue(color_nd.toElement().attribute(QSL("brush")).toLatin1()));

            groups.insert(group, QPair<QPalette::ColorRole, QPair<QColor, Qt::BrushStyle>>(role, {color, brush}));
          }
        }

        skin.m_stylePalette = groups;
      }

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
        qWarningNN << "Skin" << QUOTE_W_SPACE(skin_name) << "does not support separated custom CSS.";
      }

      skin.m_enclosureImageMarkup =
        loadSkinFile(skin_folder_no_sep, QSL("html_enclosure_image.html"), real_base_skin_folder);
      skin.m_layoutMarkup = loadSkinFile(skin_folder_no_sep, QSL("html_single_message.html"), real_base_skin_folder);
      skin.m_enclosureMarkup =
        loadSkinFile(skin_folder_no_sep, QSL("html_enclosure_every.html"), real_base_skin_folder);
      skin.m_rawData = loadSkinFile(skin_folder_no_sep, QSL("qt_style.qss"), real_base_skin_folder);
      skin.m_adblocked = loadSkinFile(skin_folder_no_sep, QSL("html_adblocked.html"), real_base_skin_folder);
      skin.m_skinFolder = skin_folder_no_sep;

      if (ok != nullptr) {
        *ok = !skin.m_author.isEmpty() && !skin.m_version.isEmpty() && !skin.m_baseName.isEmpty() &&
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

QString SkinFactory::loadSkinFile(const QString& skin_folder,
                                  const QString& file_name,
                                  const QString& base_folder) const {
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

QString SkinFactory::currentStyle() const {
  return m_currentStyle;
}

bool SkinFactory::styleIsFrozen() const {
  return m_styleIsFrozen;
}

QList<Skin> SkinFactory::installedSkins() const {
  QList<Skin> skins;
  bool skin_load_ok;
  QStringList skin_directories =
    QDir(APP_SKIN_PATH).entryList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot | QDir::Filter::Readable);

  skin_directories.append(QDir(customSkinBaseFolder())
                            .entryList(QDir::Filter::Dirs | QDir::Filter::NoDotAndDotDot | QDir::Filter::Readable));

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

QVariant Skin::colorForModel(SkinEnums::PaletteColors type, bool use_skin_colors, bool ignore_custom_colors) const {
  if (!ignore_custom_colors) {
    bool enabled = qApp->settings()->value(GROUP(CustomSkinColors), SETTING(CustomSkinColors::Enabled)).toBool();

    if (enabled) {
      const QMetaObject& mo = SkinEnums::staticMetaObject;
      QMetaEnum enumer = mo.enumerator(mo.indexOfEnumerator(QSL("PaletteColors").toLocal8Bit().constData()));
      QColor custom_clr = qApp->settings()->value(GROUP(CustomSkinColors), enumer.valueToKey(int(type))).toString();

      if (custom_clr.isValid()) {
        return custom_clr;
      }
    }
  }

  return (use_skin_colors && m_colorPalette.contains(type)) ? m_colorPalette[type] : QVariant();
}

QPalette Skin::extractPalette() const {
  QPalette pal;
  QList<QPalette::ColorGroup> groups = m_stylePalette.keys();

  if (groups.contains(QPalette::ColorGroup::All)) {

    groups.removeAll(QPalette::ColorGroup::All);
    groups.insert(0, QPalette::ColorGroup::All);
  }

  for (QPalette::ColorGroup grp : groups) {
    auto roles = m_stylePalette.values(grp);

    for (const auto& rl : roles) {
      if (rl.second.second <= 0) {
        pal.setColor(grp, rl.first, rl.second.first);
      }
      else {
        pal.setBrush(grp, rl.first, QBrush(rl.second.first, rl.second.second));
      }
    }
  }

  return pal;
}

QString SkinEnums::palleteColorText(PaletteColors col) {
  switch (col) {
    case SkinEnums::PaletteColors::FgInteresting:
      return QObject::tr("interesting stuff");

    case SkinEnums::PaletteColors::FgSelectedInteresting:
      return QObject::tr("interesting stuff (highlighted)");

    case SkinEnums::PaletteColors::FgError:
      return QObject::tr("errored items");

    case SkinEnums::PaletteColors::FgSelectedError:
      return QObject::tr("errored items (highlighted)");

    case SkinEnums::PaletteColors::FgNewMessages:
      return QObject::tr("items with new articles");

    case SkinEnums::PaletteColors::FgSelectedNewMessages:
      return QObject::tr("items with new articles (highlighted)");

    case SkinEnums::PaletteColors::Allright:
      return QObject::tr("OK-ish color");

    default:
      return {};
  }
}
