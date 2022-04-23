cmake_minimum_required(VERSION 2.8)

project(AddressTest)


set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++17")

# Boost
find_package( Boost 1.55 COMPONENTS system thread filesystem REQUIRED )
include_directories(SYSTEM ${Boost_INCLUDE_DIR} src )

file (GLOB SRCS src/*.cpp)

add_executable(${PROJECT_NAME}
    ${SRCS}
    main.cpp
)

add_executable(${PROJECT_NAME}_server
    ${SRCS}
    main_server.cpp
)

add_executable(${PROJECT_NAME}_client
    ${SRCS}
    main_client.cpp
)

