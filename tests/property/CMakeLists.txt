# Property-based testing infrastructure

# Create property testing utilities
add_library(property_test_utils STATIC
    property_test_base.cpp
    generators.cpp
)

set_target_properties(property_test_utils PROPERTIES
    AUTOMOC ON
)

target_link_libraries(property_test_utils
    Qt6::Core
    Qt6::Test
    Qt6::Network
)

target_include_directories(property_test_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Property tests will be added here as they are implemented
# Each property test will link against property_test_utils

# Network Discovery Property Tests
add_executable(test_property_networkdiscovery
    test_property_networkdiscovery.cpp
    ${CMAKE_SOURCE_DIR}/src/business/networkdiscovery.cpp
    ${CMAKE_SOURCE_DIR}/src/system/networkmonitor.cpp
    ${CMAKE_SOURCE_DIR}/src/system/nfsserviceinterface.cpp
    ${CMAKE_SOURCE_DIR}/src/core/remotenfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/nfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/nfsmount.cpp
    ${CMAKE_SOURCE_DIR}/src/core/shareconfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/core/permissionset.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
    ${CMAKE_SOURCE_DIR}/src/core/errorhandling.cpp
)

set_target_properties(test_property_networkdiscovery PROPERTIES
    AUTOMOC ON
)

target_link_libraries(test_property_networkdiscovery
    property_test_utils
    Qt6::Core
    Qt6::Test
    Qt6::Network
)

add_test(NAME PropertyNetworkDiscovery COMMAND test_property_networkdiscovery)

# Network Monitor Property Tests
add_executable(test_property_networkmonitor
    test_property_networkmonitor.cpp
    ${CMAKE_SOURCE_DIR}/src/system/networkmonitor.cpp
    ${CMAKE_SOURCE_DIR}/src/core/remotenfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/nfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/nfsmount.cpp
    ${CMAKE_SOURCE_DIR}/src/core/shareconfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/core/permissionset.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
)

set_target_properties(test_property_networkmonitor PROPERTIES
    AUTOMOC ON
)

target_link_libraries(test_property_networkmonitor
    property_test_utils
    Qt6::Core
    Qt6::Test
    Qt6::Network
)

add_test(NAME PropertyNetworkMonitor COMMAND test_property_networkmonitor)

# Configuration Manager Property Tests
add_executable(test_property_configurationmanager
    test_property_configurationmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/configurationmanager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/nfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/nfsmount.cpp
    ${CMAKE_SOURCE_DIR}/src/core/remotenfsshare.cpp
    ${CMAKE_SOURCE_DIR}/src/core/shareconfiguration.cpp
    ${CMAKE_SOURCE_DIR}/src/core/permissionset.cpp
    ${CMAKE_SOURCE_DIR}/src/core/types.cpp
)

set_target_properties(test_property_configurationmanager PROPERTIES
    AUTOMOC ON
)

target_link_libraries(test_property_configurationmanager
    property_test_utils
    Qt6::Core
    Qt6::Test
    Qt6::Network
)

add_test(NAME PropertyConfigurationManager COMMAND test_property_configurationmanager)