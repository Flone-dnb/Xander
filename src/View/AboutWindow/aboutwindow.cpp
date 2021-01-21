// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "aboutwindow.h"
#include "ui_aboutwindow.h"

// Qt
#include <QStyle>
#include <QDesktopServices>
#include <QUrl>

// Custom
#include "Model/globals.h"

AboutWindow::AboutWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AboutWindow)
{
    ui->setupUi(this);

    setFixedSize (width(), height());

    // Apply button style.
    ui->pushButton_github->setProperty("cssClass", "github");
    ui->pushButton_github->style()->unpolish(ui->pushButton_github);
    ui->pushButton_github->style()->polish(ui->pushButton_github);
    ui->pushButton_github->update();


    ui ->label_appIcon    ->setPixmap ( QPixmap(":/logo.png").scaled (128, 128, Qt::KeepAspectRatio) );
    ui ->label_version    ->setText   ( "Xander (version: " + QString::fromStdString(XANDER_VERSION) + ")" );
    ui ->label_copyright  ->setText   ( "Copyright (c) 2021.\nAleksandr \"Flone\" Tretyakov." );
}

void AboutWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

    deleteLater();
}

void AboutWindow::on_pushButton_github_clicked()
{
    QDesktopServices::openUrl(QUrl("https://github.com/Flone-dnb/Xander"));
}

AboutWindow::~AboutWindow()
{
    delete ui;
}
