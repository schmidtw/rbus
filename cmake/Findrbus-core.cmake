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

find_package(PkgConfig)

find_library(RBUSCORE_LIBRARIES NAMES rbus-core)
find_path(RBUSCORE_INCLUDE_DIRS NAMES rbus_core.h PATH_SUFFIXES rbus-core )

set(RBUSCORE_LIBRARIES ${RBUSCORE_LIBRARIES} CACHE PATH "Path to rbus-core library")
set(RBUSCORE_INCLUDE_DIRS ${RBUSCORE_INCLUDE_DIRS} )
set(RBUSCORE_INCLUDE_DIRS ${RBUSCORE_INCLUDE_DIRS} CACHE PATH "Path to rbus-core include")

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(RBUSCORE DEFAULT_MSG RBUSCORE_INCLUDE_DIRS RBUSCORE_LIBRARIES)

mark_as_advanced(
    RBUSCORE_FOUND
    RBUSCORE_INCLUDE_DIRS
    RBUSCORE_LIBRARIES
    RBUSCORE_LIBRARY_DIRS
    RBUSCORE_FLAGS)
