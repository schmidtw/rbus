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

if (BUILD_RBUS_INTERFACE_SAMPLE_APPS)

add_executable(rbusSampleProvider provider/rbusSampleProvider.c)
add_dependencies(rbusSampleProvider rbus)
target_link_libraries(rbusSampleProvider rbus)

add_executable(rbusSampleConsumer consumer/rbusSampleConsumer.c)
add_dependencies(rbusSampleConsumer rbus)
target_link_libraries(rbusSampleConsumer rbus)

add_executable(rbusEventProvider provider/rbusEventProvider.c)
add_dependencies(rbusEventProvider rbus)
target_link_libraries(rbusEventProvider rbus)

add_executable(rbusEventConsumer consumer/rbusEventConsumer.c)
add_dependencies(rbusEventConsumer rbus)
target_link_libraries(rbusEventConsumer rbus)

install (TARGETS rbusSampleConsumer rbusSampleProvider 
        RUNTIME DESTINATION bin)

endif (BUILD_RBUS_INTERFACE_SAMPLE_APPS)

