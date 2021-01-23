// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#pragma once

#include <QMainWindow>

namespace Ui {
class SearchWindow;
}

class SearchWindow : public QMainWindow
{
    Q_OBJECT

signals:
    void findNext ();
    void findPrev ();
    void searchTextChanged (QString keyword);

public:
    explicit SearchWindow(QWidget *parent = nullptr);
    ~SearchWindow();

public slots:
    void searchMatchCount (size_t iCount);

protected:

    void keyPressEvent  (QKeyEvent* ev);
    void closeEvent     (QCloseEvent* ev);

private slots:
    void on_pushButton_find_prev_clicked();

    void on_pushButton_find_next_clicked();

    void on_lineEdit_key_textChanged(const QString &arg1);

private:
    Ui::SearchWindow *ui;
};
