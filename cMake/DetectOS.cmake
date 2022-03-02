# this project is inspired by the gist:  IdentifyOS.cmake <https://gist.github.com/lbaehren/2413369>
# usage: include(DetectOS.cmake) then some variables are availables:  CMAKE_OS_NAME, CMAKE_OS_ID
# see github readme.md for more details guide
# by Qingfeng Xia, UKAEA, 2020
# BSD licensed 


# detect Linux favor/distribution ID, version detection
# detect windows 10 version/build number
# CMAKE_OS_ID  for linux /etc/os-release file's ID content

if (CMAKE_HOST_WIN32)
    set (CMAKE_OS_NAME "Windows" CACHE STRING "Operating system name" FORCE)
    # visual studio may target on diff SDK version, can be detected by
    # CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION   (visual studio target SDK version)
    # https://cmake.org/cmake/help/latest/variable/CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION.html
    
    #windows favour including:  *_STORE,  *_PHONE

    # windows has various toolset/compiler, so `CMAKE_GENERATOR`
    if (MINGW)  ## GCC compiler should be still True
    endif()
    # MSYS2, Cygwin
endif()

if (UNIX)  # UNIX-like POSIX

  if (APPLE)  # it is an unix-like os, APPLE platforms: IOS

    #set (CMAKE_OS_NAME "Darwin" CACHE STRING "Operating system name" FORCE)
    #  uname -r
    #EXEC_PROGRAM(uname ARGS -v  OUTPUT_VARIABLE DARWIN_VERSION)
    #STRING(REGEX MATCH "[0-9]+" DARWIN_VERSION ${DARWIN_VERSION})
    #MESSAGE(STATUS "DARWIN_VERSION=${DARWIN_VERSION}")

    set(CMAKE_OS_NAME "MacOS" CACHE STRING "Operating system name" FORCE)
    EXEC_PROGRAM(sw_vers ARGS -productVersion  OUTPUT_VARIABLE MACOS_VERSION)
    # strip 10.15.7 to  10.15
    MESSAGE(STATUS "MACOS_VERSION=${MACOS_VERSION}")
    set(CMAKE_OS_VERSION ${MACOS_VERSION})

  else (APPLE)

    ## Linux OS found
    find_file(LINUX_OS_RELEASE_FOUND os-release
      PATHS /etc
    )
    if (LINUX_OS_RELEASE_FOUND)
        set(GET_OS_ID_CMD  "awk '/DISTRIB_ID=/' /etc/os-release | sed 's/DISTRIB_ID=//;' ")
        # must use string inside the execute_process() not by "${GET_OS_VERSION_CMD}"
        # https://stackoverflow.com/questions/35689501/cmakes-execute-process-and-arbitrary-shell-scripts
        execute_process(COMMAND  bash "-c" "awk '/DISTRIB_ID=/' /etc/*-release | sed 's/DISTRIB_ID=//' | tr '[:upper:]' '[:lower:]' "
          OUTPUT_VARIABLE CMAKE_OS_ID
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        # CMake execute_process() does not support pipe, so must use a shell
        # not sure if all distro has the number version id,  "| sed 's/[.]0/./' " will remove zero after dot
        set(GET_OS_VERSION_CMD "bash -c \"awk '/VERSION_ID=/' /etc/*-release | sed 's/VERSION_ID=//' | sed 's/\"//' | sed 's/\"$//' ")
        execute_process(COMMAND  bash "-c"  "awk '/VERSION_ID=/' /etc/*-release | sed 's/VERSION_ID=//' | sed 's/\"//' | sed 's/\"$//'  "
          OUTPUT_VARIABLE CMAKE_OS_VERSION
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        #message(STATUS "OS detected by `${GET_OS_ID_CMD}` \n `${GET_OS_VERSION_CMD}`")
        # all empty!
        message(STATUS "OS detected as ID=${CMAKE_OS_ID},  VERSION=${CMAKE_OS_VERSION}")
        
        if(NOT CMAKE_OS_ID)  # error during detect distro name, may return empty string
            set(CMAKE_OS_ID "unknown_linux_distro")
        endif()
        set(LINUX_FOUND TRUE CACHE STRING "Linux Operating system found" FORCE)
    endif()

    ## Check for Debian GNU/Linux family ________________
    find_file (DEBIAN_FOUND debian_version debconf.conf
        PATHS /etc
    )
    if (DEBIAN_FOUND)
        set (CMAKE_OS_NAME "Debian" CACHE STRING "Operating system name" FORCE)
        set (LINUX_PACKAGE_FORMAT "deb" CACHE STRING "linux distro native package format rpm|deb" FORCE)
    else(DEBIAN_FOUND)
        set (LINUX_PACKAGE_FORMAT "rpm" CACHE STRING "linux distro native package format rpm|deb" FORCE)
    endif (DEBIAN_FOUND)


  if ("${CMAKE_OS_ID}" STREQUAL  "sles"  OR "${CMAKE_OS_ID}" STREQUAL  "opensuse")
    set (CMAKE_OS_NAME "Suse" CACHE STRING "Operating system name" FORCE)
    
  endif()

  ## Extra check for Ubuntu ____________________
  if ("${CMAKE_OS_ID}" STREQUAL  "ubuntu")   
    # without a quote enclosing ${CMAKE_OS_ID}, STREQUAL fails if CMAKE_OS_ID is empty
    set (CMAKE_OS_NAME Ubuntu CACHE STRING "Operating system name" FORCE)
    # only debian and ubuntu has codename
    find_program(LSB_RELEASE_COMMAND lsb_release)  #  uname must be there
    if(LSB_RELEASE_COMMAND)  # lsb_release is not always available, docker image may not install it
      execute_process(COMMAND ${LSB_RELEASE_COMMAND} --codename
          OUTPUT_VARIABLE UBUNTU_CODENAME
          OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    endif()
  endif()

    ##  Check for Fedora _________________________
    find_file (FEDORA_FOUND fedora-release
      PATHS /etc
      )
    if (FEDORA_FOUND)
      set (CMAKE_OS_NAME "Fedora" CACHE STRING "Operating system name" FORCE)
    endif (FEDORA_FOUND)

    ##  Check for RedHat/Centos _________________________
    find_file (REDHAT_FOUND redhat-release inittab.RH
      PATHS /etc
      )
    if (REDHAT_FOUND)
      set (CMAKE_OS_NAME "RedHat" CACHE STRING "Operating system name" FORCE)
    endif (REDHAT_FOUND)

  endif (APPLE)

endif (UNIX)