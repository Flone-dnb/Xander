// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

#include <QMainWindow>

namespace Ui {
class FXWindow;
}

class MainWindow;

class FXWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit FXWindow(MainWindow* pMainWindow, QWidget *parent = nullptr);
    ~FXWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    // Sliders.
    void on_horizontalSlider_pitch_valueChanged(int value);
    void on_horizontalSlider_reverb_valueChanged(int value);

    // Reset buttons.
    void on_pushButton_pitch_clicked();
    void on_pushButton_reverb_clicked();

private:

    void applyCurrentEffects();

    Ui::FXWindow *ui;
    MainWindow* pMainWindow;
};
