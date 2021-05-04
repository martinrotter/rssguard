// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ADBLOCKMANAGER_H
#define ADBLOCKMANAGER_H

#include <QObject>

class QUrl;
class AdblockRequestInfo;
class AdBlockUrlInterceptor;
class AdBlockIcon;

class AdBlockManager : public QObject {
  Q_OBJECT

  public:
    explicit AdBlockManager(QObject* parent = nullptr);

    // If "initial_load" is false, then we want to explicitly turn off
    // Adblock if it is running or turn on when not running.
    // if "initial_load" is true, then we want to forcefully perform
    // initial loading of Adblock.
    void load(bool initial_load);

    bool isEnabled() const;
    bool canRunOnScheme(const QString& scheme) const;
    AdBlockIcon* adBlockIcon() const;

    // General methods for adblocking.
    bool block(const AdblockRequestInfo& request) const;
    QString elementHidingRulesForDomain(const QUrl& url) const;

    QStringList filterLists() const;
    void setFilterLists(const QStringList& filter_lists);

    QStringList customFilters() const;
    void setCustomFilters(const QStringList& custom_filters);

    void updateUnifiedFiltersFile();

    static QString generateJsForElementHiding(const QString& css);

  public slots:
    void showDialog();

  signals:
    void enabledChanged(bool enabled);

  private:
    void restartServer();

  private:
    bool m_loaded;
    bool m_enabled;
    AdBlockIcon* m_adblockIcon;
    AdBlockUrlInterceptor* m_interceptor;
    QString m_unifiedFiltersFile;
};

inline AdBlockIcon* AdBlockManager::adBlockIcon() const {
  return m_adblockIcon;
}

#endif // ADBLOCKMANAGER_H
