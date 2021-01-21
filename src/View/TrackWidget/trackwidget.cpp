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

TrackWidget::TrackWidget(QString sTrackTitle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TrackWidget)
{
    ui->setupUi(this);

    ui->label_track_title->setText(sTrackTitle);


    setProperty("cssClass", "trackWidget");
    style()->unpolish(this);
    style()->polish(this);
    update();
}

void TrackWidget::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

    deleteLater();
}

TrackWidget::~TrackWidget()
{
    delete ui;
}
