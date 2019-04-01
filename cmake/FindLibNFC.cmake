# FindLibNFC.cmake
#
# Custom script to find LibNFC libraries provided by BP.ThirdParty.

find_library(LibNFC_LIBRARY nfc)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibNFC
  FOUND_VAR      LibNFC_FOUND
  REQUIRED_VARS  LibNFC_LIBRARY)

if(LibNFC_FOUND)
  set(LibNFC_LIBRARIES "${LibNFC_LIBRARY}")
else()
  	message(FATAL_ERROR "Cannot find NFC") 
endif()
