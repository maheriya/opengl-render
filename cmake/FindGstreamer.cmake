# - Try to find Gstreamer-1.0
# Once done this will define
#  GSTREAMER_FOUND - System has Gstreamer-1.0
#  GSTREAMER_INCLUDE_DIRS - The Gstreamer-1.0 include directories
#  GSTREAMER_LIBRARIES - The libraries needed to use Gstreamer-1.0
#  GSTREAMER_DEFINITIONS - Compiler switches required for using Gstreamer-1.0

find_package(PkgConfig)
pkg_check_modules(PC_GSTREAMER QUIET gstreamer-1.0)
set(GSTREAMER_DEFINITIONS ${PC_GSTREAMER_CFLAGS_OTHER})

find_path(GSTREAMER_INCLUDE_DIR gst/gst.h
          HINTS ${PC_GSTREAMER_INCLUDEDIR} ${PC_GSTREAMER_INCLUDE_DIRS}
          PATH_SUFFIXES gstreamer-1.0 )

find_library(GSTREAMER_LIBRARY NAMES gstreamer-1.0
             HINTS ${PC_GSTREAMER_LIBDIR} ${PC_GSTREAMER_LIBRARY_DIRS} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set GSTREAMER_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Gstreamer  DEFAULT_MSG
                                  GSTREAMER_LIBRARY GSTREAMER_INCLUDE_DIR)

mark_as_advanced(GSTREAMER_INCLUDE_DIR GSTREAMER_LIBRARY )

set(GSTREAMER_LIBRARIES ${GSTREAMER_LIBRARY} )
set(GSTREAMER_INCLUDE_DIRS ${GSTREAMER_INCLUDE_DIR} )

