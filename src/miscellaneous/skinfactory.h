// For license of this file, see <object-root-folder>/LICENSE.md.

#ifndef SKINFACTORY_H
#define SKINFACTORY_H

#include <QObject>

#include <QMetaType>
#include <QStringList>

struct Skin {
  QString m_baseName;
  QString m_visibleName;
  QString m_author;
  QString m_email;
  QString m_version;
  QString m_rawData;
  QString m_adblocked;
  QString m_layoutMarkupWrapper;
  QString m_enclosureImageMarkup;
  QString m_layoutMarkup;
  QString m_enclosureMarkup;
};

Q_DECLARE_METATYPE(Skin)

class SkinFactory : public QObject {
  Q_OBJECT

  public:

    // Constructor.
    explicit SkinFactory(QObject* parent = 0);

    // Destructor.
    virtual ~SkinFactory();

    // Loads skin name from settings and sets it as active.
    void loadCurrentSkin();

    inline Skin currentSkin() const {
      return m_currentSkin;
    }

    // Returns the name of the skin, that should be activated
    // after application restart.
    QString selectedSkinName() const;

    QString adBlockedPage(const QString& subscription, const QString& rule);

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

#endif // SKINFACTORY_H
