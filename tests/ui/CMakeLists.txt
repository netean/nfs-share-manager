# UI component tests

# Notification Manager test
add_executable(test_notificationmanager
    test_notificationmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/notificationmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/configurationmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/nfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/nfsmount.cpp
    ${CMAKE_SOURCE_DIR}/src/core/remotenfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/shareconfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/core/permissionset.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
)

target_link_libraries(test_notificationmanager
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
)

set_target_properties(test_notificationmanager PROPERTIES
    AUTOMOC ON
)

add_test(NAME test_notificationmanager COMMAND test_notificationmanager)

# Permission Manager Dialog test
add_executable(test_permissionmanagerdialog
    test_permissionmanagerdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/permissionmanagerdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/core/permissionset.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
)

target_link_libraries(test_permissionmanagerdialog
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
)

set_target_properties(test_permissionmanagerdialog PROPERTIES
    AUTOMOC ON
)

add_test(NAME test_permissionmanagerdialog COMMAND test_permissionmanagerdialog)

# Permission Manager Basic test (without complex GUI interactions)
add_executable(test_permissionmanager_basic
    test_permissionmanager_basic.cpp
    ${CMAKE_SOURCE_DIR}/src/ui/permissionmanagerdialog.cpp
    ${CMAKE_SOURCE_DIR}/src/core/permissionset.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
)

target_link_libraries(test_permissionmanager_basic
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
)

set_target_properties(test_permissionmanager_basic PROPERTIES
    AUTOMOC ON
)

add_test(NAME test_permissionmanager_basic COMMAND test_permissionmanager_basic)

# Permission Integration test (core functionality only)
add_executable(test_permission_integration
    test_permission_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/core/permissionset.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
)

target_link_libraries(test_permission_integration
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::Test
)

set_target_properties(test_permission_integration PROPERTIES
    AUTOMOC ON
)

add_test(NAME test_permission_integration COMMAND test_permission_integration)

# Share Create Dialog test (if needed in the future)
# add_executable(test_sharecreatedialog
#     test_sharecreatedialog.cpp
#     ${CMAKE_SOURCE_DIR}/src/ui/sharecreatedialog.cpp
#     ${CMAKE_SOURCE_DIR}/src/ui/permissionmanagerdialog.cpp
#     ${CMAKE_SOURCE_DIR}/src/core/permissionset.cpp
#     ${CMAKE_SOURCE_DIR}/src/core/shareconfiguration.cpp
#     ${CMAKE_SOURCE_DIR}/src/core/types.cpp
# )
# 
# target_link_libraries(test_sharecreatedialog
#     Qt6::Core
#     Qt6::Widgets
#     Qt6::Test
# )
# 
# set_target_properties(test_sharecreatedialog PROPERTIES
#     AUTOMOC ON
# )
# 
# add_test(NAME test_sharecreatedialog COMMAND test_sharecreatedialog)