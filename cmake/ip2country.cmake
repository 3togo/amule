if (GEOIP_INCLUDE_DIR)
	set (CMAKE_REQUIRED_INCLUDES ${GEOIP_INCLUDE_DIR})
endif()

# Check for legacy GeoIP library (old implementation)
if (NOT GEOIP_LIB)
	include (CheckIncludeFile)

	check_include_file (GeoIP.h GEOIP_H)

	if (GEOIP_H)
		find_library (GEOIP_LIB GeoIP)

		if (NOT GEOIP_LIB AND GEOIP_INCLUDE_DIR)
			find_library (GEOIP_LIB GeoIP
				PATHS ${GEOIP_INCLUDE_DIR}
			)
		endif()

		if (NOT GEOIP_LIB)
			message (STATUS "GeoIP lib not found, using new implementation")
		else()
			message (STATUS "Legacy GeoIP found (compatibility mode)")
		endif()
	else()
		message (STATUS "GeoIP headers not found, using new implementation")
	endif()
endif()

# New implementation with MaxMind DB
find_package(maxminddb QUIET)
if (NOT maxminddb_FOUND)
    # Try alternative spelling
    find_package(MaxMindDB QUIET)
endif()

if (maxminddb_FOUND)
	message (STATUS "MaxMind DB found: ${maxminddb_VERSION}")
else()
	# Try to find manually
	find_path(MAXMINDDB_INCLUDE_DIR maxminddb.h)
	find_library(MAXMINDDB_LIBRARY NAMES maxminddb)
	
	if (MAXMINDDB_INCLUDE_DIR AND MAXMINDDB_LIBRARY)
		set(maxminddb_FOUND TRUE)
		set(maxminddb_INCLUDE_DIRS ${MAXMINDDB_INCLUDE_DIR})
		set(maxminddb_LIBRARIES ${MAXMINDDB_LIBRARY})
		message (STATUS "MaxMind DB found manually")
	else()
		message (WARNING "**************************************************")
		message (WARNING "libmaxminddb not found - GeoIP/country flags will be disabled")
		message (WARNING "Please install: sudo apt install libmaxminddb-dev")
		message (WARNING "**************************************************")
		set(maxminddb_FOUND FALSE)
	endif()
endif()

if (ENABLE_IP2COUNTRY)
	if (maxminddb_FOUND)
		# New implementation with MaxMind DB
		add_library (maxminddb::maxminddb UNKNOWN IMPORTED)
		
		set_target_properties (maxminddb::maxminddb PROPERTIES
			INTERFACE_COMPILE_DEFINITIONS "ENABLE_IP2COUNTRY"
			INTERFACE_INCLUDE_DIRECTORIES "${maxminddb_INCLUDE_DIRS}"
			IMPORTED_LOCATION "${maxminddb_LIBRARIES}"
		)
		message (STATUS "Using MaxMind DB implementation")
		
	elseif (GEOIP_LIB)
		# Legacy GeoIP implementation
		add_library (GeoIP::Shared UNKNOWN IMPORTED)
		
		set_target_properties (GeoIP::Shared PROPERTIES
			INTERFACE_COMPILE_DEFINITIONS "ENABLE_IP2COUNTRY"
			INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_REQUIRED_INCLUDES}"
			IMPORTED_LOCATION "${GEOIP_LIB}"
		)
		message (STATUS "Using legacy GeoIP implementation")
		
	else()
		# No GeoIP library found, disable support
		set (ENABLE_IP2COUNTRY FALSE)
		message (STATUS "No GeoIP library found, disabling IP2Country support")
	endif()
endif()

unset (CMAKE_REQUIRED_INCLUDES)