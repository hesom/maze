set(QVR_VERSION 2.0.2)

####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was QVRConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

####################################################################################
set_and_check(QVR_INCLUDE_DIRS "${PACKAGE_PREFIX_DIR}/include")
set_and_check(QVR_LIBRARY_DIRS "${PACKAGE_PREFIX_DIR}/lib")
set(QVR_LIBRARIES qvr)
