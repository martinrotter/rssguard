// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SKINFACTORY_H
#define SKINFACTORY_H

#include "core/message.h"
#include "gui/webviewers/webviewer.h"

#include <QColor>
#include <QFont>
#include <QHash>
#include <QMetaType>
#include <QObject>
#include <QPalette>
#include <QStringList>
#include <QVariant>

class RootItem;

class SkinEnums : public QObject {
    Q_OBJECT

  public:
    enum class PaletteColors {
      // Paint foreground of some interesting items (feeds with new articles, etc.).
      FgInteresting = 1,

      // Paint foreground of some interesting items WHEN SELECTED (feeds with new articles, etc.).
      FgSelectedInteresting = 2,

      // Paint foreground of some errored items (feeds with error, etc.).
      FgError = 4,

      // Paint foreground of some errored items WHEN SELECTED (feeds with error, etc.).
      FgSelectedError = 8,

      // OK-ish color (background of list with test results of article filter).
      Allright = 16,

      // Foreground color of items with new articles.
      FgNewMessages = 32,

      // Foreground color of selected items with new articles.
      FgSelectedNewMessages = 64,

      // Foreground color of disabled items.
      FgDisabledFeed = 128
    };

    static QString palleteColorText(PaletteColors col);

    Q_ENUM(PaletteColors)
};

struct RSSGUARD_DLLSPEC Skin {
    QString m_skinFolder;
    QString m_baseName;
    QString m_visibleName;
    QString m_author;
    QString m_version;
    QString m_description;
    QString m_rawData;
    QString m_rawForcedData;
    QString m_layoutMarkupWrapper;
    QString m_enclosureImageMarkup;
    QString m_layoutMarkup;
    QString m_enclosureMarkup;
    QHash<SkinEnums::PaletteColors, QColor> m_colorPalette;
    QStringList m_forcedStyles;
    bool m_forcedSkinColors = false;
    bool m_hasStylePalette = false;
    QPalette m_stylePalette;

    bool hasPalette() const;
    QVariant colorForModel(SkinEnums::PaletteColors type,
                           bool use_skin_colors,
                           bool ignore_custom_colors = false) const;
};

uint qHash(const SkinEnums::PaletteColors& key);

Q_DECLARE_METATYPE(Skin)

class RSSGUARD_DLLSPEC SkinFactory : public QObject {
    Q_OBJECT

  public:
    explicit SkinFactory(QObject* parent = nullptr);
    virtual ~SkinFactory() = default;

    // Loads skin name from settings and sets it as active.
    void loadCurrentSkin(bool replace_existing_qss);
    Skin currentSkin() const;

    // Gets color for model from active skin.
    QVariant colorForModel(SkinEnums::PaletteColors type, bool ignore_custom_colors = false) const;

    bool isStyleGoodForAlternativeStylePalette(const QString& style_name) const;

    // Returns the name of the skin, that should be activated
    // after application restart.
    QString selectedSkinName() const;

    QString prepareHtml(const QString& inner_html);
    QString generateHtmlOfArticle(const Message& message, RootItem* root) const;

    // Gets skin about a particular skin.
    Skin skinInfo(const QString& skin_name, bool* ok = nullptr) const;

    // Returns list of installed skins.
    QList<Skin> installedSkins() const;

    // Sets the desired skin as the active one if it exists.
    void setCurrentSkinName(const QString& skin_name);

    QString customSkinBaseFolder() const;
    bool styleIsFrozen() const;
    QString currentStyle() const;

  private:
    void loadSkinFromData(const Skin& skin, bool replace_existing_qss);

    QString loadSkinFile(const QString& skin_folder, const QString& file_name, const QString& base_folder) const;
    QString replacePaletteInCss(const QString& css, const QPalette& palette) const;

    // Holds name of the current skin.
    Skin m_currentSkin;
    QString m_currentStyle;
    bool m_styleIsFrozen;
    bool m_useSkinColors;
};

inline Skin SkinFactory::currentSkin() const {
  return m_currentSkin;
}

#endif // SKINFACTORY_H
