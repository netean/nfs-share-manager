# Testing configuration for NFS Share Manager

# Find Qt Test module
find_package(Qt6 REQUIRED COMPONENTS Test)

# Enable testing
enable_testing()

# Include source directories for testing
include_directories(${CMAKE_SOURCE_DIR}/src)

# Core data model tests
add_subdirectory(core)

# Business logic tests
add_subdirectory(business)

# UI tests
add_subdirectory(ui)

# System integration tests
add_subdirectory(system)

# Property-based testing infrastructure
add_subdirectory(property)

# Test utilities
add_subdirectory(utils)

# Integration tests
add_subdirectory(integration)

# Create a test runner that runs all tests
add_custom_target(run_all_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS
        test_types
        test_permissionset
        test_shareconfiguration
        test_nfsshare
        test_remotenfsshare
        test_nfsmount
        test_configurationmanager
        test_errorhandling
        test_sharemanager
        test_mountmanager
        test_networkdiscovery
        test_policykithelper
        test_end_to_end_workflows
    COMMENT "Running all unit and integration tests"
)

# Create separate test runners for different test types
add_custom_target(run_unit_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L unit --verbose
    DEPENDS
        test_types
        test_permissionset
        test_shareconfiguration
        test_nfsshare
        test_remotenfsshare
        test_nfsmount
        test_configurationmanager
        test_errorhandling
        test_sharemanager
        test_mountmanager
        test_networkdiscovery
        test_policykithelper
    COMMENT "Running unit tests only"
)

add_custom_target(run_integration_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L integration --verbose
    DEPENDS test_end_to_end_workflows
    COMMENT "Running integration tests only"
)