// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "mainwindow.h"
#include "ui_mainwindow.h"

// Qt
#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QFile>
#include <QStyle>
#include <QScrollBar>
#include <QFileDialog>
#include <QKeyEvent>

// Custom
#include "Controller/controller.h"
#include "Model/globals.h"
#include "View/AboutWindow/aboutwindow.h"
#include "View/TrackWidget/trackwidget.h"
#include "View/AboutQtWindow/aboutqtwindow.h"
#include "View/SearchWindow/searchwindow.h"
#include "View/FXWindow/fxwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->scrollArea_tracklist->init(this);

    pSelectedTrack = nullptr;
    pPlayingTrack  = nullptr;

    bPlayButtonStatePlay = true;
    bRepeatButtonStateActive = false;
    bRandomButtonStateActive = false;

    // This to this.
    connect(this, &MainWindow::signalSetNewPlayingTrack, this, &MainWindow::slotSetNewPlayingTrack);
    connect(this, &MainWindow::signalChangePlayButtonStyle, this, &MainWindow::slotChangePlayButtonStyle);
    connect(this, &MainWindow::signalSetTrackInfo, this, &MainWindow::slotSetTrackInfo);
    connect(this, &MainWindow::signalShowMessageBox, this, &MainWindow::slotShowMessageBox);
}

void MainWindow::onExecCalled()
{
    // Setup tray icon.

    pTrayIcon = new QSystemTrayIcon(this);

    QIcon icon = QIcon(RES_LOGO_PATH);
    pTrayIcon->setIcon(icon);

    connect(pTrayIcon, &QSystemTrayIcon::activated, this, &MainWindow::slotTrayIconActivated);



    // Apply stylesheet.

    ui->pushButton_play->setProperty("cssClass", "play");
    ui->pushButton_stop->setProperty("cssClass", "stop");
    ui->pushButton_prev_track->setProperty("cssClass", "prev");
    ui->pushButton_next_track->setProperty("cssClass", "next");
    ui->pushButton_repeat->setProperty("cssClass", "repeat");
    ui->pushButton_random->setProperty("cssClass", "random");
    ui->pushButton_fx->setProperty("cssClass", "fx");
    ui->pushButton_clear->setProperty("cssClass", "clear");
    ui->scrollAreaWidgetContents->setProperty("cssClass", "tracklist_widget");

    ui->scrollAreaWidgetContents->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->verticalLayout_tracks->layout()->setSizeConstraint(QLayout::SetFixedSize);

    applyStyle();




    // Set default volume.

    ui->horizontalSlider_volume->setValue(DEFAULT_VOLUME);
    ui->label_volume->setText("Volume: " + QString::number(DEFAULT_VOLUME) + "%");



    // Create Controller.

    pController = new Controller(this);
}

void MainWindow::addTrackWidget(const std::wstring &sTrackTitle, std::promise<TrackWidget*> *pPromiseCreateWidget)
{
    std::lock_guard<std::mutex> lock(mtxUIStateChange);

    TrackWidget* pTrackWidget = new TrackWidget(QString::fromStdWString(sTrackTitle), this);

    ui->verticalLayout_tracks->addWidget(pTrackWidget);
    ui->scrollArea_tracklist->verticalScrollBar()->setSliderPosition(ui->scrollArea_tracklist->verticalScrollBar()->maximum());

    connect(pTrackWidget, &TrackWidget::signalMoveUp, this, &MainWindow::slotMoveUp);
    connect(pTrackWidget, &TrackWidget::signalMoveDown, this, &MainWindow::slotMoveDown);
    connect(pTrackWidget, &TrackWidget::signalDelete, this, &MainWindow::slotDeleteSelectedTrack);

    pPromiseCreateWidget->set_value(pTrackWidget);
}

