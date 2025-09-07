# require isl
message(STATUS "Checking osl")

include(ExternalProject)

set(OSL_BIN ${CMAKE_CURRENT_BINARY_DIR}/osl)
set(OSL_STATIC_LIB ${OSL_BIN}/lib/libosl.a)
set(OSL_INCLUDES ${OSL_BIN}/include)

file(MAKE_DIRECTORY ${OSL_INCLUDES})

ExternalProject_Add(
	osl-src
	GIT_REPOSITORY https://github.com/periscop/openscop.git
	GIT_TAG 0.9.7 #
	GIT_SUBMODULES_RECURSE false
	GIT_SUBMODULES ""
	PREFIX ${OSL_BIN}
	BUILD_IN_SOURCE true
	CONFIGURE_COMMAND ./autogen.sh && ./configure --prefix=${OSL_BIN} #
	PATCH_COMMAND ""
	INSTALL_COMMAND make install
	UPDATE_DISCONNECTED true
)


add_library(osl STATIC IMPORTED GLOBAL)
add_dependencies(osl osl-src)
set_target_properties(osl PROPERTIES IMPORTED_LOCATION ${OSL_STATIC_LIB})
set_target_properties(osl PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${OSL_INCLUDES})
