add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/steamd-client-glue.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/steamd-server-glue.hpp
    COMMAND sdbus-c++-xml2cpp ${PROJECT_SOURCE_DIR}/dbus/steamd-bindings.xml
        --proxy=steamd-client-glue.hpp --adapter=steamd-server-glue.hpp
    DEPENDS $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/dbus/steamd-bindings.xml>
    COMMENT "Generating D-Bus interfaces for steamd."
)

add_library(steamd-glue INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/steamd-client-glue.hpp>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/steamd-server-glue.hpp>
)

target_include_directories(steamd-glue INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)
