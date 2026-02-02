if (SEARCH_DIR_UPNP)
	set (PKG_CONFIG_USE_CMAKE_PREFIX_PATH TRUE)
	set (CMAKE_PREFIX_PATH ${SEARCH_DIR_UPNP})
endif()

# Skip the CONFIG-based find_package as it often contains faulty configurations with hard-coded paths
# find_package (UPNP CONFIG)

# Use pkg-config directly as the primary method
include (FindPkgConfig)
pkg_check_modules (LIBUPNP libupnp QUIET)
unset (CMAKE_PREFIX_PATH)

if (LIBUPNP_FOUND)
	# Create imported target for UPnP library
	if (NOT TARGET UPNP::Shared)
		add_library (UPNP::Shared SHARED IMPORTED)
	endif()

	# Get the library file with proper name
	find_library(UPNP_LIBRARIES NAMES upnp libupnp PATHS ${LIBUPNP_LIBRARY_DIRS} NO_DEFAULT_PATH)
	if(NOT UPNP_LIBRARIES)
		find_library(UPNP_LIBRARIES NAMES upnp libupnp)
	endif()

	set_target_properties (UPNP::Shared PROPERTIES
		IMPORTED_LOCATION "${UPNP_LIBRARIES}"
		INTERFACE_INCLUDE_DIRECTORIES "${LIBUPNP_INCLUDE_DIRS}"
		INTERFACE_LINK_LIBRARIES "${LIBUPNP_LIBRARIES}"
	)
	set (UPNP_CONFIG TRUE)
elseif (NOT LIBUPNP_FOUND AND NOT DOWNLOAD_AND_BUILD_DEPS)
	set (ENABLE_UPNP FALSE)
	message (STATUS "lib-upnp not found, disabling upnp")
elseif (NOT LIBUPNP_FOUND AND DOWNLOAD_AND_BUILD_DEPS)
	CmDaB_install ("pupnp")
endif()
