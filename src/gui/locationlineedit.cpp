// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/locationlineedit.h"

#include "network-web/googlesuggest.h"

#include <QMouseEvent>

LocationLineEdit::LocationLineEdit(QWidget* parent)
  : BaseLineEdit(parent), m_mouseSelectsAllText(true), m_googleSuggest(new GoogleSuggest(this)) {
  setPlaceholderText(tr("Website address goes here"));
  connect(this, &LocationLineEdit::submitted, m_googleSuggest, &GoogleSuggest::preventSuggest);
}

LocationLineEdit::~LocationLineEdit() {}

void LocationLineEdit::focusOutEvent(QFocusEvent* event) {
  BaseLineEdit::focusOutEvent(event);

  // User now left text box, when he enters it again and clicks,
  // then all text should be selected.
  m_mouseSelectsAllText = true;
}

void LocationLineEdit::mousePressEvent(QMouseEvent* event) {
  if (m_mouseSelectsAllText) {
    event->ignore();
    selectAll();

    // User clicked and all text was selected.
    m_mouseSelectsAllText = false;
  }
  else {
    BaseLineEdit::mousePressEvent(event);
  }
}
