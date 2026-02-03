# Core data structures and models
set(CORE_SOURCES
    core/types.cpp
    core/nfsshare.cpp
    core/shareconfiguration.cpp
    core/remotenfsshare.cpp
    core/nfsmount.cpp
    core/permissionset.cpp
    core/configurationmanager.cpp
    core/errorhandling.cpp
    core/auditlogger.cpp
    core/startupmanager.cpp
)

set(CORE_HEADERS
    core/nfsshare.h
    core/shareconfiguration.h
    core/remotenfsshare.h
    core/nfsmount.h
    core/permissionset.h
    core/types.h
    core/configurationmanager.h
    core/errorhandling.h
    core/auditlogger.h
    core/startupmanager.h
)

# System integration layer
set(SYSTEM_SOURCES
    system/policykithelper.cpp
    system/nfsserviceinterface.cpp
    system/filesystemwatcher.cpp
    system/networkmonitor.cpp
)

set(SYSTEM_HEADERS
    system/policykithelper.h
    system/nfsserviceinterface.h
    system/filesystemwatcher.h
    system/networkmonitor.h
)

# Business logic layer
set(BUSINESS_SOURCES
    business/sharemanager.cpp
    business/mountmanager.cpp
    business/networkdiscovery.cpp
    business/permissionmanager.cpp
)

set(BUSINESS_HEADERS
    business/sharemanager.h
    business/mountmanager.h
    business/networkdiscovery.h
    business/permissionmanager.h
)

# UI layer
set(UI_SOURCES
    ui/nfssharemanager.cpp
    ui/mainwindow.cpp
    ui/systemtrayicon.cpp
    ui/sharecreatedialog.cpp
    ui/shareconfigdialog.cpp
    ui/mountdialog.cpp
    ui/conflictresolutiondialog.cpp
    ui/permissionmanagerdialog.cpp
    ui/notificationmanager.cpp
    ui/notificationpreferencesdialog.cpp
    ui/progressdialog.cpp
    ui/operationmanager.cpp
    ui/startupguidancedialog.cpp
)

set(UI_HEADERS
    ui/nfssharemanager.h
    ui/mainwindow.h
    ui/systemtrayicon.h
    ui/sharecreatedialog.h
    ui/shareconfigdialog.h
    ui/mountdialog.h
    ui/conflictresolutiondialog.h
    ui/permissionmanagerdialog.h
    ui/notificationmanager.h
    ui/notificationpreferencesdialog.h
    ui/progressdialog.h
    ui/operationmanager.h
    ui/startupguidancedialog.h
)

# Main application
set(MAIN_SOURCES
    main.cpp
)

# Combine all sources
set(ALL_SOURCES
    ${CORE_SOURCES}
    ${SYSTEM_SOURCES}
    ${BUSINESS_SOURCES}
    ${UI_SOURCES}
    ${MAIN_SOURCES}
)

set(ALL_HEADERS
    ${CORE_HEADERS}
    ${SYSTEM_HEADERS}
    ${BUSINESS_HEADERS}
    ${UI_HEADERS}
)

# Create the executable
add_executable(nfs-share-manager ${ALL_SOURCES} ${ALL_HEADERS})

# Enable Qt MOC processing
set_target_properties(nfs-share-manager PROPERTIES
    AUTOMOC ON
)

# Link required libraries
target_link_libraries(nfs-share-manager
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::DBus
    # KDE Frameworks would be linked here in a real KDE environment:
    # KF6::I18n
    # KF6::Config
    # KF6::ConfigWidgets
    # KF6::CoreAddons
    # KF6::WidgetsAddons
    # KF6::Notifications
    # KF6::Auth
    # KF6::KIOCore
    # KF6::WindowSystem
)

# Install the executable
install(TARGETS nfs-share-manager DESTINATION ${KDE_INSTALL_BINDIR})