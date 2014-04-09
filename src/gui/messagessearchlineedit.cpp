#include "gui/messagessearchlineedit.h"


MessagesSearchLineEdit::MessagesSearchLineEdit(QWidget *parent) : BaseLineEdit(parent) {
  // TODO: ke standardnimu contextovemu menu (metoda createStandardContextMenu()
  // pridat submenu "Search type" = fixed string, wildcard, regexp
  // a vic neresit asi na strane tohodle kontrolu
}

MessagesSearchLineEdit::~MessagesSearchLineEdit() {
}
