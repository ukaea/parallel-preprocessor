###########################################
# Check 32/64 bit platform #
# copy from FreeCAD project, LGPL v3
###########################################
if (${CMAKE_SIZEOF_VOID_P} MATCHES "8") # It is 64bit, otherwise 32 bit systems match 4
	add_definitions(-D_OCC64)
	set(BIT_LENGTH 64)
else (${CMAKE_SIZEOF_VOID_P} MATCHES "8")
	set(BIT_LENGTH 32)
endif(${CMAKE_SIZEOF_VOID_P} MATCHES "8")

# Define helper macro option_with_default
macro( option_with_default OPTION_NAME OPTION_STRING OPTION_DEFAULT )
	if( NOT DEFINED ${OPTION_NAME} )
		set( ${OPTION_NAME} ${OPTION_DEFAULT} )
	endif( NOT DEFINED ${OPTION_NAME} )
	option( ${OPTION_NAME} "${OPTION_STRING}" ${${OPTION_NAME}} )
endmacro( option_with_default OPTION_NAME OPTION_STRING OPTION_DEFAULT )

############################################################
# Create DEB, RPM, activated by the command `make package` or `cpack`
# simultaneously building multiple package types -DCPACK_GENERATOR="DEB;RPM")
############################################################

if (UNIX)
    include("${PROJECT_SOURCE_DIR}/cMake/DetectOS.cmake")
    message("OS detected by `DetectOS.cmake` as `${CMAKE_OS_NAME}` and `${CMAKE_OS_VERSION}`")
    if (${CMAKE_OS_NAME} STREQUAL "Debian" OR ${CMAKE_OS_NAME} STREQUAL "Ubuntu")
        # Tell CPack to generate a .deb package
        set(CPACK_GENERATOR "DEB")
        set(CPACK_PACKAGE_NAME ${PACKAGE_NAME})
    endif()
    if (${CMAKE_OS_NAME} STREQUAL "RedHat" OR ${CMAKE_OS_NAME} STREQUAL "Fedora")
        set(CPACK_GENERATOR "RPM")
        set(CPACK_PACKAGE_NAME ${PACKAGE_NAME})
    endif()
    if (APPLE)
        set(CPACK_GENERATOR "DragNDrop")
        set(CPACK_PACKAGE_NAME ${PACKAGE_NAME})
    endif()

    # -${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}  not defined variable
    string( TOLOWER "${PACKAGE_NAME}-${PACKAGE_VERSION_NAME}_${CMAKE_OS_ID}-${CMAKE_OS_VERSION}" 
    CPACK_PACKAGE_FILE_NAME )
endif()
if(WIN32)
    # NSIS can be generated by opencascade and Qt dll are not bundled, so not quite working
    set(CPACK_GENERATOR "NSIS64")  # zip 7z are other choices
    set(CPACK_PACKAGE_NAME ${PACKAGE_NAME})
    set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY ${PACKAGE_NAME})
endif()

# Set a Package Maintainer. This is required
# https://github.com/ukaea/parallel-preprocessor
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Qingfeng Xia @ UKAEA")
#set(CPACK_PACKAGE_DESCRIPTION, ${PROJECT_BRIEF})
set(CPACK_PACKAGE_DESCRIPTION, "${CMAKE_CURRENT_SOURCE_DIR}/Readme.md")
# Set a Package Version
set(CPACK_PACKAGE_VERSION ${PACKAGE_VERSION_NAME})
if (NOT APPLE)
# this LICENSE file's end of line caused problem during dmg package generation on MacOS
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
endif()

message(STATUS "CPACK_PACKAGE_FILE_NAME: ${CPACK_PACKAGE_FILE_NAME}")

################################################################

#set(CPACK_IGNORE_FILES "\.psd$;/\.git/;/backup/;\.#;/#;\.tar.gz$;/stage/;/build/;/condabuild/;\.diff$;\.DS_Store")
set(CPACK_SOURCE_GENERATOR "ZIP")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PACKAGE_VERSION_NAME}")
set(CPACK_SOURCE_IGNORE_FILES ${CPACK_IGNORE_FILES})

#################################################################
#  split into the runtime, python and development component packages
# COMPONENT and GROUP are different concepts
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cpack/Component-Install-With-CPack
#################################################################

# https://dominoc925.blogspot.com/2016/09/create-windows-installer-using-cmake.html
set(PPP_SINGLE_PACKAGE ON)

