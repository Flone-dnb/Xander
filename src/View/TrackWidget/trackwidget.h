// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

#include <QWidget>

namespace Ui {
class TrackWidget;
}

class TrackWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TrackWidget(QString sTrackTitle, QWidget *parent = nullptr);
    ~TrackWidget() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    Ui::TrackWidget *ui;
};
