// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMSETTINGS_H
#define FORMSETTINGS_H

#include "ui_formsettings.h"

#include <QDialog>
#include <QPointer>

class Settings;
class SettingsPanel;
class QTabWidget;

class FormSettings : public QDialog {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FormSettings(QWidget& parent);
    virtual ~FormSettings();

  public slots:
    void reject();

  private slots:
    void openSettingsCategory(int category);
    void searchSettings(const QString& phrase);

    // Saves settings into global configuration.
    void saveSettings();
    void applySettings();
    void cancelSettings();

  private:
    struct SettingsSearchTab {
        QPointer<QTabWidget> m_tabWidget;
        int m_tabIndex = -1;
    };

    struct SettingsSearchEntry {
        QString m_text;
        QPointer<SettingsPanel> m_panel;
        QPointer<QWidget> m_widget;
        QList<SettingsSearchTab> m_tabPath;
    };

    void addSettingsPanel(SettingsPanel* panel);
    void ensureSearchIndexBuilt();
    void indexSettingsPanel(SettingsPanel* panel);
    void indexWidget(QWidget* widget, SettingsPanel* panel, QList<SettingsSearchTab> tab_path);
    void applyFirstSearchResult();
    void updateSearchHighlights();
    void clearSearch();

    static QString normalizedSearchText(const QString& text);
    static QString searchableWidgetText(QWidget* widget);

    Ui::FormSettings m_ui;
    QPushButton* m_btnApply;
    QList<SettingsPanel*> m_panels;
    QList<SettingsSearchEntry> m_searchIndex;
    QList<int> m_searchResults;
    QList<QPointer<QWidget>> m_highlightedWidgets;
    bool m_searchIndexBuilt;
    Settings& m_settings;
};

#endif // FORMSETTINGS_H
