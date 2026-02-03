# Test utilities

# Create test utilities library
add_library(test_utils STATIC
    test_helpers.cpp
)

target_link_libraries(test_utils
    Qt6::Core
    Qt6::Test
)

target_include_directories(test_utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)