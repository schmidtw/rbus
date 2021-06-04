#############################################################################
# If not stated otherwise in this file or this component's Licenses.txt file
# the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#############################################################################

include_directories(
    ../include
    )

if (BUILD_RBUS_INTERFACE_UTIL_APPS)

    if (BUILD_FOR_DESKTOP)

        include(ExternalProject)

        ExternalProject_Add(
            linenoise
            GIT_REPOSITORY https://github.com/antirez/linenoise.git
            PREFIX deps
            UPDATE_DISCONNECTED 1
            INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}
            INSTALL_COMMAND ""
            CONFIGURE_COMMAND ""
            BUILD_IN_SOURCE 1
            CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
            #PATCH_COMMAND patch -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/003-linenoise-oflag.patch
            BUILD_COMMAND gcc -c -Wall -Werror -fpic linenoise.c COMMAND gcc --shared -o liblinenoise.so linenoise.o)

        include_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}/deps/src/linenoise)

        link_directories(
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}/deps/src/linenoise
            ${CMAKE_BINARY_DIR}
            ${LIBRARY_DIR})

        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/deps/src/linenoise/liblinenoise.so DESTINATION lib)
        add_executable(rbuscli rbuscli.c)
        target_link_libraries(rbuscli rbus -llinenoise)

    else (BUILD_FOR_DESKTOP)
        add_executable(rbuscli rbuscli.c)
        target_link_libraries(rbuscli rbus ${LINENOISE_LIBRARIES})
        target_include_directories (rbuscli PUBLIC ${LINENOISE_INCLUDE_DIRS})
    endif (BUILD_FOR_DESKTOP)
    add_dependencies(rbuscli rbus linenoise)

    install (TARGETS rbuscli RUNTIME DESTINATION bin)

endif (BUILD_RBUS_INTERFACE_UTIL_APPS)

