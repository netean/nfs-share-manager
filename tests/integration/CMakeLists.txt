# Integration Tests CMakeLists.txt

# Create integration test directory if it doesn't exist
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# Find required packages for integration tests
find_package(Qt6 REQUIRED COMPONENTS Test Core Widgets Network DBus)
find_package(KF6Config REQUIRED)
find_package(KF6I18n REQUIRED)
# Note: KF6Notifications might not be available, so we'll make it optional
find_package(KF6Notifications QUIET)

# Set up test environment variables
set(TEST_ENVIRONMENT
    "QT_QPA_PLATFORM=offscreen"
    "SKIP_POLICYKIT_TESTS=1"  # Skip PolicyKit tests by default in CI
    "SKIP_NETWORK_TESTS=0"    # Enable network tests
    "TEST_TIMEOUT=30000"      # 30 second timeout
)

# End-to-end workflow integration test
add_executable(test_end_to_end_workflows
    test_end_to_end_workflows.cpp
)

# Enable Qt MOC for this target
set_target_properties(test_end_to_end_workflows PROPERTIES
    AUTOMOC ON
)

target_link_libraries(test_end_to_end_workflows
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::DBus
    KF6::ConfigCore
    KF6::I18n
)

target_include_directories(test_end_to_end_workflows PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# Simplified end-to-end test (can run without full application)
add_executable(test_end_to_end_simple
    test_end_to_end_simple.cpp
)

# Enable Qt MOC for this target
set_target_properties(test_end_to_end_simple PROPERTIES
    AUTOMOC ON
)

target_link_libraries(test_end_to_end_simple
    Qt6::Test
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# Add the tests to CTest
add_test(NAME EndToEndWorkflows COMMAND test_end_to_end_workflows)
set_tests_properties(EndToEndWorkflows PROPERTIES
    ENVIRONMENT "${TEST_ENVIRONMENT}"
    TIMEOUT 120  # 2 minute timeout for complete workflow tests
    LABELS "integration;end-to-end;workflow"
)

add_test(NAME EndToEndSimple COMMAND test_end_to_end_simple)
set_tests_properties(EndToEndSimple PROPERTIES
    ENVIRONMENT "${TEST_ENVIRONMENT}"
    TIMEOUT 60   # 1 minute timeout for simplified tests
    LABELS "integration;end-to-end;simple"
)

# Property-based integration test (if we want to add one)
# add_executable(test_integration_properties
#     test_integration_properties.cpp
# )
# 
# target_link_libraries(test_integration_properties
#     Qt6::Test
#     Qt6::Core
#     nfs-share-manager-core
#     nfs-share-manager-business
# )
# 
# add_test(NAME IntegrationProperties COMMAND test_integration_properties)
# set_tests_properties(IntegrationProperties PROPERTIES
#     ENVIRONMENT "${TEST_ENVIRONMENT}"
#     TIMEOUT 300  # 5 minute timeout for property tests
#     LABELS "integration;property-based"
# )

# Test data files (if needed)
# configure_file(test_data/sample_config.json ${CMAKE_CURRENT_BINARY_DIR}/test_data/sample_config.json COPYONLY)

# Custom test target for integration tests only
add_custom_target(integration-tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L integration --output-on-failure
    DEPENDS test_end_to_end_workflows test_end_to_end_simple
    COMMENT "Running integration tests"
)

# Custom test target for end-to-end tests specifically
add_custom_target(e2e-tests
    COMMAND ${CMAKE_CTEST_COMMAND} -L end-to-end --output-on-failure
    DEPENDS test_end_to_end_workflows test_end_to_end_simple
    COMMENT "Running end-to-end workflow tests"
)

# Custom test target for simplified tests (safe for any environment)
add_custom_target(e2e-tests-simple
    COMMAND ${CMAKE_CTEST_COMMAND} -L simple --output-on-failure
    DEPENDS test_end_to_end_simple
    COMMENT "Running simplified end-to-end tests (safe for CI)"
)

# Test with PolicyKit enabled (requires user interaction)
add_custom_target(e2e-tests-with-policykit
    COMMAND ${CMAKE_COMMAND} -E env SKIP_POLICYKIT_TESTS=0 ${CMAKE_CTEST_COMMAND} -L end-to-end --output-on-failure
    DEPENDS test_end_to_end_workflows test_end_to_end_simple
    COMMENT "Running end-to-end tests with PolicyKit integration (requires user interaction)"
)

# Test with network operations enabled
add_custom_target(e2e-tests-with-network
    COMMAND ${CMAKE_COMMAND} -E env SKIP_NETWORK_TESTS=0 ${CMAKE_CTEST_COMMAND} -L end-to-end --output-on-failure
    DEPENDS test_end_to_end_workflows test_end_to_end_simple
    COMMENT "Running end-to-end tests with network operations"
)

# Full integration test (all features enabled)
add_custom_target(e2e-tests-full
    COMMAND ${CMAKE_COMMAND} -E env SKIP_POLICYKIT_TESTS=0 SKIP_NETWORK_TESTS=0 ${CMAKE_CTEST_COMMAND} -L end-to-end --output-on-failure
    DEPENDS test_end_to_end_workflows test_end_to_end_simple
    COMMENT "Running full end-to-end tests (requires user interaction and network)"
)

message(STATUS "Integration tests configured:")
message(STATUS "  - End-to-end workflow tests")
message(STATUS "  - PolicyKit integration tests (optional)")
message(STATUS "  - Network discovery tests (optional)")
message(STATUS "  - Component failure recovery tests")
message(STATUS "")
message(STATUS "Available test targets:")
message(STATUS "  make integration-tests     - Run all integration tests")
message(STATUS "  make e2e-tests            - Run end-to-end tests (safe for CI)")
message(STATUS "  make e2e-tests-simple     - Run simplified tests (safest for CI)")
message(STATUS "  make e2e-tests-with-policykit - Run with PolicyKit (requires interaction)")
message(STATUS "  make e2e-tests-with-network   - Run with network operations")
message(STATUS "  make e2e-tests-full       - Run all features (interactive)")