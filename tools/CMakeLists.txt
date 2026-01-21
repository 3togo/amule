# GeoIP Download Helper Tool
add_executable(geoip_download_helper geoip_download_helper.cpp)

# Link against wxWidgets
find_package(wxWidgets REQUIRED COMPONENTS core base)
target_link_libraries(geoip_download_helper ${wxWidgets_LIBRARIES})

# Install to tools directory
install(TARGETS geoip_download_helper DESTINATION tools)