void MainWindow::removeTrackWidget(TrackWidget *pTrackWidget, std::promise<bool> *pPromiseRemoveWidget)
{
    std::lock_guard<std::mutex> lock(mtxUIStateChange);

    if (pTrackWidget == pSelectedTrack)
    {
        pSelectedTrack = nullptr;
    }

    if (pTrackWidget == pPlayingTrack)
    {
        pPlayingTrack = nullptr;
        ui->label_track_name->setText("");
        ui->label_track_info->setText("");
    }

    //delete pTrackWidget; // 'delete' below is causing a crash if the context menu is opened on TrackWidget
    pTrackWidget->deleteLater();

    pPromiseRemoveWidget->set_value(false);
}

void MainWindow::showMessageBox(std::wstring sMessageTitle, std::wstring sMessageText, bool bErrorMessage, bool bSendSignal)
{
    if (bSendSignal)
    {
       emit signalShowMessageBox(QString::fromStdWString(sMessageTitle), QString::fromStdWString(sMessageText), bErrorMessage);
    }
    else
    {
        slotShowMessageBox(QString::fromStdWString(sMessageTitle), QString::fromStdWString(sMessageText), bErrorMessage);
    }
}

void MainWindow::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)

    hide();
    pTrayIcon->show();
}

void MainWindow::keyPressEvent(QKeyEvent *ev)
{
    if (ev->modifiers() == Qt::KeyboardModifier::ControlModifier && ev->key() == Qt::Key_F)
    {
        SearchWindow* pSearchWindow = new SearchWindow (this);
        pSearchWindow->setWindowModality(Qt::WindowModal);

        connect (pSearchWindow, &SearchWindow::findPrev,             this,          &MainWindow::slotSearchFindPrev);
        connect (pSearchWindow, &SearchWindow::findNext,             this,          &MainWindow::slotSearchFindNext);
        connect (pSearchWindow, &SearchWindow::searchTextChanged,    this,          &MainWindow::slotSearchTextSet);
        connect (this,          &MainWindow::signalSearchMatchCount, pSearchWindow, &SearchWindow::searchMatchCount);

        pSearchWindow->show();
    }
}

void MainWindow::slotTrayIconActivated()
{
    pTrayIcon->hide();

    raise          ();
    activateWindow ();
    showNormal     ();
}

void MainWindow::applyStyle()
{
    QFile File(QString("style.css"));

    if( File.exists() && File.open(QFile::ReadOnly) )
    {
        QString sStyleSheet = QLatin1String( File.readAll() );

        qApp->setStyleSheet(sStyleSheet);

        File.close();
    }
    else
    {
        QMessageBox::warning(this, "Error", "Could not find/open style file \"style.css\".");
    }
}

void MainWindow::changePlayButtonStyle(bool bChangeStyleToPause, bool bSendSignal)
{
    std::lock_guard<std::mutex> lock(mtxUIStateChange);

    if (bSendSignal)
    {
        std::promise<bool> promiseFinish;
        std::future<bool> f = promiseFinish.get_future();

        emit signalChangePlayButtonStyle(bChangeStyleToPause, &promiseFinish);

        f.get();
    }
    else
    {
        slotChangePlayButtonStyle(bChangeStyleToPause, nullptr);
    }
}

void MainWindow::changeRepeatButtonStyle(bool bActive)
{
    std::lock_guard<std::mutex> lock(mtxUIStateChange);

    if (bActive)
    {
        ui->pushButton_repeat->setProperty("cssClass", "repeat_active");
    }
    else
    {
        ui->pushButton_repeat->setProperty("cssClass", "repeat");
    }

    ui->pushButton_repeat->style()->unpolish(ui->pushButton_repeat);
    ui->pushButton_repeat->style()->polish(ui->pushButton_repeat);
    ui->pushButton_repeat->update();

    bRepeatButtonStateActive = bActive;
}

void MainWindow::changeRandomButtonStyle(bool bActive)
{
    std::lock_guard<std::mutex> lock(mtxUIStateChange);

    if (bActive)
    {
        ui->pushButton_random->setProperty("cssClass", "random_active");
    }
    else
    {
        ui->pushButton_random->setProperty("cssClass", "random");
    }

    ui->pushButton_random->style()->unpolish(ui->pushButton_random);
    ui->pushButton_random->style()->polish(ui->pushButton_random);
    ui->pushButton_random->update();

    bRandomButtonStateActive = bActive;
}

