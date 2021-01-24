// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "fxwindow.h"
#include "ui_fxwindow.h"

// Custom
#include "View/MainWindow/mainwindow.h"
#include "Controller/controller.h"
#include "Model/globals.h"

FXWindow::FXWindow(MainWindow* pMainWindow, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FXWindow)
{
    ui->setupUi(this);

    this->pMainWindow = pMainWindow;

    setFixedSize (width(), height());

    applyCurrentEffects();
}

FXWindow::~FXWindow()
{
    delete ui;
}

void FXWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event)

    deleteLater();
}

void FXWindow::applyCurrentEffects()
{
    CurrentEffects* pEffects = pMainWindow->pController->getCurrentEffects();

    // Pan
    ui->horizontalSlider_pitch->setValue(static_cast<int>(pEffects->fPitchInSemitones));
    ui->horizontalSlider_reverb->setValue(static_cast<int>(pEffects->fReverbVolume * 100.0f));
}

void FXWindow::on_horizontalSlider_pitch_valueChanged(int value)
{
    ui->label_pitch->setText("Pitch: " + QString::number(value) + " semitones");

    pMainWindow->pController->setPitch(value);
}

void FXWindow::on_pushButton_pitch_clicked()
{
    ui->horizontalSlider_pitch->setValue(0);
}

void FXWindow::on_horizontalSlider_reverb_valueChanged(int value)
{
    ui->label_reverb->setText("Reverb Volume: " + QString::number(value) + "%");

    pMainWindow->pController->setReverbVolume(value / 100.0f);
}

void FXWindow::on_pushButton_reverb_clicked()
{
    ui->horizontalSlider_reverb->setValue(0);
}
