########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND boost_COMPONENT_NAMES Boost::diagnostic_definitions Boost::disable_autolinking Boost::dynamic_linking Boost::headers Boost::boost boost::_libboost Boost::atomic Boost::container Boost::date_time Boost::exception Boost::system Boost::chrono Boost::filesystem Boost::url Boost::thread Boost::contract)
list(REMOVE_DUPLICATES boost_COMPONENT_NAMES)
if(DEFINED boost_FIND_DEPENDENCY_NAMES)
  list(APPEND boost_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES boost_FIND_DEPENDENCY_NAMES)
else()
  set(boost_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(boost_PACKAGE_FOLDER_RELEASE "/Users/alex/.conan2/p/b/boostdf910198ac34d/p")
set(boost_BUILD_MODULES_PATHS_RELEASE )


set(boost_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_RES_DIRS_RELEASE )
set(boost_DEFINITIONS_RELEASE )
set(boost_SHARED_LINK_FLAGS_RELEASE )
set(boost_EXE_LINK_FLAGS_RELEASE )
set(boost_OBJECTS_RELEASE )
set(boost_COMPILE_DEFINITIONS_RELEASE )
set(boost_COMPILE_OPTIONS_C_RELEASE )
set(boost_COMPILE_OPTIONS_CXX_RELEASE )
set(boost_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_BIN_DIRS_RELEASE )
set(boost_LIBRARY_TYPE_RELEASE STATIC)
set(boost_IS_HOST_WINDOWS_RELEASE 0)
set(boost_LIBS_RELEASE boost_contract boost_thread boost_url boost_filesystem boost_chrono boost_exception boost_date_time boost_container boost_atomic)
set(boost_SYSTEM_LIBS_RELEASE )
set(boost_FRAMEWORK_DIRS_RELEASE )
set(boost_FRAMEWORKS_RELEASE )
set(boost_BUILD_DIRS_RELEASE )
set(boost_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(boost_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_COMPILE_OPTIONS_C_RELEASE}>")
set(boost_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_EXE_LINK_FLAGS_RELEASE}>")


set(boost_COMPONENTS_RELEASE Boost::diagnostic_definitions Boost::disable_autolinking Boost::dynamic_linking Boost::headers Boost::boost boost::_libboost Boost::atomic Boost::container Boost::date_time Boost::exception Boost::system Boost::chrono Boost::filesystem Boost::url Boost::thread Boost::contract)
########### COMPONENT Boost::contract VARIABLES ############################################

set(boost_Boost_contract_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_contract_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_contract_BIN_DIRS_RELEASE )
set(boost_Boost_contract_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_contract_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_contract_RES_DIRS_RELEASE )
set(boost_Boost_contract_DEFINITIONS_RELEASE )
set(boost_Boost_contract_OBJECTS_RELEASE )
set(boost_Boost_contract_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_contract_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_contract_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_contract_LIBS_RELEASE boost_contract)
set(boost_Boost_contract_SYSTEM_LIBS_RELEASE )
set(boost_Boost_contract_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_contract_FRAMEWORKS_RELEASE )
set(boost_Boost_contract_DEPENDENCIES_RELEASE Boost::exception Boost::thread boost::_libboost)
set(boost_Boost_contract_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_contract_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_contract_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_contract_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_contract_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_contract_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_contract_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_contract_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_contract_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_contract_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::thread VARIABLES ############################################

set(boost_Boost_thread_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_thread_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_thread_BIN_DIRS_RELEASE )
set(boost_Boost_thread_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_thread_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_thread_RES_DIRS_RELEASE )
set(boost_Boost_thread_DEFINITIONS_RELEASE )
set(boost_Boost_thread_OBJECTS_RELEASE )
set(boost_Boost_thread_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_thread_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_thread_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_thread_LIBS_RELEASE boost_thread)
set(boost_Boost_thread_SYSTEM_LIBS_RELEASE )
set(boost_Boost_thread_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_thread_FRAMEWORKS_RELEASE )
set(boost_Boost_thread_DEPENDENCIES_RELEASE Boost::atomic Boost::chrono Boost::container Boost::date_time Boost::exception Boost::system boost::_libboost)
set(boost_Boost_thread_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_thread_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_thread_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_thread_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_thread_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_thread_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_thread_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_thread_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_thread_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_thread_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::url VARIABLES ############################################

set(boost_Boost_url_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_url_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_url_BIN_DIRS_RELEASE )
set(boost_Boost_url_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_url_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_url_RES_DIRS_RELEASE )
set(boost_Boost_url_DEFINITIONS_RELEASE )
set(boost_Boost_url_OBJECTS_RELEASE )
set(boost_Boost_url_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_url_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_url_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_url_LIBS_RELEASE boost_url)
set(boost_Boost_url_SYSTEM_LIBS_RELEASE )
set(boost_Boost_url_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_url_FRAMEWORKS_RELEASE )
set(boost_Boost_url_DEPENDENCIES_RELEASE Boost::system boost::_libboost)
set(boost_Boost_url_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_url_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_url_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_url_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_url_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_url_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_url_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_url_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_url_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_url_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::filesystem VARIABLES ############################################

set(boost_Boost_filesystem_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_filesystem_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_filesystem_BIN_DIRS_RELEASE )
set(boost_Boost_filesystem_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_filesystem_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_filesystem_RES_DIRS_RELEASE )
set(boost_Boost_filesystem_DEFINITIONS_RELEASE )
set(boost_Boost_filesystem_OBJECTS_RELEASE )
set(boost_Boost_filesystem_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_filesystem_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_filesystem_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_filesystem_LIBS_RELEASE boost_filesystem)
set(boost_Boost_filesystem_SYSTEM_LIBS_RELEASE )
set(boost_Boost_filesystem_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_filesystem_FRAMEWORKS_RELEASE )
set(boost_Boost_filesystem_DEPENDENCIES_RELEASE Boost::atomic Boost::system boost::_libboost)
set(boost_Boost_filesystem_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_filesystem_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_filesystem_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_filesystem_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_filesystem_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_filesystem_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_filesystem_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_filesystem_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_filesystem_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_filesystem_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::chrono VARIABLES ############################################

set(boost_Boost_chrono_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_chrono_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_chrono_BIN_DIRS_RELEASE )
set(boost_Boost_chrono_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_chrono_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_chrono_RES_DIRS_RELEASE )
set(boost_Boost_chrono_DEFINITIONS_RELEASE )
set(boost_Boost_chrono_OBJECTS_RELEASE )
set(boost_Boost_chrono_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_chrono_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_chrono_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_chrono_LIBS_RELEASE boost_chrono)
set(boost_Boost_chrono_SYSTEM_LIBS_RELEASE )
set(boost_Boost_chrono_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_chrono_FRAMEWORKS_RELEASE )
set(boost_Boost_chrono_DEPENDENCIES_RELEASE Boost::system boost::_libboost)
set(boost_Boost_chrono_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_chrono_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_chrono_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_chrono_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_chrono_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_chrono_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_chrono_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_chrono_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_chrono_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_chrono_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::system VARIABLES ############################################

set(boost_Boost_system_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_system_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_system_BIN_DIRS_RELEASE )
set(boost_Boost_system_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_system_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_system_RES_DIRS_RELEASE )
set(boost_Boost_system_DEFINITIONS_RELEASE )
set(boost_Boost_system_OBJECTS_RELEASE )
set(boost_Boost_system_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_system_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_system_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_system_LIBS_RELEASE )
set(boost_Boost_system_SYSTEM_LIBS_RELEASE )
set(boost_Boost_system_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_system_FRAMEWORKS_RELEASE )
set(boost_Boost_system_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_system_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_system_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_system_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_system_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_system_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_system_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_system_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_system_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_system_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_system_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::exception VARIABLES ############################################

set(boost_Boost_exception_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_exception_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_exception_BIN_DIRS_RELEASE )
set(boost_Boost_exception_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_exception_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_exception_RES_DIRS_RELEASE )
set(boost_Boost_exception_DEFINITIONS_RELEASE )
set(boost_Boost_exception_OBJECTS_RELEASE )
set(boost_Boost_exception_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_exception_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_exception_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_exception_LIBS_RELEASE boost_exception)
set(boost_Boost_exception_SYSTEM_LIBS_RELEASE )
set(boost_Boost_exception_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_exception_FRAMEWORKS_RELEASE )
set(boost_Boost_exception_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_exception_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_exception_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_exception_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_exception_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_exception_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_exception_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_exception_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_exception_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_exception_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_exception_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::date_time VARIABLES ############################################

set(boost_Boost_date_time_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_date_time_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_date_time_BIN_DIRS_RELEASE )
set(boost_Boost_date_time_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_date_time_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_date_time_RES_DIRS_RELEASE )
set(boost_Boost_date_time_DEFINITIONS_RELEASE )
set(boost_Boost_date_time_OBJECTS_RELEASE )
set(boost_Boost_date_time_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_date_time_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_date_time_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_date_time_LIBS_RELEASE boost_date_time)
set(boost_Boost_date_time_SYSTEM_LIBS_RELEASE )
set(boost_Boost_date_time_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_date_time_FRAMEWORKS_RELEASE )
set(boost_Boost_date_time_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_date_time_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_date_time_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_date_time_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_date_time_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_date_time_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_date_time_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_date_time_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_date_time_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_date_time_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_date_time_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::container VARIABLES ############################################

set(boost_Boost_container_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_container_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_container_BIN_DIRS_RELEASE )
set(boost_Boost_container_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_container_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_container_RES_DIRS_RELEASE )
set(boost_Boost_container_DEFINITIONS_RELEASE )
set(boost_Boost_container_OBJECTS_RELEASE )
set(boost_Boost_container_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_container_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_container_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_container_LIBS_RELEASE boost_container)
set(boost_Boost_container_SYSTEM_LIBS_RELEASE )
set(boost_Boost_container_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_container_FRAMEWORKS_RELEASE )
set(boost_Boost_container_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_container_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_container_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_container_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_container_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_container_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_container_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_container_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_container_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_container_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_container_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::atomic VARIABLES ############################################

set(boost_Boost_atomic_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_atomic_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_atomic_BIN_DIRS_RELEASE )
set(boost_Boost_atomic_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_atomic_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_atomic_RES_DIRS_RELEASE )
set(boost_Boost_atomic_DEFINITIONS_RELEASE )
set(boost_Boost_atomic_OBJECTS_RELEASE )
set(boost_Boost_atomic_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_atomic_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_atomic_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_atomic_LIBS_RELEASE boost_atomic)
set(boost_Boost_atomic_SYSTEM_LIBS_RELEASE )
set(boost_Boost_atomic_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_atomic_FRAMEWORKS_RELEASE )
set(boost_Boost_atomic_DEPENDENCIES_RELEASE boost::_libboost)
set(boost_Boost_atomic_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_atomic_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_atomic_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_atomic_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_atomic_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_atomic_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_atomic_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_atomic_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_atomic_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_atomic_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT boost::_libboost VARIABLES ############################################

set(boost_boost__libboost_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_boost__libboost_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_boost__libboost_BIN_DIRS_RELEASE )
set(boost_boost__libboost_LIBRARY_TYPE_RELEASE STATIC)
set(boost_boost__libboost_IS_HOST_WINDOWS_RELEASE 0)
set(boost_boost__libboost_RES_DIRS_RELEASE )
set(boost_boost__libboost_DEFINITIONS_RELEASE )
set(boost_boost__libboost_OBJECTS_RELEASE )
set(boost_boost__libboost_COMPILE_DEFINITIONS_RELEASE )
set(boost_boost__libboost_COMPILE_OPTIONS_C_RELEASE "")
set(boost_boost__libboost_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_boost__libboost_LIBS_RELEASE )
set(boost_boost__libboost_SYSTEM_LIBS_RELEASE )
set(boost_boost__libboost_FRAMEWORK_DIRS_RELEASE )
set(boost_boost__libboost_FRAMEWORKS_RELEASE )
set(boost_boost__libboost_DEPENDENCIES_RELEASE Boost::headers)
set(boost_boost__libboost_SHARED_LINK_FLAGS_RELEASE )
set(boost_boost__libboost_EXE_LINK_FLAGS_RELEASE )
set(boost_boost__libboost_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_boost__libboost_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_boost__libboost_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_boost__libboost_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_boost__libboost_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_boost__libboost_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_boost__libboost_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_boost__libboost_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::boost VARIABLES ############################################

set(boost_Boost_boost_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_boost_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_boost_BIN_DIRS_RELEASE )
set(boost_Boost_boost_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_boost_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_boost_RES_DIRS_RELEASE )
set(boost_Boost_boost_DEFINITIONS_RELEASE )
set(boost_Boost_boost_OBJECTS_RELEASE )
set(boost_Boost_boost_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_boost_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_boost_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_boost_LIBS_RELEASE )
set(boost_Boost_boost_SYSTEM_LIBS_RELEASE )
set(boost_Boost_boost_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_boost_FRAMEWORKS_RELEASE )
set(boost_Boost_boost_DEPENDENCIES_RELEASE Boost::headers)
set(boost_Boost_boost_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_boost_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_boost_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_boost_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_boost_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_boost_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_boost_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_boost_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_boost_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_boost_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::headers VARIABLES ############################################

set(boost_Boost_headers_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_headers_LIB_DIRS_RELEASE )
set(boost_Boost_headers_BIN_DIRS_RELEASE )
set(boost_Boost_headers_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_headers_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_headers_RES_DIRS_RELEASE )
set(boost_Boost_headers_DEFINITIONS_RELEASE )
set(boost_Boost_headers_OBJECTS_RELEASE )
set(boost_Boost_headers_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_headers_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_headers_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_headers_LIBS_RELEASE )
set(boost_Boost_headers_SYSTEM_LIBS_RELEASE )
set(boost_Boost_headers_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_headers_FRAMEWORKS_RELEASE )
set(boost_Boost_headers_DEPENDENCIES_RELEASE Boost::diagnostic_definitions Boost::disable_autolinking Boost::dynamic_linking)
set(boost_Boost_headers_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_headers_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_headers_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_headers_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_headers_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_headers_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_headers_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_headers_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_headers_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_headers_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::dynamic_linking VARIABLES ############################################

set(boost_Boost_dynamic_linking_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_dynamic_linking_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_dynamic_linking_BIN_DIRS_RELEASE )
set(boost_Boost_dynamic_linking_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_dynamic_linking_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_dynamic_linking_RES_DIRS_RELEASE )
set(boost_Boost_dynamic_linking_DEFINITIONS_RELEASE )
set(boost_Boost_dynamic_linking_OBJECTS_RELEASE )
set(boost_Boost_dynamic_linking_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_dynamic_linking_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_dynamic_linking_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_dynamic_linking_LIBS_RELEASE )
set(boost_Boost_dynamic_linking_SYSTEM_LIBS_RELEASE )
set(boost_Boost_dynamic_linking_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_dynamic_linking_FRAMEWORKS_RELEASE )
set(boost_Boost_dynamic_linking_DEPENDENCIES_RELEASE )
set(boost_Boost_dynamic_linking_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_dynamic_linking_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_dynamic_linking_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_dynamic_linking_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_dynamic_linking_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_dynamic_linking_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_dynamic_linking_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_dynamic_linking_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_dynamic_linking_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_dynamic_linking_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::disable_autolinking VARIABLES ############################################

set(boost_Boost_disable_autolinking_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_disable_autolinking_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_disable_autolinking_BIN_DIRS_RELEASE )
set(boost_Boost_disable_autolinking_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_disable_autolinking_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_disable_autolinking_RES_DIRS_RELEASE )
set(boost_Boost_disable_autolinking_DEFINITIONS_RELEASE )
set(boost_Boost_disable_autolinking_OBJECTS_RELEASE )
set(boost_Boost_disable_autolinking_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_disable_autolinking_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_disable_autolinking_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_disable_autolinking_LIBS_RELEASE )
set(boost_Boost_disable_autolinking_SYSTEM_LIBS_RELEASE )
set(boost_Boost_disable_autolinking_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_disable_autolinking_FRAMEWORKS_RELEASE )
set(boost_Boost_disable_autolinking_DEPENDENCIES_RELEASE )
set(boost_Boost_disable_autolinking_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_disable_autolinking_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_disable_autolinking_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_disable_autolinking_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_disable_autolinking_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_disable_autolinking_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_disable_autolinking_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_disable_autolinking_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_disable_autolinking_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_disable_autolinking_COMPILE_OPTIONS_C_RELEASE}>")
########### COMPONENT Boost::diagnostic_definitions VARIABLES ############################################

set(boost_Boost_diagnostic_definitions_INCLUDE_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/include")
set(boost_Boost_diagnostic_definitions_LIB_DIRS_RELEASE "${boost_PACKAGE_FOLDER_RELEASE}/lib")
set(boost_Boost_diagnostic_definitions_BIN_DIRS_RELEASE )
set(boost_Boost_diagnostic_definitions_LIBRARY_TYPE_RELEASE STATIC)
set(boost_Boost_diagnostic_definitions_IS_HOST_WINDOWS_RELEASE 0)
set(boost_Boost_diagnostic_definitions_RES_DIRS_RELEASE )
set(boost_Boost_diagnostic_definitions_DEFINITIONS_RELEASE )
set(boost_Boost_diagnostic_definitions_OBJECTS_RELEASE )
set(boost_Boost_diagnostic_definitions_COMPILE_DEFINITIONS_RELEASE )
set(boost_Boost_diagnostic_definitions_COMPILE_OPTIONS_C_RELEASE "")
set(boost_Boost_diagnostic_definitions_COMPILE_OPTIONS_CXX_RELEASE "")
set(boost_Boost_diagnostic_definitions_LIBS_RELEASE )
set(boost_Boost_diagnostic_definitions_SYSTEM_LIBS_RELEASE )
set(boost_Boost_diagnostic_definitions_FRAMEWORK_DIRS_RELEASE )
set(boost_Boost_diagnostic_definitions_FRAMEWORKS_RELEASE )
set(boost_Boost_diagnostic_definitions_DEPENDENCIES_RELEASE )
set(boost_Boost_diagnostic_definitions_SHARED_LINK_FLAGS_RELEASE )
set(boost_Boost_diagnostic_definitions_EXE_LINK_FLAGS_RELEASE )
set(boost_Boost_diagnostic_definitions_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(boost_Boost_diagnostic_definitions_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${boost_Boost_diagnostic_definitions_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${boost_Boost_diagnostic_definitions_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${boost_Boost_diagnostic_definitions_EXE_LINK_FLAGS_RELEASE}>
)
set(boost_Boost_diagnostic_definitions_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${boost_Boost_diagnostic_definitions_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${boost_Boost_diagnostic_definitions_COMPILE_OPTIONS_C_RELEASE}>")