set(VISP_CXX_STANDARD_98 199711L) # Keep for compat with previous releases
set(VISP_CXX_STANDARD_11 201103L)
set(VISP_CXX_STANDARD_14 201402L)
set(VISP_CXX_STANDARD_17 201703L)

if(DEFINED USE_CXX_STANDARD)

  if(USE_CXX_STANDARD STREQUAL "98")
    set(CMAKE_CXX_STANDARD 98)
    set(VISP_CXX_STANDARD ${VISP_CXX_STANDARD_98})
    set(CXX98_CXX_FLAGS "-std=c++98" CACHE STRING "C++ compiler flags for C++98 support")
    mark_as_advanced(CXX98_CXX_FLAGS)
  elseif(USE_CXX_STANDARD STREQUAL "11")
    set(CMAKE_CXX_STANDARD 11)
    set(VISP_CXX_STANDARD ${VISP_CXX_STANDARD_11})
    vp_check_compiler_flag(CXX "-std=c++11" HAVE_STD_CXX11_FLAG "${PROJECT_SOURCE_DIR}/cmake/checks/cxx11.cpp")
    if(HAVE_STD_CXX11_FLAG)
      set(CXX11_CXX_FLAGS "-std=c++11" CACHE STRING "C++ compiler flags for C++11 support")
      mark_as_advanced(CXX11_CXX_FLAGS)
    endif()
    mark_as_advanced(HAVE_STD_CXX11_FLAG)
  elseif(USE_CXX_STANDARD STREQUAL "14")
    set(CMAKE_CXX_STANDARD 14)
    set(VISP_CXX_STANDARD ${VISP_CXX_STANDARD_14})
    vp_check_compiler_flag(CXX "-std=c++14" HAVE_STD_CXX14_FLAG "${PROJECT_SOURCE_DIR}/cmake/checks/cxx14.cpp")
    if(HAVE_STD_CXX14_FLAG)
      set(CXX14_CXX_FLAGS "-std=c++14" CACHE STRING "C++ compiler flags for C++14 support")
      mark_as_advanced(CXX14_CXX_FLAGS)
    endif()
    mark_as_advanced(HAVE_STD_CXX14_FLAG)
  elseif(USE_CXX_STANDARD STREQUAL "17")
    set(CMAKE_CXX_STANDARD 17)
    set(VISP_CXX_STANDARD ${VISP_CXX_STANDARD_17})
    vp_check_compiler_flag(CXX "-std=c++17" HAVE_STD_CXX17_FLAG "${PROJECT_SOURCE_DIR}/cmake/checks/cxx17.cpp")
    if(HAVE_STD_CXX17_FLAG)
      set(CXX17_CXX_FLAGS "-std=c++17" CACHE STRING "C++ compiler flags for C++17 support")
      mark_as_advanced(CXX17_CXX_FLAGS)
    endif()
    mark_as_advanced(HAVE_STD_CXX17_FLAG)
  endif()

  set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++11 instead of -std=gnu++11

  # Additional check for nullptr that is available with clang but not with g++ when cxx98 standard is enabled
  # Here FORCE optional argument is used to ensure that the check is launched when cxx standard is modified
  vp_check_compiler_flag(CXX "" HAVE_NULLPTR FORCE "${PROJECT_SOURCE_DIR}/cmake/checks/nullptr.cpp")

  mark_as_advanced(HAVE_NULLPTR)

