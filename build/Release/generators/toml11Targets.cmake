# Load the debug and release variables
file(GLOB DATA_FILES "${CMAKE_CURRENT_LIST_DIR}/toml11-*-data.cmake")

foreach(f ${DATA_FILES})
    include(${f})
endforeach()

# Create the targets for all the components
foreach(_COMPONENT ${toml11_COMPONENT_NAMES} )
    if(NOT TARGET ${_COMPONENT})
        add_library(${_COMPONENT} INTERFACE IMPORTED)
        message(${toml11_MESSAGE_MODE} "Conan: Component target declared '${_COMPONENT}'")
    endif()
endforeach()

if(NOT TARGET toml11::toml11)
    add_library(toml11::toml11 INTERFACE IMPORTED)
    message(${toml11_MESSAGE_MODE} "Conan: Target declared 'toml11::toml11'")
endif()
# Load the debug and release library finders
file(GLOB CONFIG_FILES "${CMAKE_CURRENT_LIST_DIR}/toml11-Target-*.cmake")

foreach(f ${CONFIG_FILES})
    include(${f})
endforeach()