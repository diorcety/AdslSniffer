# http://www.cmake.org/pipermail/cmake/2006-October/011446.html
# Modified to use pkg config and use standard var names

#
# Find the CppUnit includes and library
#
# This module defines
# LOG4CPP_INCLUDE_DIR, where to find tiff.h, etc.
# LOG4CPP_LIBRARIES, the libraries to link against to use CppUnit.
# LOG4CPP_FOUND, If false, do not try to use CppUnit.

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_LOG4CPP "log4cpp")

FIND_PATH(LOG4CPP_INCLUDE_DIRS
    NAMES log4cpp/config.h
    HINTS ${PC_LOG4CPP_INCLUDE_DIR}
    PATHS
    /usr/local/include
    /usr/include
)

FIND_LIBRARY(LOG4CPP_LIBRARIES
    NAMES log4cpp
    HINTS ${PC_LOG4CPP_LIBDIR}
    PATHS
    ${LOG4CPP_INCLUDE_DIRS}/../lib
    /usr/local/lib
    /usr/lib
)

LIST(APPEND LOG4CPP_LIBRARIES ${CMAKE_DL_LIBS})

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LOG4CPP DEFAULT_MSG LOG4CPP_LIBRARIES LOG4CPP_INCLUDE_DIRS)
MARK_AS_ADVANCED(LOG4CPP_LIBRARIES LOG4CPP_INCLUDE_DIRS)