else()

  set(AVAILABLE_CXX_STANDARD TRUE)
  set(VISP_CXX_STANDARD ${VISP_CXX_STANDARD_98})

  set(CMAKE_CXX_STANDARD_REQUIRED FALSE)

  if(CMAKE_CXX98_COMPILE_FEATURES)
    set(CXX98_STANDARD_FOUND ON CACHE STRING "cxx98 standard")
    mark_as_advanced(CXX98_STANDARD_FOUND)
  endif()

  if(CMAKE_CXX11_COMPILE_FEATURES)
    # cxx11 implementation maybe incomplete especially with g++ 4.6.x.
    # That's why we check more in depth for cxx_override that is not available with g++ 4.6.3
    list (FIND CMAKE_CXX11_COMPILE_FEATURES "cxx_override" _index)
    if (${_index} GREATER -1)
      # Setting CMAKE_CXX_STANDARD doesn't affect check_cxx_source_compiles() used to detect isnan() in FindIsNaN.cmake
      # or erfc() in FindErfc.cmake.
      # That's why we have the following lines that are used to set cxx flags corresponding to the c++ standard
      vp_check_compiler_flag(CXX "" HAVE_CXX11_FLAG "${PROJECT_SOURCE_DIR}/cmake/checks/cxx11.cpp")
      if(HAVE_CXX11_FLAG)
        set(CXX11_STANDARD_FOUND ON CACHE STRING "cxx11 standard")
        mark_as_advanced(CXX11_STANDARD_FOUND)
        mark_as_advanced(HAVE_CXX11_FLAG)
      else()
        vp_check_compiler_flag(CXX "-std=c++11" HAVE_STD_CXX11_FLAG "${PROJECT_SOURCE_DIR}/cmake/checks/cxx11.cpp")
        if(HAVE_STD_CXX11_FLAG)
          set(CXX11_CXX_FLAGS "-std=c++11" CACHE STRING "C++ compiler flags for C++11 support")
          set(CXX11_STANDARD_FOUND ON CACHE STRING "cxx11 standard")
          mark_as_advanced(CXX11_STANDARD_FOUND)
          mark_as_advanced(CXX11_CXX_FLAGS)
          mark_as_advanced(HAVE_STD_CXX11_FLAG)
        endif()
      endif()
    endif()
  endif()

  if(CMAKE_CXX14_COMPILE_FEATURES)
    # Additional check in case of c++14 is incomplete and also needed to set CXX14_CXX_FLAGS
    vp_check_compiler_flag(CXX "" HAVE_CXX14_FLAG "${PROJECT_SOURCE_DIR}/cmake/checks/cxx14.cpp")
    if(HAVE_CXX14_FLAG)
      set(CXX14_STANDARD_FOUND ON CACHE STRING "cxx14 standard")
      mark_as_advanced(CXX14_STANDARD_FOUND)
      mark_as_advanced(HAVE_CXX14_FLAG)
    else()
      vp_check_compiler_flag(CXX "-std=c++14" HAVE_STD_CXX14_FLAG "${PROJECT_SOURCE_DIR}/cmake/checks/cxx14.cpp")
      if(HAVE_STD_CXX14_FLAG)
        set(CXX14_CXX_FLAGS "-std=c++14" CACHE STRING "C++ compiler flags for C++14 support")
        set(CXX14_STANDARD_FOUND ON CACHE STRING "cxx14 standard")
        mark_as_advanced(CXX14_STANDARD_FOUND)
        mark_as_advanced(CXX14_CXX_FLAGS)
        mark_as_advanced(HAVE_STD_CXX14_FLAG)
      endif()
    endif()
  endif()

  if(CMAKE_CXX17_COMPILE_FEATURES)
    # c++17 seems available, but on Fedora 25 and g++ 6.3.1 it seems incomplete where
    # optional header is not found as well as std::clamp()
    vp_check_compiler_flag(CXX "" HAVE_CXX17_FLAG "${PROJECT_SOURCE_DIR}/cmake/checks/cxx17.cpp")
    if(HAVE_CXX17_FLAG)
      set(CXX17_STANDARD_FOUND ON CACHE STRING "cxx17 standard")
      mark_as_advanced(CXX17_STANDARD_FOUND)
      mark_as_advanced(HAVE_CXX17_FLAG)
    else()
      vp_check_compiler_flag(CXX "-std=c++17" HAVE_STD_CXX17_FLAG "${PROJECT_SOURCE_DIR}/cmake/checks/cxx17.cpp")
      if(HAVE_STD_CXX17_FLAG)
        set(CXX17_CXX_FLAGS "-std=c++17" CACHE STRING "C++ compiler flags for C++17 support")
        set(CXX17_STANDARD_FOUND ON CACHE STRING "cxx17 standard")
        mark_as_advanced(CXX17_STANDARD_FOUND)
        mark_as_advanced(CXX17_CXX_FLAGS)
        mark_as_advanced(HAVE_STD_CXX17_FLAG)
      endif()
    endif()
  endif()

  # Set default c++ standard to 17, the first in the list
  if(CXX17_STANDARD_FOUND)
    set(USE_CXX_STANDARD "17" CACHE STRING "Selected cxx standard")
  elseif(CXX14_STANDARD_FOUND)
    set(USE_CXX_STANDARD "14" CACHE STRING "Selected cxx standard")
  elseif(CXX11_STANDARD_FOUND)
    set(USE_CXX_STANDARD "11" CACHE STRING "Selected cxx standard")
  elseif(CXX98_STANDARD_FOUND)
    set(USE_CXX_STANDARD "98" CACHE STRING "Selected cxx standard")
  else()
    set(AVAILABLE_CXX_STANDARD FALSE)
  endif()

  # Hack for msvc12 (Visual 2013) where C++11 implementation is incomplete
  if(MSVC_VERSION EQUAL 1800)
    set(CXX11_STANDARD_FOUND TRUE)
  endif()

  if(AVAILABLE_CXX_STANDARD)
    if(CXX98_STANDARD_FOUND)
      set_property(CACHE USE_CXX_STANDARD APPEND_STRING PROPERTY STRINGS "98")
    endif()
    if(CXX11_STANDARD_FOUND)
      set_property(CACHE USE_CXX_STANDARD APPEND_STRING PROPERTY STRINGS ";11")
    endif()
    if(CXX14_STANDARD_FOUND)
      set_property(CACHE USE_CXX_STANDARD APPEND_STRING PROPERTY STRINGS ";14")
    endif()
    if(CXX17_STANDARD_FOUND)
      set_property(CACHE USE_CXX_STANDARD APPEND_STRING PROPERTY STRINGS ";17")
    endif()

    if(USE_CXX_STANDARD STREQUAL "98")
      set(CMAKE_CXX_STANDARD 98)
      set(VISP_CXX_STANDARD ${VISP_CXX_STANDARD_98})
      set(CXX98_CXX_FLAGS "-std=c++98" CACHE STRING "C++ compiler flags for C++98 support")
      mark_as_advanced(CXX98_CXX_FLAGS)
    elseif(USE_CXX_STANDARD STREQUAL "11")
      set(CMAKE_CXX_STANDARD 11)
      set(VISP_CXX_STANDARD ${VISP_CXX_STANDARD_11})
      vp_check_compiler_flag(CXX "-std=c++11" HAVE_STD_CXX11_FLAG "${PROJECT_SOURCE_DIR}/cmake/checks/cxx11.cpp")
      if(HAVE_STD_CXX11_FLAG)
        set(CXX11_CXX_FLAGS "-std=c++11" CACHE STRING "C++ compiler flags for C++11 support")
        mark_as_advanced(CXX11_CXX_FLAGS)
      endif()
      mark_as_advanced(HAVE_STD_CXX11_FLAG)
    elseif(USE_CXX_STANDARD STREQUAL "14")
      set(CMAKE_CXX_STANDARD 14)
      set(VISP_CXX_STANDARD ${VISP_CXX_STANDARD_14})
      vp_check_compiler_flag(CXX "-std=c++14" HAVE_STD_CXX14_FLAG "${PROJECT_SOURCE_DIR}/cmake/checks/cxx14.cpp")
      if(HAVE_STD_CXX14_FLAG)
        set(CXX14_CXX_FLAGS "-std=c++14" CACHE STRING "C++ compiler flags for C++14 support")
        mark_as_advanced(CXX14_CXX_FLAGS)
      endif()
      mark_as_advanced(HAVE_STD_CXX14_FLAG)
    elseif(USE_CXX_STANDARD STREQUAL "17")
      set(CMAKE_CXX_STANDARD 17)
      set(VISP_CXX_STANDARD ${VISP_CXX_STANDARD_17})
      vp_check_compiler_flag(CXX "-std=c++17" HAVE_STD_CXX17_FLAG "${PROJECT_SOURCE_DIR}/cmake/checks/cxx17.cpp")
      if(HAVE_STD_CXX17_FLAG)
        set(CXX17_CXX_FLAGS "-std=c++17" CACHE STRING "C++ compiler flags for C++17 support")
        mark_as_advanced(CXX17_CXX_FLAGS)
      endif()
      mark_as_advanced(HAVE_STD_CXX17_FLAG)
    endif()
  endif()

  set(CMAKE_CXX_EXTENSIONS OFF) # use -std=c++11 instead of -std=gnu++11

  # Additional check for nullptr that is available with clang but not with g++ when cxx98 standard is enabled
  # Here FORCE optional argument is used to ensure that the check is launched when cxx standard is modified
  vp_check_compiler_flag(CXX "" HAVE_NULLPTR FORCE "${PROJECT_SOURCE_DIR}/cmake/checks/nullptr.cpp")

  mark_as_advanced(HAVE_NULLPTR)
endif()
