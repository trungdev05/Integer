find_path(GMP_INCLUDE_DIR NAMES temp/gmp_bench/gmp.h gmp.h)
find_library(GMP_LIBRARY NAMES gmp libgmp)
find_library(GMPXX_LIBRARY NAMES gmpxx libgmpxx)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GMP DEFAULT_MSG GMP_INCLUDE_DIR GMP_LIBRARY GMPXX_LIBRARY)

if(GMP_FOUND)
  if (NOT TARGET gmp)
    add_library(gmp UNKNOWN IMPORTED)
    set_target_properties(gmp PROPERTIES
      IMPORTED_LOCATION "${GMP_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
    )
  endif()
  if (NOT TARGET gmpxx)
    add_library(gmpxx UNKNOWN IMPORTED)
    set_target_properties(gmpxx PROPERTIES
      IMPORTED_LOCATION "${GMPXX_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
    )
  endif()
endif()
