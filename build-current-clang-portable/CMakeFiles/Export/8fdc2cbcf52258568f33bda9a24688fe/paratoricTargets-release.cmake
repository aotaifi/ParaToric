#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "paratoric::core" for configuration "Release"
set_property(TARGET paratoric::core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(paratoric::core PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib64/libparatoric_core.a"
  )

list(APPEND _cmake_import_check_targets paratoric::core )
list(APPEND _cmake_import_check_files_for_paratoric::core "${_IMPORT_PREFIX}/lib64/libparatoric_core.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
