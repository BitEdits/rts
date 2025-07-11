# ##############################################################################
# mm/kasan/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################
set(SRCS hook.c)

if(CONFIG_MM_KASAN)
  list(APPEND FLAGS ${NO_LTO})
  list(APPEND FLAGS -fno-builtin)
  list(APPEND FLAGS -fno-sanitize=kernel-address)

  if(NOT "${CONFIG_MM_KASAN_MARK_LOCATION}" STREQUAL "")
    target_compile_definitions(
      mm PRIVATE -DMM_KASAN_MARK_LOCATION="${CONFIG_MM_KASAN_MARK_LOCATION}")
  endif()

endif()

target_sources(mm PRIVATE ${SRCS})
target_compile_options(mm PRIVATE ${FLAGS})
