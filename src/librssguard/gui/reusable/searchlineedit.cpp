// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/searchlineedit.h"

#include "3rd-party/boolinq/boolinq.h"
#include "gui/reusable/plaintoolbutton.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QActionGroup>
#include <QTimer>
#include <QWidgetAction>

SearchLineEdit::SearchLineEdit(const QList<CustomSearchChoice>& choices, QWidget* parent) : BaseLineEdit(parent) {
  QWidgetAction* act = new QWidgetAction(this);
  PlainToolButton* btn = new PlainToolButton(this);

  m_tmrSearchPattern = new QTimer(this);
  m_tmrSearchPattern->setSingleShot(true);
  m_tmrSearchPattern->setInterval(300);

  m_menu = new QMenu(btn);

  m_actionGroupChoices = new QActionGroup(this);
  m_actionGroupChoices->setExclusive(true);

  m_actionGroupModes = new QActionGroup(this);
  m_actionGroupModes->setExclusive(true);

  m_actCaseSensitivity = m_menu->addAction(tr("Case-sensitive"));
  m_actCaseSensitivity->setCheckable(true);

  m_menu->addSeparator();

  // Setup tool button.
  btn->setIcon(qApp->icons()->fromTheme(QSL("system-search")));
  btn->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
  btn->setMenu(m_menu);

  act->setDefaultWidget(btn);

  addAction(act, QLineEdit::ActionPosition::LeadingPosition);

  // Load predefined modes.
  for (SearchMode mode : {SearchMode::FixedString, SearchMode::Wildcard, SearchMode::RegularExpression}) {
    QAction* ac = m_actionGroupModes->addAction(m_menu->addAction(titleForMode(mode)));

    ac->setCheckable(true);
    ac->setData(int(mode));
  }

  m_actionGroupModes->actions().first()->setChecked(true);

  if (!choices.isEmpty()) {
    m_menu->addSeparator();

    // Load custom coices.
    for (const CustomSearchChoice& choice : choices) {
      QAction* ac = m_actionGroupChoices->addAction(m_menu->addAction(choice.m_title));

      ac->setCheckable(true);
      ac->setData(choice.m_data);
    }

    m_actionGroupChoices->actions().first()->setChecked(true);
  }

  // NOTE: When any change is made, (re)start the timer which fires
  // the signal with delay to avoid throttling.
  connect(this, &SearchLineEdit::textChanged, m_tmrSearchPattern, QOverload<>::of(&QTimer::start));
  connect(m_menu, &QMenu::triggered, m_tmrSearchPattern, QOverload<>::of(&QTimer::start));
  connect(m_tmrSearchPattern, &QTimer::timeout, this, &SearchLineEdit::startSearch);
}

void SearchLineEdit::startSearch() {
  SearchMode mode = SearchMode(boolinq::from(m_actionGroupModes->actions())
                                 .first([](const QAction* act) {
                                   return act->isChecked();
                                 })
                                 ->data()
                                 .toInt());

  int custom_criteria = boolinq::from(m_actionGroupChoices->actions())
                          .first([](const QAction* act) {
                            return act->isChecked();
                          })
                          ->data()
                          .toInt();

  bool case_sensitive = m_actCaseSensitivity->isChecked();

  emit searchCriteriaChanged(mode,
                             case_sensitive ? Qt::CaseSensitivity::CaseSensitive : Qt::CaseSensitivity::CaseInsensitive,
                             custom_criteria,
                             text());
}

QString SearchLineEdit::titleForMode(SearchMode mode) {
  switch (mode) {
    case SearchLineEdit::SearchMode::FixedString:
      return tr("Fixed text");

    case SearchLineEdit::SearchMode::Wildcard:
      return tr("Wildcard");

    case SearchLineEdit::SearchMode::RegularExpression:
      return tr("Regular expression");

    default:
      return {};
  }
}
