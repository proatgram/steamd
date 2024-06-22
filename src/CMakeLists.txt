add_compile_definitions("STEAMWORKS_CLIENT_INTERFACES")

add_executable(steamd)

target_sources(steamd
    PRIVATE
        "main.cpp"
)

target_link_libraries(steamd
    PRIVATE
        Steam
)

add_library(Steam)

target_sources(Steam
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Steam.cpp>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/Steam.hpp>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/Imported/SteamIPAddress.hpp>
)

target_include_directories(Steam
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

target_link_libraries(Steam
    PUBLIC
        OpenSteamAPI
        sdbus-c++
        steamd-glue
)
