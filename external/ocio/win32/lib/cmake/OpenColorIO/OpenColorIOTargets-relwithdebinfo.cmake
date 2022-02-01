#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "OpenColorIO::OpenColorIO" for configuration "RelWithDebInfo"
set_property(TARGET OpenColorIO::OpenColorIO APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(OpenColorIO::OpenColorIO PROPERTIES
  IMPORTED_IMPLIB_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/OpenColorIO.lib"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/OpenColorIO_2_2.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS OpenColorIO::OpenColorIO )
list(APPEND _IMPORT_CHECK_FILES_FOR_OpenColorIO::OpenColorIO "${_IMPORT_PREFIX}/lib/OpenColorIO.lib" "${_IMPORT_PREFIX}/bin/OpenColorIO_2_2.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
