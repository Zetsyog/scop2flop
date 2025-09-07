message(STATUS "Checking polylib")

include(ExternalProject)

set(POLYLIB_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/polylib)
set(POLYLIB_STATIC_LIB ${POLYLIB_PREFIX}/lib/libpolylib64.a)
set(POLYLIB_INCLUDES ${POLYLIB_PREFIX}/include)

file(MAKE_DIRECTORY ${POLYLIB_INCLUDES})

ExternalProject_Add(
	polylib-src
	GIT_REPOSITORY https://github.com/vincentloechner/polylib.git
	GIT_TAG v5.22.8 #
	GIT_SUBMODULES_RECURSE false
	GIT_SUBMODULES ""
	PREFIX ${POLYLIB_PREFIX}
	BUILD_IN_SOURCE true
	CONFIGURE_COMMAND ./autogen.sh && ./configure --prefix=${POLYLIB_PREFIX} 
	PATCH_COMMAND ""
	INSTALL_COMMAND make install
	UPDATE_DISCONNECTED true
)


add_library(polylib STATIC IMPORTED GLOBAL)
add_dependencies(polylib polylib-src)
set_target_properties(polylib PROPERTIES IMPORTED_LOCATION ${POLYLIB_STATIC_LIB})
set_target_properties(polylib PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${POLYLIB_INCLUDES})
