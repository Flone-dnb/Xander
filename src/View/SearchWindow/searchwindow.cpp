// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "searchwindow.h"
#include "ui_searchwindow.h"

// Qt
#include <QKeyEvent>

SearchWindow::SearchWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SearchWindow)
{
    ui->setupUi(this);

    setFixedSize( width(), height() );
}

SearchWindow::~SearchWindow()
{
    delete ui;
}

void SearchWindow::searchMatchCount(size_t iCount)
{
    if (iCount == 0)
    {
        ui->label_matches->setText("No matches.");
    }
    else
    {
        ui->label_matches->setText("Found matches: " + QString::number(iCount) + ".");
    }
}

void SearchWindow::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Return)
    {
        findNext();
    }
    else if (ev->key() == Qt::Key_Escape)
    {
        close();
    }
}

void SearchWindow::closeEvent(QCloseEvent *ev)
{
    Q_UNUSED(ev)

    deleteLater();
}

void SearchWindow::on_pushButton_find_prev_clicked()
{
    emit findPrev();
}

void SearchWindow::on_pushButton_find_next_clicked()
{
    emit findNext();
}

void SearchWindow::on_lineEdit_key_textChanged(const QString &arg1)
{
    emit searchTextChanged(arg1);
}
