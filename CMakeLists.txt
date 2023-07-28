cmake_minimum_required(VERSION 3.20)
project(tempoclient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set project wide warning levels
if (WIN32 OR MSVC)
    add_compile_options(/W1)
    # If VCPKG_ROOT is set include the vcpkg.cmake here
    IF(DEFINED ENV{VCPKG_ROOT})
        include($ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
    ENDIF()
else()
    add_compile_options(-Wall -Wextra -pedantic -Wno-sign-compare)
endif()

# set to 1 to enable HTTPS host strings
# static link of OPENSSL
# OpenSSL is not in 3rdParty install separately and set Environment variable OPENSSL_INCLUDE_DIR
set (USE_OPEN_SSL 0)
if(${USE_OPEN_SSL})
    find_package(OpenSSL REQUIRED)
    add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
    if(DEFINED $ENV{OPENSSL_ROOT_DIR})
        set(OPENSSL_LICENSE_DIR $ENV{OPENSSL_ROOT_DIR})
    else()
        set(OPENSSL_LICENSE_DIR ${OPENSSL_INCLUDE_DIR}/..)
    endif()
endif()

add_subdirectory(3rdParty/CLI11)
add_subdirectory(3rdParty/nlohmann/json)


function(make_includable input_file output_file)
    file(READ ${input_file} content)
    set(delim "licenseText")
    set(content "R\"${delim}(\n${content})${delim}\"")
    file(WRITE ${output_file} "${content}")
endfunction(make_includable)

make_includable("${CMAKE_SOURCE_DIR}/LICENSE.MIT" "${CMAKE_CURRENT_BINARY_DIR}/bioradLicense.h")
make_includable("${CMAKE_SOURCE_DIR}/3rdParty/cpp-httplib/LICENSE" "${CMAKE_CURRENT_BINARY_DIR}/httplibLicense.h")
make_includable("${CMAKE_SOURCE_DIR}/3rdParty/CLI11/LICENSE" "${CMAKE_CURRENT_BINARY_DIR}/cli11License.h")
make_includable("${CMAKE_SOURCE_DIR}/3rdParty/nlohmann/json/LICENSE.MIT" ${CMAKE_CURRENT_BINARY_DIR}/jsonLicense.h)
if(${USE_OPEN_SSL})
    if(EXISTS "${OPENSSL_LICENSE_DIR}/license.txt")
        make_includable("${OPENSSL_LICENSE_DIR}/license.txt" "${CMAKE_CURRENT_BINARY_DIR}/opensslLicense.h")
    else()
        set(LicenseContent "OPEN_SSL license see: https://www.openssl.org/source/license.html\n")
        set(content "R\"licenseText(\n${LicenseContent})licenseText\"")
        file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/opensslLicense.h"  "${content}")
    endif()
endif()


include_directories(./include
                    ./3rdParty/cpp-httplib/
                    ./3rdParty/CLI11/include/
                    ./3rdParty/nlohmann/json/include/
                    ${OPENSSL_INCLUDE_DIR}
                    ${CMAKE_CURRENT_BINARY_DIR})

add_executable( tempoclient
    include/Config.hpp
    include/TempoClient.hpp
    include/Router.hpp
    include/Settings.hpp
    include/Monitor.hpp
    main.cpp)

if(${USE_OPEN_SSL})
    target_link_libraries(tempoclient OpenSSL::Crypto OpenSSL::SSL)
endif()
