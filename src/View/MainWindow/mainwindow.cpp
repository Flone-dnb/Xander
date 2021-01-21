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

// Custom
#include "Controller/controller.h"
#include "Model/globals.h"
#include "View/AboutWindow/aboutwindow.h"
#include "View/TrackWidget/trackwidget.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->scrollArea_tracklist->init(this);

    bPlayButtonStatePlay = true;
    bRepeatButtonStateActive = false;
    bRandomButtonStateActive = false;
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
    std::lock_guard<std::mutex> lock(mtxButtonStateChange);

    TrackWidget* pTrackWidget = new TrackWidget(QString::fromStdWString(sTrackTitle), this);

    ui->verticalLayout_tracks->addWidget(pTrackWidget);
    ui->scrollArea_tracklist->verticalScrollBar()->setSliderPosition(ui->scrollArea_tracklist->verticalScrollBar()->maximum());

    pPromiseCreateWidget->set_value(pTrackWidget);
}

void MainWindow::removeTrackWidget(TrackWidget *pTrackWidget, std::promise<bool> *pPromiseRemoveWidget)
{
    std::lock_guard<std::mutex> lock(mtxButtonStateChange);

    delete pTrackWidget;

    pPromiseRemoveWidget->set_value(false);
}

void MainWindow::showMessageBox(std::wstring sMessageTitle, std::wstring sMessageText, bool bErrorMessage)
{
    if (bErrorMessage)
    {
        QMessageBox::warning(this, QString::fromStdWString(sMessageTitle), QString::fromStdWString(sMessageText));
    }
    else
    {
        QMessageBox::information(this, QString::fromStdWString(sMessageTitle), QString::fromStdWString(sMessageText));
    }
}

void MainWindow::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event)

    hide();
    pTrayIcon->show();
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

void MainWindow::changePlayButtonStyle(bool bChangeStyleToPause)
{
    std::lock_guard<std::mutex> lock(mtxButtonStateChange);

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
}

void MainWindow::changeRepeatButtonStyle(bool bActive)
{
    std::lock_guard<std::mutex> lock(mtxButtonStateChange);

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
    std::lock_guard<std::mutex> lock(mtxButtonStateChange);

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

void MainWindow::on_horizontalSlider_volume_valueChanged(int value)
{
    ui->label_volume->setText("Volume: " + QString::number(value) + "%");
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

MainWindow::~MainWindow()
{
    delete pController;

    delete pTrayIcon;

    delete ui;
}
