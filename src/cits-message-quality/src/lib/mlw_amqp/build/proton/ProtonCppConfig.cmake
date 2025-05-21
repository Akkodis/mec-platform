#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

# Name: Proton
# Description: Qpid Proton C library
# Version: 0.32.0
# URL: http://qpid.apache.org/proton/

set (ProtonCpp_VERSION       0.32.0)

set (ProtonCpp_INCLUDE_DIRS  /usr/local/include)
set (ProtonCpp_LIBRARIES     optimized /usr/local/lib/libqpid-proton-cpp.so debug /usr/local/lib/libqpid-proton-cpp.so)

if (NOT TARGET Proton::cpp)
  # Sigh.. have to make this compat with cmake 2.8.12
  add_library(Proton::cpp UNKNOWN IMPORTED)
  set_target_properties(Proton::cpp
    PROPERTIES
      IMPORTED_LOCATION "/usr/local/lib/libqpid-proton-cpp.so"
      IMPORTED_LOCATION_DEBUG "/usr/local/lib/libqpid-proton-cpp.so"
      INTERFACE_INCLUDE_DIRECTORIES "${ProtonCpp_INCLUDE_DIRS}")
endif()

set (ProtonCpp_FOUND True)
