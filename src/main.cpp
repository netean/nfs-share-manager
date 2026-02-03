/*
 * Copyright 2026 NFS Share Manager Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

#include "ui/nfssharemanager.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Set application metadata
    app.setApplicationName(QStringLiteral("NFS Share Manager"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("KDE"));
    app.setOrganizationDomain(QStringLiteral("kde.org"));
    app.setApplicationDisplayName(QStringLiteral("NFS Share Manager"));
    
    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("KDE NFS Share Manager - Manage NFS shares and mounts"));
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption minimizedOption(QStringList() << "m" << "minimized",
                                      QStringLiteral("Start minimized to system tray"));
    parser.addOption(minimizedOption);
    
    parser.process(app);
    
    // Create and show the main application
    NFSShareManager::NFSShareManagerApp nfsManager;
    
    if (!parser.isSet(minimizedOption)) {
        nfsManager.show();
    }
    
    return app.exec();
}