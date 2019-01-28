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

# Expose variable CATCH_MODULE_PATH to parent scope
ExternalProject_Get_Property(catch DOWNLOAD_DIR)
set(CATCH_MODULE_PATH ${DOWNLOAD_DIR} CACHE INTERNAL "Path to include catch")

# Download module for CTest integration
if(NOT EXISTS "${CATCH_MODULE_PATH}/Catch.cmake")
    file(DOWNLOAD "https://raw.githubusercontent.com/catchorg/Catch2/master/contrib/Catch.cmake"
        "${CATCH_MODULE_PATH}/Catch.cmake")
endif()
if(NOT EXISTS "${CATCH_MODULE_PATH}/CatchAddTests.cmake")
    file(DOWNLOAD "https://raw.githubusercontent.com/catchorg/Catch2/master/contrib/CatchAddTests.cmake"
        "${CATCH_MODULE_PATH}/CatchAddTests.cmake")
endif()