void MainWindow::setNewPlayingTrack(TrackWidget *pTrackWidget, bool bSendSignal)
{
    std::lock_guard<std::mutex> lock(mtxUIStateChange);

    if (bSendSignal)
    {
        std::promise<bool> promiseFinish;
        std::future<bool> f = promiseFinish.get_future();

        emit signalSetNewPlayingTrack(pTrackWidget, &promiseFinish);

        f.get();
    }
    else
    {
        slotSetNewPlayingTrack(pTrackWidget, nullptr);
    }
}

void MainWindow::setTrackInfo(const std::wstring &sTrackTitle, const std::wstring &sTrackInfo)
{
    std::lock_guard<std::mutex> lock(mtxUIStateChange);

    std::promise<bool> promiseFinish;

    std::future<bool> f = promiseFinish.get_future();

    emit signalSetTrackInfo(QString::fromStdWString(sTrackTitle), QString::fromStdWString(sTrackInfo), &promiseFinish);

    f.get();
}

void MainWindow::setSearchMatchCount(size_t iMatches)
{
    emit signalSearchMatchCount(iMatches);
}

void MainWindow::searchSetSelected(TrackWidget* pTrackWidget)
{
    on_trackWidget_mousePress(pTrackWidget);

    ui->scrollArea_tracklist->ensureWidgetVisible(pTrackWidget);
}

void MainWindow::on_horizontalSlider_volume_valueChanged(int value)
{
    ui->label_volume->setText("Volume: " + QString::number(value) + "%");

    pController->setVolume(value);
}

void MainWindow::on_actionAbout_triggered()
{
    AboutWindow* pAboutWindow = new AboutWindow(this);
    pAboutWindow->setWindowModality(Qt::WindowModality::WindowModal);

    pAboutWindow->show();
}

void MainWindow::on_actionOpen_File_triggered()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "Open File(s)", "", "Audio Files (*"
                                                      + QString::fromStdString(EXTENSION_MP3)
                                                      + " *" + QString::fromStdString(EXTENSION_WAV) +
                                                      "*" + QString::fromStdString(EXTENSION_OGG) + ")");

    if (files.size() > 0)
    {
        std::vector<std::wstring> vLocalPaths;

        for (int i = 0; i < files.size(); i++)
        {
            vLocalPaths.push_back(files[i].toStdWString());
        }

        pController->addTracks(vLocalPaths);
    }
}

void MainWindow::on_actionOpen_Folder_triggered()
{
    QString dir = QFileDialog::getExistingDirectory(nullptr, "Open Folder", "");

    if (dir != "")
    {
        pController->addTracks(dir.toStdWString());
    }
}

void MainWindow::on_trackWidget_mousePress(TrackWidget *pPressedTrack)
{
    std::lock_guard<std::mutex> lock(mtxUIStateChange);

    if (pSelectedTrack == nullptr)
    {
        // Nothing is selected.

        pSelectedTrack = pPressedTrack;

        pPressedTrack->setSelected();
    }
    else if (pSelectedTrack != pPressedTrack)
    {
        // Something is already selected.

        if (pSelectedTrack == pPlayingTrack)
        {
            pSelectedTrack->setPlaying();
        }
        else
        {
            pSelectedTrack->setIdle();
        }

        pSelectedTrack = pPressedTrack;

        pPressedTrack->setSelected();
    }
    else if (pSelectedTrack == pPressedTrack)
    {
        if (pPlayingTrack == pSelectedTrack)
        {
            pSelectedTrack->setPlaying();
        }
        else
        {
            pSelectedTrack->setIdle();
        }

        pSelectedTrack = nullptr;
    }
}