if(NOT PPP_SINGLE_PACKAGE)
    set(CPACK_COMPONENTS_ALL applications libraries headers python data)
    # all components in one package is the default behaviour
    # windows and macos may select component in the GUI installer wizard
    # otherwise generate multiple component packages on Linux
    set(CPACK_RPM_COMPONENT_INSTALL ON)
    set(CPACK_DEB_COMPONENT_INSTALL ON)

    set (CPACK_COMPONENT_APPLICATIONS_DISPLAY_NAME "MyLib Applications")
    set (CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
    set (CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers")
    set (CPACK_COMPONENT_PYTHON_DISPLAY_NAME "python interface")
endif()

# set(CPACK_COMPONENTS_GROUPING IGNORE)  #1 package per component
# set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE) # 1 package for all 
# 1 package per component group the default behavior. for deb and rpm
#set(CPACK_COMPONENT_APPLICATIONS_GROUP "Runtime")
#set(CPACK_COMPONENT_LIBRARIES_GROUP "Development")
#set(CPACK_COMPONENT_HEADERS_GROUP "Development")


#####################################################################
# windows NSIS-specific CPack variables                  
# to enable, or cmake with -DCPACK_GENERATOR="NSIS64"          
# cmake -DCPACK_GENERATOR="ZIP" ..                                  
#######################################################################
if( CPACK_GENERATOR MATCHES ".*NSIS.*" )

    #set(CPACK_NSIS_INSTALLED_ICON_NAME "${APP_LOW_NAME}.ico")
    #set(CPACK_NSIS_HELP_LINK ${APP_URL})
    #set(CPACK_NSIS_URL_INFO_ABOUT ${APP_URL})
    set(CPACK_NSIS_CONTACT ${CPACK_DEBIAN_PACKAGE_MAINTAINER})

endif()


#####################################################################
# Debian-specific CPack variables                  
# automatic enabled by DetectOS.cmake                                                 
#######################################################################
if( CPACK_GENERATOR MATCHES ".*DEB.*" )
	set(CPACK_DEBIAN_PACKAGE_SECTION "science")
	# potentially split nonGui and Gui packages, lots of dep are given by OCC
	set(CPACK_DEBIAN_PACKAGE_BUILDS_DEPENDS "cmake (>= 2.8),
        libocct-foundation-dev, libocct-data-exchange-dev, libocct-modeling-data-dev,
        libocct-modeling-algorithms-dev, libocct-ocaf-dev, occt-misc,
        python3-dev, tbb2-dev")
    # optional GUI remated : libx11-6, libxt6, libxext6, libxi-dev, libxmu-dev,
    #           libfreetype6 (>= 2.2.1), libfreeimage, 
    #           libocct-visualization-dev, tcl8.5 (>= 8.5.0), tk8.5 (>= 8.5.0), 
    # on Ubuntu, version number is part of package name, no package alias?
    # so there is no easy to make it compatible for ubuntu 18.04, 20.04, just skip
    #   libocct-foundation-7.3, libocct-data-exchange-7.3, libocct-modeling-data,
    #   libocct-modeling-algorithms, libocct-ocaf,
	set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.3), python3, libtbb2,
        libgcc1 (>= 1:4.1.1), libgomp1 (>= 4.2.1), libstdc++6 (>= 4.4.0)"
        )
	set(CPACK_DEBIAN_PACKAGE_SUGGESTS "freecad")
	set(CPACK_DEBIAN_PACKAGE_PROVIDES ${PROJECT_NAME})
	if( BIT_LENGTH EQUAL 64 )
		set( CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64" )
	else()
		set( CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386" )
	endif()
endif()

###############################################################################
# RPM-specific CPack variables.                                               #
# automatic enabled by DetectOS.cmake, 
# or enforace,   cmake with -DCPACK_GENERATOR="RPM", or uncomment             #
# set(CPACK_GENERATOR "RPM")                                                  #
###############################################################################
if( CPACK_GENERATOR MATCHES ".*RPM.*" )
    # c and c++ runtime should be skipped:  libstdc++6 >= 4.4.0, libc6 >= 2.3, libgcc >= 4.1.1, libgomp >= 4.2.1
    # X-windows, tcl and tk, they are dep of OpenCASCADE
    # using dnf list <package name> to find out the version,  project installation guide has the package name
    # libfreetype6 >= 2.2.1, libgl1-mesa-glx, libglu1-mesa,  libx11-6, libxext6, libxt6 tcl >= 8.5.0, tk >= 8.5.0
    # tk, tcl, tk-devel, tcl-devel, freetype, freetype-devel, freeimage, freeimage-devel, 
    # glew-devel, SDL2-devel, SDL2_image-devel, glm-devel, libXmu-devel, libXi-devel,
    set( CPACK_RPM_PACKAGE_REQUIRES "tbb, tbb-devel, python3") # occt is not listed, since usr may compile from source
	set( CPACK_RPM_PACKAGE_PROVIDES ${PACKAGE_NAME})
	set( CPACK_PACKAGE_RELOCATABLE "FALSE" )
	if( BIT_LENGTH EQUAL 64 )
		set( CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64" )
	else()
		set( CPACK_RPM_PACKAGE_ARCHITECTURE "i586" )
	endif()

endif()

# finally include CPack
include(CPack)