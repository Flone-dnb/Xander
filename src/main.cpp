// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "View/MainWindow/mainwindow.h"

#include <QApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    QTimer::singleShot(0, &w, &MainWindow::onExecCalled);

    if (argc > 1)
    {
        // Load tracks

        QStringList paths;
        for (int i = 1; i < argc; i++)
        {
            paths.push_back(a.arguments().at(i));
        }

        w.addTracksFromArgs(paths);
    }

    return a.exec();
}
