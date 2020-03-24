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

cmake_minimum_required (VERSION 2.8.7)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

find_package(PkgConfig REQUIRED)
find_package(rbus-core REQUIRED)
find_package(rtMessage REQUIRED)

set(CMAKE_C_FLAGS_DEBUG "-Wall -Wextra -g -O0 -fno-inline")
set(CMAKE_C_FLAGS_RELEASE "-Werror -Wall -Wextra")

option(BUILD_RBUS_INTERFACE_SAMPLE_APPS "BUILD_RBUS_INTERFACE_SAMPLE_APPS" ON)
option(BUILD_RBUS_INTERFACE_TEST_APPS "BUILD_RBUS_INTERFACE_TEST_APPS" ON)
option(BUILD_RBUS_INTERFACE_UTIL_APPS "BUILD_RBUS_INTERFACE_UTIL_APPS" ON)

add_subdirectory(src)

if(BUILD_RBUS_INTERFACE_SAMPLE_APPS)
    add_subdirectory(sampleapps)
endif()

if(BUILD_RBUS_INTERFACE_TEST_APPS)
    add_subdirectory(test)
endif()

if(BUILD_RBUS_INTERFACE_UTIL_APPS)
    add_subdirectory(util)
endif()
