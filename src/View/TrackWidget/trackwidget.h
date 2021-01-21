// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// Qt
#include <QWidget>

namespace Ui {
class TrackWidget;
}

class MainWindow;

class TrackWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrackWidget(QString sTrackTitle, MainWindow* pMainWindow, QWidget *parent = nullptr);
    ~TrackWidget() override;

    void setSelected ();
    void setPlaying  ();
    void setIdle     ();

    QString getTrackTitle();

protected:
    void closeEvent            (QCloseEvent* event) override;
    void mouseDoubleClickEvent (QMouseEvent* ev) override;
    void mousePressEvent       (QMouseEvent* ev) override;

private:
    Ui::TrackWidget *ui;
    MainWindow* pMainWindow;

    QString sTrackTitle;
};
