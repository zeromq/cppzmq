include(ExternalProject)

ExternalProject_Add(
  catch
  PREFIX ${CMAKE_BINARY_DIR}/catch
  URL "https://raw.githubusercontent.com/catchorg/Catch2/Catch1.x/single_include/catch.hpp"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  TEST_COMMAND ""
  DOWNLOAD_NO_EXTRACT ON
)

# Expose variable CATCH_INCLUDE_DIR to parent scope
ExternalProject_Get_Property(catch DOWNLOAD_DIR)
set(CATCH_INCLUDE_DIR ${DOWNLOAD_DIR} CACHE INTERNAL "Path to include catch")
