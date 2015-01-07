/**
 * Copyright (c) 2008, Benjamin C. Meyer  <ben@meyerhome.net>
 * Copyright (c) 2009, Jakub Wieczorek <faw217@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Benjamin Meyer nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "edittableview.h"

#include <qevent.h>

EditTableView::EditTableView(QWidget *parent)
    : QTableView(parent)
{
}

void EditTableView::keyPressEvent(QKeyEvent *event)
{
    if (model() && event->key() == Qt::Key_Delete) {
        removeSelected();
        event->setAccepted(true);
    } else {
        QAbstractItemView::keyPressEvent(event);
    }
}

void EditTableView::removeSelected()
{
    if (!model() || !selectionModel() || !selectionModel()->hasSelection())
        return;

    QModelIndexList selectedRows = selectionModel()->selectedRows();
    if (selectedRows.isEmpty())
        return;

    int newSelectedRow = selectedRows.at(0).row();
    for (int i = selectedRows.count() - 1; i >= 0; --i) {
        QModelIndex idx = selectedRows.at(i);
        model()->removeRow(idx.row(), rootIndex());
    }

    // select the item at the same position
    QModelIndex newSelectedIndex = model()->index(newSelectedRow, 0, rootIndex());
    // if that was the last item
    if (!newSelectedIndex.isValid())
        newSelectedIndex = model()->index(newSelectedRow - 1, 0, rootIndex());

    selectionModel()->select(newSelectedIndex, QItemSelectionModel::SelectCurrent | QItemSelectionModel::Rows);
    setCurrentIndex(newSelectedIndex);
}

void EditTableView::removeAll()
{
    if (!model())
        return;

    model()->removeRows(0, model()->rowCount(rootIndex()), rootIndex());
}