void MainWindow::on_trackWidget_mouseDoublePress(TrackWidget *pPressedTrack)
{
    mtxUIStateChange.lock();

    if (pPressedTrack == pSelectedTrack)
    {
        pSelectedTrack = nullptr;
    }

    if (pPlayingTrack == nullptr)
    {
        pPlayingTrack = pPressedTrack;

        pPressedTrack->setPlaying();
    }
    else if (pPlayingTrack != pPressedTrack)
    {
        pPlayingTrack->setIdle();

        pPlayingTrack = pPressedTrack;

        pPressedTrack->setPlaying();
    }

    mtxUIStateChange.unlock();

    pController->playTrack(pPressedTrack->getTrackTitle().toStdWString());
}

void MainWindow::slotSearchFindPrev()
{
    pController->searchFindPrev ();
}

void MainWindow::slotSearchFindNext()
{
    pController->searchFindNext ();
}

void MainWindow::slotSearchTextSet(QString keyword)
{
    pController->searchTextSet (keyword.toStdWString());
}

void MainWindow::slotMoveUp()
{
    std::lock_guard<std::mutex> lock(mtxUIStateChange);

    if (pSelectedTrack == nullptr)
    {
        return;
    }

    pController->moveUp(pSelectedTrack->getTrackTitle().toStdWString());


    int iTrackIndex = ui->verticalLayout_tracks->indexOf(pSelectedTrack);
    QLayoutItem* pItem = ui->verticalLayout_tracks->takeAt(iTrackIndex);

    if (iTrackIndex == 0)
    {
        ui->verticalLayout_tracks->addItem(pItem);
    }
    else
    {
        ui->verticalLayout_tracks->insertItem(iTrackIndex - 1, pItem);
    }


    // Deselect track widget.

    if (pSelectedTrack == pPlayingTrack)
    {
        pSelectedTrack->setPlaying();
    }
    else
    {
        pSelectedTrack->setIdle();
    }

    pSelectedTrack = nullptr;
}

void MainWindow::slotMoveDown()
{
    std::lock_guard<std::mutex> lock(mtxUIStateChange);

    if (pSelectedTrack == nullptr)
    {
        return;
    }

    pController->moveDown(pSelectedTrack->getTrackTitle().toStdWString());


    int iTrackIndex = ui->verticalLayout_tracks->indexOf(pSelectedTrack);
    QLayoutItem* pItem = ui->verticalLayout_tracks->takeAt(iTrackIndex);

    if (iTrackIndex == ui->verticalLayout_tracks->count())
    {
        ui->verticalLayout_tracks->insertItem(0, pItem);
    }
    else
    {
        ui->verticalLayout_tracks->insertItem(iTrackIndex + 1, pItem);
    }


    // Deselect track widget.

    if (pSelectedTrack == pPlayingTrack)
    {
        pSelectedTrack->setPlaying();
    }
    else
    {
        pSelectedTrack->setIdle();
    }

    pSelectedTrack = nullptr;
}

void MainWindow::slotDeleteSelectedTrack()
{
    if (pSelectedTrack == nullptr)
    {
        return;
    }

    pController->removeTrack(pSelectedTrack->getTrackTitle().toStdWString());
}

void MainWindow::slotSetNewPlayingTrack(TrackWidget *pTrackWidget, std::promise<bool>* pPromiseFinish)
{
    if (pTrackWidget == pSelectedTrack)
    {
        pSelectedTrack->setPlaying();
        pSelectedTrack = nullptr;

        pPlayingTrack = pTrackWidget;
    }

    if (pPlayingTrack == nullptr)
    {
        pPlayingTrack = pTrackWidget;

        pTrackWidget->setPlaying();
    }
    else if (pPlayingTrack != pTrackWidget)
    {
        pPlayingTrack->setIdle();

        pPlayingTrack = pTrackWidget;

        pTrackWidget->setPlaying();
    }

    ui->scrollArea_tracklist->ensureWidgetVisible(pTrackWidget);

    if (pPromiseFinish)
    {
        pPromiseFinish->set_value(false);
    }
}

