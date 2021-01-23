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

signals:

    // This to this.
    void signalSetNewPlayingTrack    (TrackWidget* pTrackWidget, std::promise<bool>* pPromiseFinish);
    void signalChangePlayButtonStyle (bool bChangeStyleToPause, std::promise<bool>* pPromiseFinish);

    void signalSearchMatchCount      (size_t iCount);

    void signalShowMessageBox        (QString sMessageTitle, QString sMessageText, bool bErrorMessage);

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


    void onExecCalled             ();


    void addTrackWidget           (const std::wstring& sTrackTitle, std::promise<TrackWidget*>* pPromiseCreateWidget);
    void removeTrackWidget        (TrackWidget* pTrackWidget, std::promise<bool>* pPromiseRemoveWidget);


    void changePlayButtonStyle    (bool bChangeStyleToPause, bool bSendSignal);
    void changeRepeatButtonStyle  (bool bActive);
    void changeRandomButtonStyle  (bool bActive);


    void setNewPlayingTrack       (TrackWidget* pTrackWidget, bool bSendSignal);


    void setSearchMatchCount      (size_t iMatches);
    void searchSetSelected        (TrackWidget* pTrackWidget);


    void showMessageBox           (std::wstring sMessageTitle, std::wstring sMessageText, bool bErrorMessage, bool bSendSignal);


protected:

    void  hideEvent               (QHideEvent  *event);
    void  keyPressEvent           (QKeyEvent* ev);

    // Track Widgets.
    void  on_trackWidget_mousePress       (TrackWidget* pPressedTrack);
    void  on_trackWidget_mouseDoublePress (TrackWidget* pPressedTrack);

public slots:

    // Search Window
    void  slotSearchFindPrev  ();
    void  slotSearchFindNext  ();
    void  slotSearchTextSet   (QString keyword);

    // Context menu on TrackWidget
    void  slotMoveUp          ();
    void  slotMoveDown        ();
    void  slotDeleteSelectedTrack ();

private slots:

    // This to this.
    void  slotSetNewPlayingTrack          (TrackWidget* pTrackWidget, std::promise<bool>* pPromiseFinish);
    void  slotChangePlayButtonStyle       (bool bChangeStyleToPause, std::promise<bool>* pPromiseFinish);
    void  slotShowMessageBox              (QString sMessageTitle, QString sMessageText, bool bErrorMessage);

    // Tray icon.
    void  slotTrayIconActivated           ();

    // Main UI button.
    void  on_pushButton_play_clicked      ();
    void  on_pushButton_stop_clicked      ();
    void  on_pushButton_prev_track_clicked();
    void  on_pushButton_clear_clicked     ();
    void  on_pushButton_repeat_clicked    ();
    void  on_pushButton_random_clicked    ();
    void  on_pushButton_next_track_clicked();

    // Volume slider.
    void  on_horizontalSlider_volume_valueChanged (int value);

    // QMenuBar.
    void  on_actionAbout_triggered        ();
    void  on_actionOpen_File_triggered    ();
    void  on_actionOpen_Folder_triggered  ();
    void  on_actionAbout_Qt_triggered     ();

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
