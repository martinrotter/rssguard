// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/skinfactory.h"

#include "definitions/globals.h"
#include "miscellaneous/application.h"
#include "miscellaneous/domdocument.h"
#include "miscellaneous/settings.h"
#include "services/abstract/rootitem.h"

#include <QDir>
#include <QDomElement>
#include <QFontDatabase>
#include <QProcessEnvironment>
#include <QStyle>
#include <QStyleFactory>
#include <QStyleHints>
#include <QTextDocument>
#include <QToolTip>

SkinFactory::SkinFactory(QObject* parent) : QObject(parent), m_styleIsFrozen(false), m_useSkinColors(false) {}

void SkinFactory::loadCurrentSkin(bool replace_existing_qss) {
  QList<QString> skin_names_to_try = {selectedSkinName(), QSL(APP_SKIN_DEFAULT)};
  bool skin_parsed;
  Skin skin_data;
  QString skin_name;

  while (!skin_names_to_try.isEmpty()) {
    skin_name = skin_names_to_try.takeFirst();
    skin_data = skinInfo(skin_name, &skin_parsed);

    if (skin_parsed) {
      loadSkinFromData(skin_data, replace_existing_qss);

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
  static QRegularExpression re = QRegularExpression(QSL("^(fusion|windows|windowsvista|windows11|qt[56]ct-style)$"));

  return re.match(style_name.toLower()).hasMatch();
}

// NOTE: Taken from "QPlatformThemePrivate" Qt class.
// This is here because in Qt 6.5.0, they hardcoded
// DARK palette if user has enabled "dark mode" in OS.
QPalette qt_fusionPalette(bool dark_appearance) {
  const QColor windowText = dark_appearance ? QColor(240, 240, 240) : Qt::black;
  const QColor backGround = dark_appearance ? QColor(50, 50, 50) : QColor(239, 239, 239);
  const QColor light = backGround.lighter(150);
  const QColor mid = (backGround.darker(130));
  const QColor midLight = mid.lighter(110);
  const QColor base = dark_appearance ? backGround.darker(140) : Qt::white;
  const QColor disabledBase(backGround);
  const QColor dark = backGround.darker(150);
  const QColor darkDisabled = QColor(209, 209, 209).darker(110);
  const QColor text = dark_appearance ? windowText : Qt::black;
  const QColor highlight = QColor(48, 140, 198);
  const QColor hightlightedText = dark_appearance ? windowText : Qt::white;
  const QColor disabledText = dark_appearance ? QColor(130, 130, 130) : QColor(190, 190, 190);
  const QColor button = backGround;
  const QColor shadow = dark.darker(135);
  const QColor disabledShadow = shadow.lighter(150);
  const QColor disabledHighlight(145, 145, 145);

  QColor placeholder = text;
  placeholder.setAlpha(128);

  QPalette fusionPalette(windowText, backGround, light, dark, mid, text, base);
  fusionPalette.setBrush(QPalette::ColorRole::Midlight, midLight);
  fusionPalette.setBrush(QPalette::ColorRole::Button, button);
  fusionPalette.setBrush(QPalette::ColorRole::Shadow, shadow);
  fusionPalette.setBrush(QPalette::ColorRole::HighlightedText, hightlightedText);

  fusionPalette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Text, disabledText);
  fusionPalette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::WindowText, disabledText);
  fusionPalette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::ButtonText, disabledText);
  fusionPalette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Base, disabledBase);
  fusionPalette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Dark, darkDisabled);
  fusionPalette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Shadow, disabledShadow);

  fusionPalette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Highlight, highlight);
  fusionPalette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Highlight, highlight);
  fusionPalette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Highlight, disabledHighlight);

#if QT_VERSION >= 0x060600 // Qt >= 6.6.0
  fusionPalette.setBrush(QPalette::ColorGroup::Active, QPalette::ColorRole::Accent, highlight);
  fusionPalette.setBrush(QPalette::ColorGroup::Inactive, QPalette::ColorRole::Accent, highlight);
  fusionPalette.setBrush(QPalette::ColorGroup::Disabled, QPalette::ColorRole::Accent, disabledHighlight);
#endif

  fusionPalette.setBrush(QPalette::PlaceholderText, placeholder);

  // Use a more legible light blue on dark backgrounds than the default Qt::blue.
  if (dark_appearance) {
    fusionPalette.setBrush(QPalette::ColorRole::Link, highlight);
  }

  return fusionPalette;
}

