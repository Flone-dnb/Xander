// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

#include <QScrollArea>

namespace Ui {
class TrackList;
}

class QDragEnterEvent;
class QDropEvent;
class MainWindow;

class TrackList : public QScrollArea
{
    Q_OBJECT

public:
    explicit TrackList(QWidget *parent = nullptr);
    void init(MainWindow* pMainWindow);
    ~TrackList();

protected:
    void  dragEnterEvent          (QDragEnterEvent* event);
    void  dropEvent               (QDropEvent* event);

private:

    QStringList filterPaths       (QStringList paths);

    Ui::TrackList *ui;
    MainWindow* pMainWindow;
};
