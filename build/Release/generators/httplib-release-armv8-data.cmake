########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(cpp-httplib_COMPONENT_NAMES "")
if(DEFINED cpp-httplib_FIND_DEPENDENCY_NAMES)
  list(APPEND cpp-httplib_FIND_DEPENDENCY_NAMES brotli OpenSSL ZLIB)
  list(REMOVE_DUPLICATES cpp-httplib_FIND_DEPENDENCY_NAMES)
else()
  set(cpp-httplib_FIND_DEPENDENCY_NAMES brotli OpenSSL ZLIB)
endif()
set(brotli_FIND_MODE "NO_MODULE")
set(OpenSSL_FIND_MODE "NO_MODULE")
set(ZLIB_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(cpp-httplib_PACKAGE_FOLDER_RELEASE "/Users/alex/.conan2/p/cpp-hbe59c2a74ca37/p")
set(cpp-httplib_BUILD_MODULES_PATHS_RELEASE )


set(cpp-httplib_INCLUDE_DIRS_RELEASE "${cpp-httplib_PACKAGE_FOLDER_RELEASE}/include"
			"${cpp-httplib_PACKAGE_FOLDER_RELEASE}/include/httplib")
set(cpp-httplib_RES_DIRS_RELEASE )
set(cpp-httplib_DEFINITIONS_RELEASE "-DCPPHTTPLIB_OPENSSL_SUPPORT"
			"-DCPPHTTPLIB_ZLIB_SUPPORT"
			"-DCPPHTTPLIB_BROTLI_SUPPORT")
set(cpp-httplib_SHARED_LINK_FLAGS_RELEASE )
set(cpp-httplib_EXE_LINK_FLAGS_RELEASE )
set(cpp-httplib_OBJECTS_RELEASE )
set(cpp-httplib_COMPILE_DEFINITIONS_RELEASE "CPPHTTPLIB_OPENSSL_SUPPORT"
			"CPPHTTPLIB_ZLIB_SUPPORT"
			"CPPHTTPLIB_BROTLI_SUPPORT")
set(cpp-httplib_COMPILE_OPTIONS_C_RELEASE )
set(cpp-httplib_COMPILE_OPTIONS_CXX_RELEASE )
set(cpp-httplib_LIB_DIRS_RELEASE )
set(cpp-httplib_BIN_DIRS_RELEASE )
set(cpp-httplib_LIBRARY_TYPE_RELEASE UNKNOWN)
set(cpp-httplib_IS_HOST_WINDOWS_RELEASE 0)
set(cpp-httplib_LIBS_RELEASE )
set(cpp-httplib_SYSTEM_LIBS_RELEASE )
set(cpp-httplib_FRAMEWORK_DIRS_RELEASE )
set(cpp-httplib_FRAMEWORKS_RELEASE )
set(cpp-httplib_BUILD_DIRS_RELEASE )
set(cpp-httplib_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(cpp-httplib_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${cpp-httplib_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${cpp-httplib_COMPILE_OPTIONS_C_RELEASE}>")
set(cpp-httplib_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${cpp-httplib_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${cpp-httplib_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${cpp-httplib_EXE_LINK_FLAGS_RELEASE}>")


set(cpp-httplib_COMPONENTS_RELEASE )