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