# require isl
message(STATUS "Checking clan")

include(ExternalProject)

set(CLAN_BIN ${CMAKE_CURRENT_BINARY_DIR}/clan)
set(CLAN_STATIC_LIB ${CLAN_BIN}/lib/libclan.a)
set(CLAN_INCLUDES ${CLAN_BIN}/include)

file(MAKE_DIRECTORY ${CLAN_INCLUDES})

ExternalProject_Add(
	clan-src
	GIT_REPOSITORY https://github.com/periscop/clan.git
	GIT_TAG 0.8.1 #
	GIT_SUBMODULES_RECURSE false
	GIT_SUBMODULES ""
	PREFIX ${CLAN_BIN}
	BUILD_IN_SOURCE true
	CONFIGURE_COMMAND ./autogen.sh && ./configure --prefix=${CLAN_BIN} --with-osl=system
					  --with-osl-prefix=${OSL_BIN}
	PATCH_COMMAND ""
	INSTALL_COMMAND make install
	UPDATE_DISCONNECTED true
)

add_dependencies(clan-src osl-src)

add_library(clan STATIC IMPORTED GLOBAL)
add_dependencies(clan clan-src osl)
set_target_properties(clan PROPERTIES IMPORTED_LOCATION ${CLAN_STATIC_LIB})
set_target_properties(clan PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CLAN_INCLUDES})
