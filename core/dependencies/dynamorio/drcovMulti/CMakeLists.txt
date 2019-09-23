# **********************************************************
# Copyright (c) 2010-2016 Google, Inc.    All rights reserved.
# Copyright (c) 2009-2010 VMware, Inc.    All rights reserved.
# **********************************************************

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of VMware, Inc. nor the names of its contributors may be
#   used to endorse or promote products derived from this software without
#   specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
# DAMAGE.

cmake_minimum_required(VERSION 2.6)

include(../../make/policies.cmake NO_POLICY_SCOPE)

# We do not need libc, and we avoid a 10x size increase in both our
# dll and pdb (plus we avoid stressing private library isolation) by
# not using it (i#714).
set(DynamoRIO_USE_LIBC OFF)

# add drcovMulti client
add_library(drcovMulti SHARED drcovMulti.c)
configure_DynamoRIO_client(drcovMulti)
use_DynamoRIO_extension(drcovMulti drcovlib)
use_DynamoRIO_extension(drcovMulti drx)
# We keep our shared libs in the lib dir, not the bin dir:
place_shared_lib_in_lib_dir(drcovMulti)

# ensure we rebuild if includes change
add_dependencies(drcovMulti api_headers)

# Provide a hint for how to use the client
if (NOT DynamoRIO_INTERNAL OR NOT "${CMAKE_GENERATOR}" MATCHES "Ninja")
  add_custom_command(TARGET drcovMulti
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -E echo "Usage: pass to drconfig or drrun: -t drcovMulti"
    VERBATIM)
endif ()



install_target(drcovMulti ${INSTALL_CLIENTS_LIB})


set(INSTALL_DRCOVMULTI_CONFIG ${INSTALL_CLIENTS_BASE})

if (X64)
  set(CONFIG_INSTALL ${PROJECT_BINARY_DIR}/drcovMulti.drrun64)
  set(CONFIG_BUILD ${PROJECT_BINARY_DIR}/tools/drcovMulti.drrun64)
else (X64)
  set(CONFIG_INSTALL ${PROJECT_BINARY_DIR}/drcovMulti.drrun32)
  set(CONFIG_BUILD ${PROJECT_BINARY_DIR}/tools/drcovMulti.drrun32)
endif (X64)

function (write_config_file dst libdir)
  file(WRITE  ${dst} "# drcovMulti tool config file\n")
  file(APPEND ${dst} "# DynamoRIO options\n")
  file(APPEND ${dst} "DR_OP=-nop_initial_bblock\n")
  file(APPEND ${dst} "# client tool path\n")
  file(APPEND ${dst} "CLIENT_REL=${libdir}/${LIB_PFX}drcovMulti${LIB_EXT}\n")
  file(APPEND ${dst} "# client tool options\n")
  file(APPEND ${dst} "TOOL_OP=\n")
endfunction ()

set(BUILD_CLIENTS_LIB clients/${INSTALL_LIB})

write_config_file(${CONFIG_INSTALL} ${INSTALL_CLIENTS_LIB})
write_config_file(${CONFIG_BUILD} ${BUILD_CLIENTS_LIB})

DR_install(FILES "${CONFIG_INSTALL}" DESTINATION ${INSTALL_DRCOVMULTI_CONFIG})
register_tool_file("drcovMulti")

# We provide a version of genhtml that has Windows support
DR_install(FILES "${PROJECT_SOURCE_DIR}/third_party/lcov/genhtml"
  DESTINATION ${INSTALL_CLIENTS_BIN}
  PERMISSIONS ${owner_access} OWNER_EXECUTE GROUP_READ GROUP_EXECUTE
  WORLD_READ WORLD_EXECUTE)


