cmake_minimum_required(VERSION 3.16)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# We MUST compile and link with 32 bit libraries and binaries
# regardless if the platform is 
set(CMAKE_C_FLAGS "-m32")
set(CMAKE_CXX_FLAGS "-m32")

project(SteamD
    VERSION 0.1
    DESCRIPTION "A semi-open-source Steamclient client written with QML, Qt, and C++"
    HOMEPAGE_URL "https://github.com/proatgram/steamd"
    LANGUAGES CXX
)

# For 32bit compilation and linking with OpenSteamAPI and OpenSteamworks
add_compile_options("-m32")
add_link_options("-m32")
set(FIND_LIBRARY_USE_LIB64_PATHS False CACHE BOOL "Disables 64bit libraries from being found with find_package" FORCE)
set(FIND_LIBRARY_USE_LIB32_PATHS True CACHE BOOL "Enables 32bit libraries being found with find_packahe" FORCE)

fetchcontent_declare(OpenSteamworks
    GIT_REPOSITORY "https://github.com/m4dEngi/open-steamworks.git"
        GIT_PROGRESS True
)

fetchcontent_declare(SDBus-CPP
    GIT_REPOSITORY "https://github.com/Kistler-Group/sdbus-cpp.git"
        GIT_TAG "b7d85f936d622299ee57966c2b2aefde5e4f3684" # Version 2.0.0
        GIT_PROGRESS True
    OVERRIDE_FIND_PACKAGE
    SYSTEM
)

if(NOT opensteamworks_POPULATED)
    fetchcontent_populate(OpenSteamworks)

    add_library(OpenSteamworks INTERFACE)
    target_include_directories(OpenSteamworks INTERFACE "${opensteamworks_SOURCE_DIR}/OpenSteamworks")

    set_property(TARGET OpenSteamworks PROPERTY INCLUDE_DIRECTORIES "${opensteamworks_SOURCE_DIR}/OpenSteamworks")
    set_property(TARGET OpenSteamworks PROPERTY INTERFACE_COMPILE_OPTIONS "-I${opensteamworks_SOURCE_DIR}/OpenSteamworks")

    set(OpenSteamAPI_SOURCES
        ${opensteamworks_SOURCE_DIR}/OpenSteamAPI/src/CCallbackMgr.cpp
        ${opensteamworks_SOURCE_DIR}/OpenSteamAPI/src/ClientWrap.cpp
        ${opensteamworks_SOURCE_DIR}/OpenSteamAPI/src/Interface_OSW.cpp)

    if(APPLE)
        set(OpenSteamAPI_SOURCES ${OpenSteamAPI_SOURCES} "${opensteamworks_SOURCE_DIR}/OpenSteamAPI/src/OSXPathHelper")
    endif()

    add_library(OpenSteamAPI STATIC ${OpenSteamAPI_SOURCES})

    target_link_libraries(OpenSteamAPI PUBLIC OpenSteamworks)
endif()

set(SDBUSCPP_BUILD_CODEGEN True)
set(SDBUSCPP_BUILD_DOCS False)
set(SDBUSCPP_BUILD_DOXYGEN_DOCS False)

# False for now
set(SDBUSCPP_BUILD_TESTS False)

# Apple and Windows don't have libsystemd or sd-bus implementation
if(WIN32 OR APPLE)
    set(SDBUSCPP_BUILD_LIBSYSTEMD True)
endif()

# Only for SDBus-C++
set(GLOBAL_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS False)

set(SDBUSCPP_BUILD_EXAMPLES False)

fetchcontent_makeavailable(SDBus-CPP)

#[[
add_custom_command(
    OUTPUT steamd-client-glue.h steamd-server-glue.h
    COMMAND sdbus-c++-xml2cpp ${PROJECT_SOURCE_DIR}/dbus/steamd-bindings.xml
        --proxy=steamd-client-glue.h --adapter=steamd-server.h
    DEPENDS ${CMAKE_SOURCE_DIR}/dbus/steamd-bindings.xml
    COMMENT "Generating D-Bus interfaces for steamd."
)
]]

# Revert back to original BUILD_SHARED_LIBS
set(BUILD_SHARED_LIBS ${GLOBAL_BUILD_SHARED_LIBS})
unset(GLOBAL_BUILD_SHARED_LIBS)

add_subdirectory(dbus)
add_subdirectory(src)
