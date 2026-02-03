cmake_minimum_required(VERSION 3.16)

project(nfs-share-manager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required Qt6 components
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    DBus
    Test
)

# Find required KDE Frameworks 6 components
# Note: KF6 not available in this environment, using Qt6 equivalents for now
# In a real KDE environment, uncomment the following:
# find_package(KF6 REQUIRED COMPONENTS
#     I18n
#     Config
#     ConfigWidgets
#     CoreAddons
#     WidgetsAddons
#     Notifications
#     Auth
#     KIO
#     WindowSystem
# )

# Set up include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Enable testing
enable_testing()

# Install PolicyKit policy file
install(FILES policy/org.kde.nfs-share-manager.policy 
        DESTINATION ${CMAKE_INSTALL_PREFIX}/share/polkit-1/actions/)

# Install desktop file and icons (placeholder for future)
# install(FILES nfs-share-manager.desktop DESTINATION ${KDE_INSTALL_APPDIR})