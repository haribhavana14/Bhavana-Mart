cmake_minimum_required(VERSION 3.25)

project(BhavanaMart LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)

find_package(unofficial-sodium CONFIG QUIET)

if(TARGET unofficial-sodium::sodium)
    set(SODIUM_TARGET unofficial-sodium::sodium)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)
    set(SODIUM_TARGET PkgConfig::SODIUM)
endif()

add_executable(BhavanaMart
    src/main.cpp
    repositories/productrepository.cpp
    plugin/DatabasePlugin.cpp

    src/util/passwordUtil.cpp
    src/model/User.h

    src/repository/AuthRepository.cpp
    src/service/AuthService.cpp
    src/service/MigrationService.cpp

    src/controller/AuthController.cpp
    src/filter/AuthFilter.cpp
)

target_include_directories(BhavanaMart PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(BhavanaMart PRIVATE
    Drogon::Drogon
    ${SODIUM_TARGET}
)
