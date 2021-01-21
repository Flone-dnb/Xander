// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "trackwidget.h"
#include "ui_trackwidget.h"

// Qt
#include <QStyle>

// Custom
#include "View/MainWindow/mainwindow.h"

TrackWidget::TrackWidget(QString sTrackTitle, MainWindow* pMainWindow, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TrackWidget)
{
    ui->setupUi(this);

    this->sTrackTitle = sTrackTitle;

    this->pMainWindow = pMainWindow;

    ui->label_track_title->setText(sTrackTitle);


    ui->frame->setProperty("cssClass", "trackWidget");
    ui->frame->style()->unpolish(ui->frame);
    ui->frame->style()->polish(ui->frame);
    ui->frame->update();
}

void TrackWidget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

    deleteLater();
}

void TrackWidget::mouseDoubleClickEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev)

    pMainWindow->on_trackWidget_mouseDoublePress(this);
}

void TrackWidget::mousePressEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev)

    pMainWindow->on_trackWidget_mousePress(this);
}

void TrackWidget::setSelected()
{
    ui->frame->setProperty("cssClass", "trackWidget_selected");
    ui->frame->style()->unpolish(ui->frame);
    ui->frame->style()->polish(ui->frame);
    ui->frame->update();
}

void TrackWidget::setPlaying()
{
    ui->frame->setProperty("cssClass", "trackWidget_playing");
    ui->frame->style()->unpolish(ui->frame);
    ui->frame->style()->polish(ui->frame);
    ui->frame->update();
}

void TrackWidget::setIdle()
{
    ui->frame->setProperty("cssClass", "trackWidget");
    ui->frame->style()->unpolish(ui->frame);
    ui->frame->style()->polish(ui->frame);
    ui->frame->update();
}

QString TrackWidget::getTrackTitle()
{
    return sTrackTitle;
}

TrackWidget::~TrackWidget()
{
    delete ui;
}
