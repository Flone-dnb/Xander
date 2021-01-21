// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// Qt
#include <QMainWindow>

// STL
#include <mutex>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QSystemTrayIcon;
class QHideEvent;
class Controller;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


    void onExecCalled             ();


    void changePlayButtonStyle    (bool bChangeStyleToPause);
    void changeRepeatButtonStyle  (bool bActive);
    void changeRandomButtonStyle  (bool bActive);


    void showMessageBox           (std::wstring sMessageTitle, std::wstring sMessageText, bool bErrorMessage);

private slots:

    void  hideEvent               (QHideEvent  *event);
    void  slotTrayIconActivated   ();

    // UI slots.
    void  on_horizontalSlider_volume_valueChanged (int value);

    // QMenuBar.
    void  on_actionAbout_triggered();

private:

    void applyStyle               ();


    Ui::MainWindow * ui;

    QSystemTrayIcon* pTrayIcon;

    Controller*      pController;


    std::mutex       mtxButtonStateChange;


    bool             bPlayButtonStatePlay;
    bool             bRepeatButtonStateActive;
    bool             bRandomButtonStateActive;
};
