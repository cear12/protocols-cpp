cmake_minimum_required(VERSION 3.16)
project(protocolscpp CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(PROTOCOLSCPP_BUILD_TESTS "Build the Catch2 unit test suite" ON)
option(PROTOCOLSCPP_BUILD_EXAMPLES "Build examples/" ON)
option(PROTOCOLSCPP_ENABLE_GRPC "Build grpc_client_demo.cpp (needs protoc + gRPC)" OFF)
option(PROTOCOLSCPP_ENABLE_WEBRTC "Build web_rtc_data_channel.cpp (fetches libdatachannel)" OFF)

find_package(Threads REQUIRED)

# --- Zero-dependency core: builds and is unit-tested in every environment ---
add_library(protocolscpp_core STATIC src/tcp_echo_server.cpp)
target_include_directories(protocolscpp_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(protocolscpp_core PUBLIC Threads::Threads)
target_compile_options(protocolscpp_core PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>)

# --- Vendored, MIT-licensed easywsclient (see third_party/easywsclient/COPYING) ---
add_library(easywsclient STATIC third_party/easywsclient/easywsclient.cpp)
target_include_directories(easywsclient PUBLIC third_party/easywsclient)

add_executable(web_socket_client web_socket_client.cpp)
target_compile_definitions(web_socket_client PRIVATE PROTOCOLSCPP_BUILD_STANDALONE_DEMO)
target_link_libraries(web_socket_client PRIVATE easywsclient)

# --- Everything below needs a system library this repo does not vendor. ---
# Each target is only added if CMake actually finds the dependency, so
# `cmake configure` never hard-fails on a machine that's missing one of
# them -- see README.md for the apt packages CI installs to build all of
# them for real.

find_package(CURL QUIET)
if(CURL_FOUND)
    add_executable(ftp_client ftp_client.cpp)
    target_compile_definitions(ftp_client PRIVATE PROTOCOLSCPP_BUILD_STANDALONE_DEMO)
    target_link_libraries(ftp_client PRIVATE CURL::libcurl)

    add_executable(smtp_email_client smtp_email_client.cpp)
    target_compile_definitions(smtp_email_client PRIVATE PROTOCOLSCPP_BUILD_STANDALONE_DEMO)
    target_link_libraries(smtp_email_client PRIVATE CURL::libcurl)
else()
    message(STATUS "protocols-cpp: CURL not found, skipping ftp_client / smtp_email_client")
endif()

find_package(jsoncpp QUIET)
if(jsoncpp_FOUND)
    add_executable(native_messaging_host native_messaging_host.cpp)
    target_link_libraries(native_messaging_host PRIVATE jsoncpp_lib)
else()
    message(STATUS "protocols-cpp: jsoncpp not found, skipping native_messaging_host")
endif()

find_package(websocketpp QUIET)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()
find_package(Boost QUIET)
if(websocketpp_FOUND AND Boost_FOUND)
    add_executable(web_socket_server web_socket_server.cpp)
    # Ubuntu's packaged websocketpp still uses syntax rejected by C++20.
    # Keep the rest of the project on C++20, but build this legacy adapter
    # with the standard supported by the installed websocketpp headers.
    set_target_properties(web_socket_server PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    target_compile_definitions(web_socket_server PRIVATE
        PROTOCOLSCPP_BUILD_STANDALONE_DEMO
        _WEBSOCKETPP_CPP11_STL_
    )
    target_include_directories(web_socket_server PRIVATE ${WEBSOCKETPP_INCLUDE_DIR})
    target_link_libraries(web_socket_server PRIVATE Threads::Threads)
    if(TARGET Boost::system)
        target_link_libraries(web_socket_server PRIVATE Boost::system)
    elseif(TARGET Boost::headers)
        # Boost.System is header-only in recent Boost releases and no longer
        # always exports a separate Boost::system target.
        target_link_libraries(web_socket_server PRIVATE Boost::headers)
        target_compile_definitions(web_socket_server PRIVATE
            BOOST_ERROR_CODE_HEADER_ONLY
        )
    else()
        target_compile_definitions(web_socket_server PRIVATE
            BOOST_ERROR_CODE_HEADER_ONLY
        )
    endif()
else()
    message(STATUS "protocols-cpp: websocketpp/Boost not found, skipping web_socket_server")
endif()

if(PROTOCOLSCPP_ENABLE_GRPC)
    find_package(Protobuf REQUIRED)
    find_package(gRPC REQUIRED)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS proto/simple_math.proto)
    grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${CMAKE_CURRENT_BINARY_DIR} proto/simple_math.proto)

    add_executable(grpc_client_demo grpc_client_demo.cpp ${PROTO_SRCS} ${GRPC_SRCS})
    target_include_directories(grpc_client_demo PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(grpc_client_demo PRIVATE gRPC::grpc++ protobuf::libprotobuf)
endif()

if(PROTOCOLSCPP_ENABLE_WEBRTC)
    include(FetchContent)
    FetchContent_Declare(libdatachannel GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel.git
                          GIT_TAG v0.20.1 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(libdatachannel)

    add_executable(web_rtc_data_channel web_rtc_data_channel.cpp)
    target_compile_definitions(web_rtc_data_channel PRIVATE PROTOCOLSCPP_BUILD_STANDALONE_DEMO)
    target_link_libraries(web_rtc_data_channel PRIVATE LibDataChannel::LibDataChannel)
endif()

if(PROTOCOLSCPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(PROTOCOLSCPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
