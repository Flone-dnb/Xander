// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

// Qt
#include <QMainWindow>

namespace Ui {
class AboutWindow;
}

class QCloseEvent;

class AboutWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit AboutWindow(QWidget *parent = nullptr);
    ~AboutWindow();

protected:

    void closeEvent(QCloseEvent* event);

private slots:

    void on_pushButton_github_clicked();

private:

    Ui::AboutWindow *ui;
};
