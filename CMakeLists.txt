cmake_minimum_required(VERSION 3.25)

project(BhavanaMart LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fix JsonCpp discovery on Linux/Docker
if(UNIX)
    set(JSONCPP_INCLUDE_DIRS "/usr/include/jsoncpp")
    set(JSONCPP_LIBRARIES "/usr/lib/x86_64-linux-gnu/libjsoncpp.so")
endif()

find_package(Drogon CONFIG REQUIRED)

# SQLite
# On Linux, Drogon already provides SQLite3_lib.
# On Windows, use the vcpkg SQLite target.
if(WIN32)
    find_package(unofficial-sqlite3 CONFIG REQUIRED)
    set(SQLITE_TARGET unofficial::sqlite3::sqlite3)
elseif(TARGET SQLite3_lib)
    set(SQLITE_TARGET SQLite3_lib)
else()
    find_package(SQLite3 REQUIRED)
    set(SQLITE_TARGET SQLite::SQLite3)
endif()

add_executable(BhavanaMart
    src/main.cpp
    repositories/productrepository.cpp
)

target_link_libraries(BhavanaMart PRIVATE
    Drogon::Drogon
    ${SQLITE_TARGET}
)