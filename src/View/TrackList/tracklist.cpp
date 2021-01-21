// ******************************************************************
// This file is part of the Xander.
// Copyright Aleksandr "Flone" Tretyakov (github.com/Flone-dnb).
// Licensed under the ZLib license.
// Refer to the LICENSE file included.
// ******************************************************************

#include "tracklist.h"
#include "ui_tracklist.h"

// Qt
#include <QDragEnterEvent>
#include <QMimeData>
#include <QDir>

// Custom
#include "View/MainWindow/mainwindow.h"
#include "Controller/controller.h"
#include "Model/globals.h"

TrackList::TrackList(QWidget *parent) :
    QScrollArea(parent),
    ui(new Ui::TrackList)
{
    ui->setupUi(this);
    setAcceptDrops(true);
}

void TrackList::init(MainWindow *pMainWindow)
{
    this->pMainWindow = pMainWindow;
}

TrackList::~TrackList()
{
    delete ui;
}

void TrackList::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls())
    {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();

        for (int i = 0; i < urlList.size(); i++)
        {
            pathList.append(urlList.at(i).toLocalFile());
        }

        QStringList filteredPaths;


        bool bFolderFound = false;

        // Look for extensions.
        for (int i = 0; i < pathList.size(); i++)
        {
            // Determine the start position of the extension.
            int iExtensionPos = -1;
            for (int j = pathList[i].size() - 1; j >=0; j--)
            {
                if (pathList[i][j] == '.')
                {
                    iExtensionPos = j + 1;
                    break;
                }
            }

            if (iExtensionPos != -1)
            {
                // File

                // Read the extension
                QString ext = "";
                for (int j = iExtensionPos; j < pathList[i].size(); j++)
                {
                    ext += pathList[i][j];
                }

                ext = ext.toUpper();

                std::string sMP3 = std::string(EXTENSION_MP3).substr(1, std::string(EXTENSION_MP3).size() - 1);
                std::string sWAV = std::string(EXTENSION_WAV).substr(1, std::string(EXTENSION_WAV).size() - 1);
                std::string sOGG = std::string(EXTENSION_OGG).substr(1, std::string(EXTENSION_OGG).size() - 1);

                if ( (ext == QString::fromStdString(sMP3).toUpper()) || (ext == QString::fromStdString(sWAV).toUpper())
                     || (ext == QString::fromStdString(sOGG).toUpper()) )
                {
                    filteredPaths.push_back(pathList[i]);
                }
            }
            else
            {
                // Folder (maybe)
                bFolderFound = true;
            }
        }

        if ( (filteredPaths.size() > 0) || bFolderFound)
        {
            event->acceptProposedAction();
        }
    }
}

void TrackList::dropEvent(QDropEvent *event)
{
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls())
    {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();

        for (int i = 0; i < urlList.size(); i++)
        {
            pathList.append(urlList.at(i).toLocalFile());
        }

        QStringList filteredPaths;

        filteredPaths = filterPaths(pathList);

        if (filteredPaths.size() > 0)
        {
            std::vector<std::wstring> vPaths;

            for (int i = 0; i < filteredPaths.size(); i++)
            {
                vPaths.push_back(filteredPaths[i].toStdWString());
            }

            pMainWindow->pController->addTracks(vPaths);
        }
    }
}

QStringList TrackList::filterPaths(QStringList paths)
{
    QStringList filteredPaths;

    // Look for extensions
    for (int i = 0; i < paths.size(); i++)
    {
        // Determine the start position of the extension.
        int iExtensionPos = -1;
        for (int j = paths[i].size() - 1; j >=0; j--)
        {
            if (paths[i][j] == '.')
            {
                iExtensionPos = j + 1;
                break;
            }
        }

        if (iExtensionPos != -1)
        {
            // File

            // Read the extension
            QString ext = "";
            for (int j = iExtensionPos; j < paths[i].size(); j++)
            {
                ext += paths[i][j];
            }

            ext = ext.toUpper();

            std::string sMP3 = std::string(EXTENSION_MP3).substr(1, std::string(EXTENSION_MP3).size() - 1);
            std::string sWAV = std::string(EXTENSION_WAV).substr(1, std::string(EXTENSION_WAV).size() - 1);
            std::string sOGG = std::string(EXTENSION_OGG).substr(1, std::string(EXTENSION_OGG).size() - 1);

            if ( (ext == QString::fromStdString(sMP3).toUpper()) || (ext == QString::fromStdString(sWAV).toUpper())
                 || (ext == QString::fromStdString(sOGG).toUpper()) )
            {
                filteredPaths.push_back(paths[i]);
            }
        }
        else
        {
            // Folder
            QDir folder(paths[i]);

            QStringList in = folder.entryList( QDir::Dirs | QDir::Files );
            for (int j = 0; j < in.size(); j++)
            {
                QString fullPath = folder.absolutePath() + "/";
                in[j] = fullPath + in[j];
            }


            QStringList files = filterPaths( in );
            for (int j = 0; j < files.size(); j++)
            {
                filteredPaths.push_back(files[j]);
            }
        }
    }

    return filteredPaths;
}
