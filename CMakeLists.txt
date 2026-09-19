cmake_minimum_required(VERSION 3.25)

project(BhavanaMart LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_executable(BhavanaMart
    src/main.cpp
    repositories/ProductRepository.cpp
)

target_link_libraries(BhavanaMart PRIVATE
    Drogon::Drogon
    unofficial::sqlite3::sqlite3
)
