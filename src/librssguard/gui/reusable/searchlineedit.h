// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SEARCHLINEEDIT_H
#define SEARCHLINEEDIT_H

#include "gui/reusable/baselineedit.h"

class QActionGroup;

class SearchLineEdit : public BaseLineEdit {
    Q_OBJECT

  public:
    struct CustomSearchChoice {
      public:
        CustomSearchChoice(const QString& title, int data) : m_title(title), m_data(data) {}

        QString m_title;
        int m_data;
    };

    enum class SearchMode { FixedString = 1, Wildcard = 2, RegularExpression = 4 };

    explicit SearchLineEdit(const QList<CustomSearchChoice>& choices, QWidget* parent = nullptr);

  private slots:
    void startSearch();

  signals:
    void searchCriteriaChanged(SearchMode mode,
                               Qt::CaseSensitivity sensitivity,
                               int custom_criteria,
                               const QString& phrase);

  private:
    QString titleForMode(SearchMode mode);

  private:
    QTimer* m_tmrSearchPattern;
    QMenu* m_menu;
    QAction* m_actCaseSensitivity;
    QActionGroup* m_actionGroupModes;
    QActionGroup* m_actionGroupChoices;
};

#endif // SEARCHLINEEDIT_H
