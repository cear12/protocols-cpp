add_library(catch2_main STATIC test_main.cpp)
target_include_directories(catch2_main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/catch2)

add_executable(protocolscpp_tests test_tcp_echo_server.cpp)
target_link_libraries(protocolscpp_tests PRIVATE protocolscpp_core catch2_main)
target_include_directories(protocolscpp_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party/catch2)

include(CTest)
add_test(NAME protocolscpp_tests COMMAND protocolscpp_tests)
