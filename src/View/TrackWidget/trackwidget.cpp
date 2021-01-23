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
#include <QMenu>
#include <QAction>
#include <QMouseEvent>

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


    bSelected = false;


    pMenuContextMenu = new QMenu(this);
        pActionMoveUp = new QAction  ("Move Up");
        connect(pActionMoveUp, &QAction::triggered, this, &TrackWidget::slotMoveUp);

        pActionMoveDown = new QAction("Move Down");
        connect(pActionMoveDown, &QAction::triggered, this, &TrackWidget::slotMoveDown);

        pActionDelete = new QAction  ("Delete");
        connect(pActionDelete, &QAction::triggered, this, &TrackWidget::slotDelete);

    pMenuContextMenu->addAction(pActionMoveUp);
    pMenuContextMenu->addAction(pActionMoveDown);
    pMenuContextMenu->addAction(pActionDelete);
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

    bSelected = true;

    if (ev->button() == Qt::MouseButton::RightButton)
    {
        if (bSelected) pMenuContextMenu->exec(mapToGlobal(ev->pos()));
    }
}

void TrackWidget::setSelected()
{
    ui->frame->setProperty("cssClass", "trackWidget_selected");
    ui->frame->style()->unpolish(ui->frame);
    ui->frame->style()->polish(ui->frame);
    ui->frame->update();

    bSelected = true;
}

void TrackWidget::setPlaying()
{
    ui->frame->setProperty("cssClass", "trackWidget_playing");
    ui->frame->style()->unpolish(ui->frame);
    ui->frame->style()->polish(ui->frame);
    ui->frame->update();

    bSelected = false;
}

void TrackWidget::setIdle()
{
    ui->frame->setProperty("cssClass", "trackWidget");
    ui->frame->style()->unpolish(ui->frame);
    ui->frame->style()->polish(ui->frame);
    ui->frame->update();

    bSelected = false;
}

QString TrackWidget::getTrackTitle()
{
    return sTrackTitle;
}

TrackWidget::~TrackWidget()
{
    delete pActionDelete;
    delete pActionMoveDown;
    delete pActionMoveUp;
    delete pMenuContextMenu;

    delete ui;
}

void TrackWidget::slotMoveUp()
{
    emit signalMoveUp();
}

void TrackWidget::slotMoveDown()
{
    emit signalMoveDown();
}

void TrackWidget::slotDelete()
{
    emit signalDelete();
}
