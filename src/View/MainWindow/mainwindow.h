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
#include <future>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QSystemTrayIcon;
class QHideEvent;
class Controller;
class TrackWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


    void onExecCalled             ();


    void addTrackWidget           (const std::wstring& sTrackTitle, std::promise<TrackWidget*>* pPromiseCreateWidget);
    void removeTrackWidget        (TrackWidget* pTrackWidget, std::promise<bool>* pPromiseRemoveWidget);


    void changePlayButtonStyle    (bool bChangeStyleToPause);
    void changeRepeatButtonStyle  (bool bActive);
    void changeRandomButtonStyle  (bool bActive);

    void setNewPlayingTrack       (TrackWidget* pTrackWidget);


    void showMessageBox           (std::wstring sMessageTitle, std::wstring sMessageText, bool bErrorMessage);


protected:

    void  hideEvent               (QHideEvent  *event);

    // Track Widgets.
    void  on_trackWidget_mousePress       (TrackWidget* pPressedTrack);
    void  on_trackWidget_mouseDoublePress (TrackWidget* pPressedTrack);

private slots:

    void  slotTrayIconActivated           ();

    // Main UI button.
    void  on_pushButton_play_clicked      ();
    void  on_pushButton_stop_clicked      ();
    void  on_pushButton_prev_track_clicked();

    // Volume slider.
    void  on_horizontalSlider_volume_valueChanged (int value);

    // QMenuBar.
    void  on_actionAbout_triggered        ();
    void  on_actionOpen_File_triggered    ();
    void  on_actionOpen_Folder_triggered  ();

private:

    friend class TrackList;
    friend class TrackWidget;

    void applyStyle               ();


    Ui::MainWindow * ui;

    QSystemTrayIcon* pTrayIcon;

    Controller*      pController;


    std::mutex       mtxUIStateChange;


    TrackWidget*     pSelectedTrack;
    TrackWidget*     pPlayingTrack;


    bool             bPlayButtonStatePlay;
    bool             bRepeatButtonStateActive;
    bool             bRandomButtonStateActive;
};
