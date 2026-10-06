cmake_minimum_required(VERSION 3.25)

project(BhavanaMart LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)

find_library(SODIUM_LIBRARY sodium REQUIRED)

add_executable(BhavanaMart
    src/main.cpp

    repositories/productrepository.cpp

    plugin/DatabasePlugin.cpp

    src/util/PasswordUtil.cpp
)

target_include_directories(BhavanaMart PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(BhavanaMart PRIVATE
    Drogon::Drogon
    ${SODIUM_LIBRARY}
)