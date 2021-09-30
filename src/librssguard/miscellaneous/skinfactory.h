// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SKINFACTORY_H
#define SKINFACTORY_H

#include <QObject>

#include <QColor>
#include <QHash>
#include <QMetaType>
#include <QStringList>

struct RSSGUARD_DLLSPEC Skin {
  enum class PaletteColors {
    Highlight = 1,
    Error = 2,
    Allright = 3
  };

  QString m_baseName;
  QString m_visibleName;
  QString m_author;
  QString m_version;
  QString m_rawData;
  QString m_adblocked;
  QString m_layoutMarkupWrapper;
  QString m_enclosureImageMarkup;
  QString m_layoutMarkup;
  QString m_enclosureMarkup;
  QHash<Skin::PaletteColors, QColor> m_colorPalette;
};

uint qHash(const Skin::PaletteColors& key);

Q_DECLARE_METATYPE(Skin)

class RSSGUARD_DLLSPEC SkinFactory : public QObject {
  Q_OBJECT

  public:
    explicit SkinFactory(QObject* parent = nullptr);
    virtual ~SkinFactory() = default;

    // Loads skin name from settings and sets it as active.
    void loadCurrentSkin();
    Skin currentSkin() const;

    // Returns the name of the skin, that should be activated
    // after application restart.
    QString selectedSkinName() const;

    QString adBlockedPage(const QString& url, const QString& filter);

    // Gets skin about a particular skin.
    Skin skinInfo(const QString& skin_name, bool* ok = nullptr) const;

    // Returns list of installed skins.
    QList<Skin> installedSkins() const;

    // Sets the desired skin as the active one if it exists.
    void setCurrentSkinName(const QString& skin_name);

    QString customSkinBaseFolder() const;

  private:

    // Loads the skin from give skin_data.
    void loadSkinFromData(const Skin& skin);

    // Holds name of the current skin.
    Skin m_currentSkin;
};

inline Skin SkinFactory::currentSkin() const {
  return m_currentSkin;
}

#endif // SKINFACTORY_H
