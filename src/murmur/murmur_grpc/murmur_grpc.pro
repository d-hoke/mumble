# Copyright 2005-2019 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# Mumble source tree or at <https://www.mumble.info/LICENSE>.

include(../../../qmake/compiler.pri)
include(../../../qmake/protoc.pri)

GRPC *= ../MurmurRPC.proto

grpc_pbh.output = ${QMAKE_FILE_BASE}.pb.h
grpc_pbh.depends = ${QMAKE_FILE_BASE}.pb.cc
grpc_pbh.commands = $$escape_expand(\\n)
grpc_pbh.input = GRPC
grpc_pbh.CONFIG *= no_link explicit_dependencies target_predeps
grpc_pbh.variable_out = HEADERS

grpc_pb.output = ${QMAKE_FILE_BASE}.pb.cc
grpc_pb.commands = $${PROTOC} --cpp_out=. -I. -I.. ${QMAKE_FILE_NAME}
grpc_pb.input = GRPC
grpc_pb.CONFIG *= no_link explicit_dependencies
grpc_pb.variable_out = SOURCES

grpch.output = ${QMAKE_FILE_BASE}.grpc.pb.h
grpch.depends = ${QMAKE_FILE_BASE}.grpc.pb.cc
grpch.commands = $$escape_expand(\\n)
grpch.input = GRPC
grpch.CONFIG *= no_link explicit_dependencies target_predeps

grpc.output = ${QMAKE_FILE_BASE}.grpc.pb.cc
grpc.depends = ${QMAKE_FILE_BASE}.pb.h
win32 {
#hmm, how did it work for release, what's diff in debug?  And, do we need path since protoc doesn't really seem to correctly
#utilize it anyway? And we had already previously added its location to path in prep.cmd...
#  grpc.commands = $${PROTOC} --grpc_out=. --plugin=protoc-gen-grpc=$$system(which grpc_cpp_plugin) -I. -I.. ${QMAKE_FILE_NAME}
  grpc.commands = $${PROTOC} --grpc_out=. --plugin=protoc-gen-grpc=$$system(cygpath -w $$system(which grpc_cpp_plugin)) -I. -I.. ${QMAKE_FILE_NAME}
#  grpc.commands = $${PROTOC} --grpc_out=. --plugin=protoc-gen-grpc=grpc_cpp_plugin -I. -I.. ${QMAKE_FILE_NAME}
} else {
  grpc.commands = $${PROTOC} --grpc_out=. --plugin=protoc-gen-grpc=$$system(which grpc_cpp_plugin) -I. -I.. ${QMAKE_FILE_NAME}
}
grpc.input = GRPC
grpc.CONFIG *= no_link explicit_dependencies
grpc.variable_out = SOURCES

TEMPLATE = lib
CONFIG -= qt
CONFIG += debug_and_release
CONFIG += staticlib

QMAKE_EXTRA_COMPILERS *= grpc_pb grpc_pbh grpc grpch

!CONFIG(third-party-warnings) {
  # We ignore warnings in third party builds. We won't actually look
  # at them and they clutter out our warnings.
  CONFIG -= warn_on
  CONFIG += warn_off
}

unix {
  QMAKE_CXXFLAGS *= -std=c++11
}

include(../../../qmake/symbols.pri)
