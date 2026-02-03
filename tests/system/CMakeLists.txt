# System integration tests

# PolicyKit Helper test
add_executable(test_policykithelper 
    test_policykithelper.cpp
    ${CMAKE_SOURCE_DIR}/src/system/policykithelper.cpp
)

# Set up MOC processing
set_target_properties(test_policykithelper PROPERTIES
    AUTOMOC ON
)

# Link required libraries
target_link_libraries(test_policykithelper
    Qt6::Core
    Qt6::Test
    Qt6::DBus
)

# Add to test suite
add_test(NAME PolicyKitHelperTest COMMAND test_policykithelper)

# Set test properties
set_tests_properties(PolicyKitHelperTest PROPERTIES
    TIMEOUT 30
    LABELS "system;integration"
)

# NFS Service Interface test
add_executable(test_nfsserviceinterface
    test_nfsserviceinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/system/nfsserviceinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/core/shareconfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/core/remotenfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
)

# Set up MOC processing
set_target_properties(test_nfsserviceinterface PROPERTIES
    AUTOMOC ON
)

# Link required libraries
target_link_libraries(test_nfsserviceinterface
    Qt6::Core
    Qt6::Test
    Qt6::Network
)

# Add to test suite
add_test(NAME NFSServiceInterfaceTest COMMAND test_nfsserviceinterface)

# Set test properties
set_tests_properties(NFSServiceInterfaceTest PROPERTIES
    TIMEOUT 60
    LABELS "system;integration;nfs"
)

# FileSystem Watcher test
add_executable(test_filesystemwatcher
    test_filesystemwatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/system/filesystemwatcher.cpp
)

# Set up MOC processing
set_target_properties(test_filesystemwatcher PROPERTIES
    AUTOMOC ON
)

# Link required libraries
target_link_libraries(test_filesystemwatcher
    Qt6::Core
    Qt6::Test
)

# Add to test suite
add_test(NAME FileSystemWatcherTest COMMAND test_filesystemwatcher)

# Set test properties
set_tests_properties(FileSystemWatcherTest PROPERTIES
    TIMEOUT 30
    LABELS "system;integration;filesystem"
)