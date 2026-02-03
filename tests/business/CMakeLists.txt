# Business logic tests

# ShareManager test
add_executable(test_sharemanager test_sharemanager.cpp
    ${CMAKE_SOURCE_DIR}/src/business/sharemanager.cpp
    ${CMAKE_SOURCE_DIR}/src/system/policykithelper.cpp
    ${CMAKE_SOURCE_DIR}/src/system/nfsserviceinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/core/nfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/shareconfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/core/permissionset.cpp
    ${CMAKE_SOURCE_DIR}/src/core/remotenfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
    ${CMAKE_SOURCE_DIR}/src/core/errorhandling.cpp
)
target_link_libraries(test_sharemanager
    Qt6::Test
    Qt6::Core
    Qt6::DBus
    Qt6::Network
)

# Enable automoc for this target
set_target_properties(test_sharemanager PROPERTIES AUTOMOC ON)

add_test(NAME ShareManagerTest COMMAND test_sharemanager)
set_tests_properties(ShareManagerTest PROPERTIES LABELS "business")

# MountManager test
add_executable(test_mountmanager test_mountmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/business/mountmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/system/policykithelper.cpp
    ${CMAKE_SOURCE_DIR}/src/system/nfsserviceinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/core/nfsmount.cpp
    ${CMAKE_SOURCE_DIR}/src/core/remotenfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/shareconfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/core/permissionset.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
    ${CMAKE_SOURCE_DIR}/src/core/errorhandling.cpp
)
target_link_libraries(test_mountmanager
    Qt6::Test
    Qt6::Core
    Qt6::DBus
    Qt6::Network
)

# Enable automoc for this target
set_target_properties(test_mountmanager PROPERTIES AUTOMOC ON)

add_test(NAME MountManagerTest COMMAND test_mountmanager)
set_tests_properties(MountManagerTest PROPERTIES LABELS "business")

# NetworkDiscovery test
add_executable(test_networkdiscovery test_networkdiscovery.cpp
    ${CMAKE_SOURCE_DIR}/src/business/networkdiscovery.cpp
    ${CMAKE_SOURCE_DIR}/src/system/networkmonitor.cpp
    ${CMAKE_SOURCE_DIR}/src/system/nfsserviceinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/core/remotenfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/shareconfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/core/permissionset.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
    ${CMAKE_SOURCE_DIR}/src/core/errorhandling.cpp
)
target_link_libraries(test_networkdiscovery
    Qt6::Test
    Qt6::Core
    Qt6::Network
)

# Enable automoc for this target
set_target_properties(test_networkdiscovery PROPERTIES AUTOMOC ON)

add_test(NAME NetworkDiscoveryTest COMMAND test_networkdiscovery)
set_tests_properties(NetworkDiscoveryTest PROPERTIES LABELS "business")

# Business Integration test
add_executable(test_business_integration test_business_integration.cpp
    ${CMAKE_SOURCE_DIR}/src/business/sharemanager.cpp
    ${CMAKE_SOURCE_DIR}/src/business/mountmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/business/networkdiscovery.cpp
    ${CMAKE_SOURCE_DIR}/src/system/policykithelper.cpp
    ${CMAKE_SOURCE_DIR}/src/system/nfsserviceinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/system/networkmonitor.cpp
    ${CMAKE_SOURCE_DIR}/src/core/nfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/nfsmount.cpp
    ${CMAKE_SOURCE_DIR}/src/core/remotenfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/shareconfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/core/permissionset.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
    ${CMAKE_SOURCE_DIR}/src/core/errorhandling.cpp
)
target_link_libraries(test_business_integration
    Qt6::Test
    Qt6::Core
    Qt6::DBus
    Qt6::Network
)

# Enable automoc for this target
set_target_properties(test_business_integration PROPERTIES AUTOMOC ON)

add_test(NAME BusinessIntegrationTest COMMAND test_business_integration)
set_tests_properties(BusinessIntegrationTest PROPERTIES LABELS "business")