void SkinFactory::loadSkinFromData(const Skin& skin, bool replace_existing_qss) {
#if QT_VERSION >= 0x060500 // Qt >= 6.5.0
  auto system_color_scheme = qApp->styleHints()->colorScheme();

  qDebugNN << LOGSEC_GUI << "OS defines color scheme:" << QUOTE_W_SPACE_DOT(system_color_scheme);
#endif

  QString style_name = qApp->settings()->value(GROUP(GUI), SETTING(GUI::Style)).toString();
  auto env = QProcessEnvironment::systemEnvironment();
  const QString env_forced_style = env.value(QSL("QT_STYLE_OVERRIDE"));
  const QString cli_forced_style = qApp->cmdParser()->value(QSL(CLI_STYLE_SHORT));

  if (env_forced_style.isEmpty() && cli_forced_style.isEmpty()) {
    m_styleIsFrozen = false;

    if (!skin.m_forcedStyles.isEmpty()) {
      qDebugNN << LOGSEC_GUI << "Forcing one of skin's declared styles:" << QUOTE_W_SPACE_DOT(skin.m_forcedStyles);

      for (const QString& skin_forced_style : skin.m_forcedStyles) {
        if (qApp->setStyle(skin_forced_style) != nullptr) {
          m_currentStyle = skin_forced_style;
          break;
        }
      }
    }
    else if (!style_name.isEmpty()) {
      qApp->setStyle(style_name);
      m_currentStyle = style_name;

      qDebugNN << LOGSEC_GUI << "Setting style:" << QUOTE_W_SPACE_DOT(m_currentStyle);
    }
    else {
      // Default style is set. Just use what is already set.
      m_currentStyle = QString();

      qDebugNN << LOGSEC_GUI << "Using default style:" << QUOTE_W_SPACE_DOT(qApp->style()->objectName());
    }
  }
  else {
    m_styleIsFrozen = true;
    m_currentStyle = qApp->style()->objectName();

    qWarningNN << LOGSEC_GUI << "Respecting forced style(s):\n"
               << "  QT_STYLE_OVERRIDE:" << QUOTE_W_SPACE(env_forced_style) << "\n"
               << "  CLI (-style):" << QUOTE_W_SPACE(cli_forced_style) << "\n"
               << "  current style:" << QUOTE_W_SPACE_DOT(m_currentStyle);
  }

  m_useSkinColors =
    skin.m_forcedSkinColors || qApp->settings()->value(GROUP(GUI), SETTING(GUI::ForcedSkinColors)).toBool();

  if (m_useSkinColors && isStyleGoodForAlternativeStylePalette(m_currentStyle)) {
    if (skin.hasPalette()) {
      qDebugNN << LOGSEC_GUI << "Activating alternative palette.";

      QToolTip::setPalette(skin.m_stylePalette);
      QApplication::setPalette(skin.m_stylePalette);
      QGuiApplication::setPalette(skin.m_stylePalette);
    }
    // NOTE: Very hacky way of avoiding automatic "dark mode"
    // palettes in some styles. Also in light mode,
    // colors are now derived from system.
    //
    // https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5
#if QT_VERSION >= 0x060500 // Qt >= 6.5.0
    else {
      qApp
        ->setPalette(qt_fusionPalette(false /*QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark*/));
    }
#endif
  }

  QString qss_to_set = skin.m_rawForcedData;

  if (m_useSkinColors && !skin.m_rawData.isEmpty()) {
    if (qApp->styleSheet().simplified().isEmpty()) {
      qss_to_set += QSL("\r\n") + skin.m_rawData;
    }
    else {
      qCriticalNN << LOGSEC_GUI
                  << "Skipped setting of application style and skin because there is already some style set.";
    }
  }

  if (!replace_existing_qss) {
    qss_to_set = qApp->styleSheet() + QSL("\r\n") + qss_to_set;
  }

  qApp->setStyleSheet(qss_to_set);
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

QString SkinFactory::prepareHtml(const QString& inner_html) {
  return currentSkin()
    .m_layoutMarkupWrapper.replace(QSL("%article_title%"), QString())
    .replace(QSL("%article_body%"), inner_html);
}

QString SkinFactory::generateHtmlOfArticle(const Message& message, RootItem* root) const {
  const Skin skin = currentSkin();
  const bool display_enclosures =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::DisplayEnclosuresInMessage)).toBool();
  const int forced_img_height =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::LimitArticleImagesHeight)).toInt();
  const bool is_plain = !TextFactory::couldBeHtml(message.m_contents);

  QString messages_layout;
  QString enclosures;
  QString enclosure_images;

  if (root == nullptr || root->account()->displaysEnclosures()) {
    for (const QSharedPointer<MessageEnclosure>& enclosure : message.m_enclosures) {
      QString enc_url = QUrl::fromPercentEncoding(enclosure->url().toUtf8());

      enclosures += QString(skin.m_enclosureMarkup)
                      .replace(QSL("%enclosure_url%"), enc_url)
                      .replace(QSL("%enclosure_mime%"), enclosure->mimeType());

      if (display_enclosures && enclosure->mimeType().startsWith(QSL("image/"))) {
        // Add thumbnail image.
        enclosure_images +=
          QString(skin.m_enclosureImageMarkup)
            .replace(QSL("%enclosure_url%"), enc_url)
            .replace(QSL("%enclosure_mime%"), enclosure->mimeType())
            .replace(QSL("%image_size%"), forced_img_height <= 0 ? QSL("none") : QSL("%1px").arg(forced_img_height));
      }
    }
  }

  QString msg_date =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::UseCustomDate)).toBool()
      ? message.m_created.toLocalTime()
          .toString(qApp->settings()->value(GROUP(Messages), SETTING(Messages::CustomDateFormat)).toString())
      : qApp->localization()->loadedLocale().toString(message.m_created.toLocalTime(),
                                                      QLocale::FormatType::ShortFormat);

  QString msg_contents =
    is_plain ? Qt::convertFromPlainText(message.m_contents, Qt::WhiteSpaceMode::WhiteSpaceNormal) : message.m_contents;

  messages_layout.append(QString(skin.m_layoutMarkup)
                           .replace(QSL("%article_title%"), message.m_title)
                           .replace(QSL("%article_author%"),
                                    message.m_author.isEmpty() ? tr("unknown author") : message.m_author)
                           .replace(QSL("%article_feed%"), message.m_feedTitle)
                           .replace(QSL("%article_author_full%"),
                                    tr("Written by ") +
                                      (message.m_author.isEmpty() ? tr("unknown author") : message.m_author))
                           .replace(QSL("%article_url%"), message.m_url)
                           .replace(QSL("%article_contents%"), msg_contents)
                           .replace(QSL("%article_date%"), msg_date)
                           .replace(QSL("%enclosures_all%"), enclosures)
                           .replace(QSL("%enclosures_images%"), enclosure_images)
                           .replace(QSL("%article_id%"), QString::number(message.m_id))
                           .replace(QSL("%article_rtl%"),
                                    (message.m_rtlBehavior == RtlBehavior::Everywhere ||
                                     message.m_rtlBehavior == RtlBehavior::EverywhereExceptFeedList ||
                                     message.m_rtlBehavior == RtlBehavior::OnlyViewer)
                                      ? QSL("rtl")
                                      : QSL("ltr")));

  QString html = QString(skin.m_layoutMarkupWrapper)
                   .replace(QSL("%article_title%"), message.m_title)
                   .replace(QSL("%article_body%"), messages_layout);

  return html;
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
      DomDocument document;

      if (!skin_file.open(QIODevice::OpenModeFlag::Text | QIODevice::OpenModeFlag::ReadOnly) ||
          !document.setContent(skin_file.readAll(), true)) {
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

      // Obtain color palette.
      QHash<SkinEnums::PaletteColors, QColor> palette;
      QDomNodeList colors_of_palette = skin_node.namedItem(QSL("palette")).toElement().elementsByTagName(QSL("color"));

      for (int i = 0; i < colors_of_palette.size(); i++) {
        QDomElement elem_clr = colors_of_palette.item(i).toElement();
        QString en_val = elem_clr.attribute(QSL("key"));
        auto key = stringToEnum<SkinEnums::PaletteColors>(en_val);
        QColor value = elem_clr.text();

        if (value.isValid()) {
          palette.insert(key, value);
        }
      }

      skin.m_colorPalette = palette;

      // Obtain alternative style palette.
      skin.m_forcedStyles =
        skin_node.namedItem(QSL("forced-styles")).toElement().text().split(QChar(','), SPLIT_BEHAVIOR::SkipEmptyParts);

      skin.m_forcedSkinColors =
        skin_node.namedItem(QSL("forced-skin-colors")).toElement().text() == QVariant(true).toString();

      QDomElement style_palette_root = skin_node.namedItem(QSL("style-palette")).toElement();

      if (!style_palette_root.isNull()) {
        // NOTE: We avoid stringToEnum() for performance reasons here.
        QMetaEnum enumerp = QMetaEnum::fromType<QPalette::ColorGroup>();
        QMetaEnum enumerx = QMetaEnum::fromType<QPalette::ColorRole>();
        QMetaEnum enumery = QMetaEnum::fromType<Qt::BrushStyle>();
        QPalette pal;

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

            int brush_val = enumery.keyToValue(color_nd.toElement().attribute(QSL("brush")).toLatin1());

            if (brush_val >= 0) {
              Qt::BrushStyle brush = Qt::BrushStyle(brush_val);
              pal.setBrush(group, role, QBrush(color, brush));
            }
            else {
              pal.setColor(group, role, color);
            }
          }
        }

        skin.m_hasStylePalette = true;
        skin.m_stylePalette = pal;
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
        auto target_palette =
          ((skin.m_forcedSkinColors || qApp->settings()->value(GROUP(GUI), SETTING(GUI::ForcedSkinColors)).toBool()) &&
           skin.hasPalette())
            ? skin.m_stylePalette
            : qApp->palette();

        custom_css = replacePaletteInCss(custom_css, target_palette);

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

      try {
        skin.m_rawForcedData = loadSkinFile(skin_folder_no_sep, QSL("qt_style_forced.qss"), real_base_skin_folder);
      }
      catch (...) {
        qWarningNN << "Skin" << QUOTE_W_SPACE(skin_name) << "does not support forced QSS.";
      }

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
  QStringList prefixes = {QString()};

  for (const QString& prefix : prefixes) {
    QString local_file = QDir::toNativeSeparators(skin_folder + QDir::separator() + prefix + file_name);
    QString base_file = QDir::toNativeSeparators(base_folder + QDir::separator() + prefix + file_name);
    QString data;

    if (QFile::exists(local_file)) {
      qDebugNN << LOGSEC_GUI << "Local file" << QUOTE_W_SPACE(local_file) << "exists, using it for the skin.";
      data = QString::fromUtf8(IOFactory::readFile(local_file));
      return data.replace(QSL(USER_DATA_PLACEHOLDER), skin_folder);
    }
    else if (QFile::exists(base_file)) {
      qDebugNN << LOGSEC_GUI << "Base file" << QUOTE_W_SPACE(base_file) << "exists, using it for the skin.";
      data = QString::fromUtf8(IOFactory::readFile(base_file));
      return data.replace(QSL(USER_DATA_PLACEHOLDER), base_folder);
    }
  }

  throw ApplicationException(tr("file %1 not found").arg(file_name));
}

