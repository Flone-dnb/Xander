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
class QMenu;

class TrackWidget : public QWidget
{
    Q_OBJECT

signals:

    // Context menu signals
    void signalDelete();
    void signalMoveUp();
    void signalMoveDown();

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

private slots:

    // Context menu
    void slotMoveUp   ();
    void slotMoveDown ();
    void slotDelete   ();

private:
    Ui::TrackWidget *ui;
    MainWindow* pMainWindow;

    // Context menu
    QMenu* pMenuContextMenu;
        QAction* pActionMoveUp;
        QAction* pActionMoveDown;
        QAction* pActionDelete;

    QString sTrackTitle;

    bool bSelected;
};
