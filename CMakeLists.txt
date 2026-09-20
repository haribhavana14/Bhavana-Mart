cmake_minimum_required(VERSION 3.25)

project(BhavanaMart LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)

find_package(unofficial-sqlite3 CONFIG QUIET)

if(unofficial-sqlite3_FOUND)
    set(SQLITE_TARGET unofficial::sqlite3::sqlite3)
else()
    find_package(SQLite3 REQUIRED)
    set(SQLITE_TARGET SQLite::SQLite3)
endif()

add_executable(BhavanaMart
    src/main.cpp
    repositories/ProductRepository.cpp
)

target_link_libraries(BhavanaMart PRIVATE
    Drogon::Drogon
    ${SQLITE_TARGET}
)