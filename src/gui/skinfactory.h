#ifndef SKINFACTORY_H
#define SKINFACTORY_H

#include <QObject>
#include <QPointer>
#include <QStringList>
#include <QMetaType>


struct Skin {
    QString m_baseName;
    QString m_visibleName;
    QStringList m_stylesNames;
    QString m_author;
    QString m_email;
    QString m_version;
    QString m_rawData;
    QString m_layoutMarkupWrapper;
    QString m_layoutMarkup;
};

Q_DECLARE_METATYPE(Skin)

// NOTE: Skin "base/vergilius.xml" is now NEEDED for rssguard
// to run. IT IS DEFAULT skin. It sets no styles and stylesheet.
// It just contains markup for webbrowser.

class SkinFactory : public QObject {
    Q_OBJECT

  private:
    // Constructor.
    explicit SkinFactory(QObject *parent = 0);

    // Loads the skin from give skin_data.
    // NOTE: Extra relative path escaping is done for loading of
    // external resources.
    bool loadSkinFromData(const Skin &skin);

  public:
    // Singleton getter.
    static SkinFactory *instance();

    // Destructor.
    virtual ~SkinFactory();

    // Loads skin name from settings and sets it as active.
    void loadCurrentSkin();

    // Returns contents of current layout markup.
    inline QString currentMarkup() {
      return m_currentSkin.m_layoutMarkup;
    }

    inline QString currentMarkupLayout() {
      return m_currentSkin.m_layoutMarkupWrapper;
    }

    // Returns the name of the skin, that should be activated
    // after application restart.
    QString selectedSkinName();

    // Gets skin about a particular skin.
    Skin skinInfo(const QString &skin_name, bool *ok = NULL);

    // Returns list of installed skins, including "default system skin".
    // NOTE: Default skin always lies at position 0.
    QList<Skin> installedSkins();

    // Sets the desired skin as the active one if it exists.
    void setCurrentSkinName(const QString &skin_name);

  private:
    // Holds name of the current skin.
    Skin m_currentSkin;

    // Singleton.
    static QPointer<SkinFactory> s_instance;
};

#endif // SKINFACTORY_H