void MainWindow::slotChangePlayButtonStyle(bool bChangeStyleToPause, std::promise<bool> *pPromiseFinish)
{
    if (bChangeStyleToPause)
    {
        ui->pushButton_play->setProperty("cssClass", "pause");
    }
    else
    {
        ui->pushButton_play->setProperty("cssClass", "play");
    }

    ui->pushButton_play->style()->unpolish(ui->pushButton_play);
    ui->pushButton_play->style()->polish(ui->pushButton_play);
    ui->pushButton_play->update();

    bPlayButtonStatePlay = !bChangeStyleToPause;

    if (pPromiseFinish)
    {
        pPromiseFinish->set_value(false);
    }
}

void MainWindow::slotShowMessageBox(QString sMessageTitle, QString sMessageText, bool bErrorMessage)
{
    if (bErrorMessage)
    {
        QMessageBox::warning(this, sMessageTitle, sMessageText);
    }
    else
    {
        QMessageBox::information(this, sMessageTitle, sMessageText);
    }
}

void MainWindow::slotSetTrackInfo(QString sTrackTitle, QString sTrackInfo, std::promise<bool>* pPromiseFinish)
{
    ui->label_track_name->setText(sTrackTitle);
    ui->label_track_info->setText(sTrackInfo);

    pPromiseFinish->set_value(false);
}

void MainWindow::on_pushButton_play_clicked()
{
    mtxUIStateChange.lock();

    if (bPlayButtonStatePlay)
    {
        // Nothing is playing.

        // Maybe track was stopped before.
        mtxUIStateChange.unlock();
        pController->playTrack();
    }
    else
    {
        // Trying to pause.
        mtxUIStateChange.unlock();
        pController->pauseTrack();
    }
}

void MainWindow::on_pushButton_stop_clicked()
{
    pController->stopTrack();
}

void MainWindow::on_pushButton_prev_track_clicked()
{
    pController->prevTrack();
}

void MainWindow::on_pushButton_clear_clicked()
{
    pController->clearTracklist();
}

void MainWindow::on_pushButton_repeat_clicked()
{
    pController->setRepeatTrack();
}

void MainWindow::on_pushButton_random_clicked()
{
    pController->setRandomTrack();
}

void MainWindow::on_pushButton_next_track_clicked()
{
    pController->nextTrack();
}

void MainWindow::on_actionAbout_Qt_triggered()
{
    AboutQtWindow* pAboutQtWindow = new AboutQtWindow(this);
    pAboutQtWindow->setWindowModality(Qt::WindowModality::WindowModal);

    pAboutQtWindow->show();
}

void MainWindow::on_actionSave_Tracklist_triggered()
{
    QString file = QFileDialog::getSaveFileName(nullptr, "Save Tracklist", "", "Xander Tracklist (*.xtl)");

    if (file != "")
    {
        pController->saveTracklist(file.toStdWString());
    }
}

void MainWindow::on_actionOpen_Tracklist_triggered()
{
    QString file = QFileDialog::getOpenFileName(nullptr, "Open Tracklist", "", "Xander Tracklist (*.xtl)");
    if (file != "")
    {
        QMessageBox::StandardButton answer = QMessageBox::question(nullptr, "Open Tracklist",
                                             "Do you want to clear the current tracklist? "
                                             "\nSelect \"No\" if you want to add tracks from the new tracklist to the current one.");
        if (answer == QMessageBox::StandardButton::Yes)
        {
            pController->openTracklist(file.toStdWString(), true);
        }
        else
        {
            pController->openTracklist(file.toStdWString(), false);
        }
    }
}

void MainWindow::on_pushButton_fx_clicked()
{
    FXWindow* pFXWindow = new FXWindow(this, this);
    pFXWindow->setWindowModality(Qt::WindowModality::WindowModal);

    pFXWindow->show();
}

MainWindow::~MainWindow()
{
    delete pController;

    delete pTrayIcon;

    delete ui;
}