QString SkinFactory::replacePaletteInCss(const QString& css, const QPalette& palette) const {
  static QRegularExpression re(QSL("palette\\((\\w+)\\)"));
  const std::function<QString(const QRegularExpressionMatch&)>& replacer =
    [&palette](const QRegularExpressionMatch& match) {
      auto role = stringToEnum<QPalette::ColorRole>(match.captured(1));
      auto color = palette.color(QPalette::ColorGroup::All, role);

      return color.isValid() ? color.name() : QSL("#000000");
    };

  QString result;
  int last_pos = 0;
  QRegularExpressionMatchIterator it = re.globalMatch(css);

  while (it.hasNext()) {
    QRegularExpressionMatch match = it.next();

    // Append text before the match.
    result += css.mid(last_pos, match.capturedStart() - last_pos);

    // Append lambda-generated replacement.
    result += replacer(match);
    last_pos = match.capturedEnd();
  }

  // Append remaining text after the last match.
  result += css.mid(last_pos);
  return result;
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

bool Skin::hasPalette() const {
  return m_hasStylePalette;
}

QVariant Skin::colorForModel(SkinEnums::PaletteColors type, bool use_skin_colors, bool ignore_custom_colors) const {
  if (!ignore_custom_colors) {
    bool enabled = qApp->settings()->value(GROUP(CustomSkinColors), SETTING(CustomSkinColors::Enabled)).toBool();

    if (enabled) {
      QColor custom_clr = qApp->settings()->value(GROUP(CustomSkinColors), enumToString(type)).toString();

      if (custom_clr.isValid()) {
        return custom_clr;
      }
    }
  }

  return (use_skin_colors && m_colorPalette.contains(type)) ? m_colorPalette[type] : QVariant();
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

    case SkinEnums::PaletteColors::FgDisabledFeed:
      return QObject::tr("disabled items");

    default:
      return {};
  }
}
