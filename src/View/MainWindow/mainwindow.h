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
class QCPItemText;
class QCPItemRect;

class MainWindow : public QMainWindow
{
    Q_OBJECT

signals:

    // This to this.
    void signalSetNewPlayingTrack    (TrackWidget* pTrackWidget, std::promise<bool>* pPromiseFinish);
    void signalChangePlayButtonStyle (bool bChangeStyleToPause, std::promise<bool>* pPromiseFinish);
    void signalShowMessageBox        (QString sMessageTitle, QString sMessageText, bool bErrorMessage);
    void signalSetTrackInfo          (QString sTrackTitle, QString sTrackInfo, std::promise<bool>* pPromiseFinish);
    void signalClearGraph            ();
    void signalSetMaxXToGraph        (unsigned int iMaxX);
    void signalAddWaveDataToGraph    (std::vector<float> vWaveData);
    void signalSetCurrentPos         (double x, QString sTime);

    void signalSearchMatchCount      (size_t iCount);

public:

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


    void onExecCalled             ();
    void addTracksFromArgs        (QStringList paths);


    void addTrackWidget           (const std::wstring& sTrackTitle, std::promise<TrackWidget*>* pPromiseCreateWidget);
    void removeTrackWidget        (TrackWidget* pTrackWidget, std::promise<bool>* pPromiseRemoveWidget);


    void changePlayButtonStyle    (bool bChangeStyleToPause, bool bSendSignal);
    void changeRepeatButtonStyle  (bool bActive);
    void changeRandomButtonStyle  (bool bActive);


    void setNewPlayingTrack       (TrackWidget* pTrackWidget, bool bSendSignal);
    void setTrackInfo             (const std::wstring& sTrackTitle, const std::wstring& sTrackInfo);


    void setSearchMatchCount      (size_t iMatches);
    void searchSetSelected        (TrackWidget* pTrackWidget);


    void clearGraph               ();
    void setMaxXToGraph           (unsigned int iMaxX);
    void addWaveDataToGraph       (std::vector<float> vWaveData);
    void setCurrentPos            (double x, const std::string& sTime);


    unsigned int getMaxXPosOnGraph();


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
    void  slotSetTrackInfo                (QString sTrackTitle, QString sTrackInfo, std::promise<bool>* pPromiseFinish);
    void  slotClearGraph                  ();
    void  slotSetMaxXToGraph              (unsigned int iMaxX);
    void  slotAddWaveDataToGraph          (std::vector<float> vWaveData);
    void  slotSetCurrentPos               (double x, QString sTime);

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
    void  on_pushButton_fx_clicked        ();

    // Volume slider.
    void  on_horizontalSlider_volume_valueChanged (int value);

    // QMenuBar.
    void  on_actionAbout_triggered        ();
    void  on_actionOpen_File_triggered    ();
    void  on_actionOpen_Folder_triggered  ();
    void  on_actionAbout_Qt_triggered     ();
    void  on_actionSave_Tracklist_triggered();
    void  on_actionOpen_Tracklist_triggered();

    // Oscillogram.
    void  slotClickOnGraph                 (QMouseEvent* ev);

private:

    friend class TrackList;
    friend class TrackWidget;
    friend class FXWindow;

    void setupGraph ();
    void applyStyle ();

    QStringList      args;

    Ui::MainWindow * ui;

    QSystemTrayIcon* pTrayIcon;
    QCPItemText*     pGraphTextTrackTime;
    QCPItemRect*     backgnd;
    QCPItemRect*     backgndRight;

    Controller*      pController;


    std::mutex       mtxUIStateChange;
    std::mutex       mtxDrawGraph;


    TrackWidget*     pSelectedTrack;
    TrackWidget*     pPlayingTrack;


    double           minPosOnGraphForText;
    double           maxPosOnGraphForText;
    unsigned int     iCurrentXPosOnGraph;
    unsigned int     iMaxXOnGraph;


    bool             bPlayButtonStatePlay;
    bool             bRepeatButtonStateActive;
    bool             bRandomButtonStateActive;
};
