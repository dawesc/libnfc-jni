# FindLibLLCP.cmake
#
# Custom script to find LibLLCP libraries provided by BP.ThirdParty.

find_library(LibLLCP_LIBRARY llcp)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibLLCP
  FOUND_VAR      LibLLCP_FOUND
  REQUIRED_VARS  LibLLCP_LIBRARY)

if(LibLLCP_FOUND)
  set(LibLLCP_LIBRARIES "${LibLLCP_LIBRARY}")
else()
  message(FATAL_ERROR "Cannot find LLCP") 
endif()
