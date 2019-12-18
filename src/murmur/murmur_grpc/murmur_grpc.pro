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
  # grpc_cpp_plugin.exe should be in path in (cygwin) windows build...
  # grpc.commands = $${PROTOC} --grpc_out=. --plugin=protoc-gen-grpc=grpc_cpp_plugin -I. -I.. ${QMAKE_FILE_NAME}
#  grpc.commands = $${PROTOC} --grpc_out=. --plugin=protoc-gen-grpc=c:\MumbleBuild\win32-static-1.3.x-2019-10-26-ce82683-899\cygwin\bin\grpc_protoc_plugins\grpc_cpp_plugin.exe -I. -I.. ${QMAKE_FILE_NAME}
#  grpc.commands = $${PROTOC} --grpc_out=. --plugin=protoc-gen-grpc=c:\MumbleBuild\win32-static-1.3.x-2019-10-26-ce82683-899\cygwin\bin\grpc_protoc_plugins\grpc_cpp_plugin.exe -I. -I.. -I../protobuf/include ${QMAKE_FILE_NAME}
#  grpc.commands = $${PROTOC} --grpc_out=. --plugin=protoc-gen-grpc=c:\MumbleBuild\win32-static-1.3.x-2019-10-26-ce82683-899\grpc\bin\grpc_cpp_plugin.exe -I. -I.. -I../protobuf/include ${QMAKE_FILE_NAME}
  grpc.commands = $${PROTOC} --grpc_out=. --plugin=protoc-gen-grpc=c:\MumbleBuild\win32-static-1.3.x-2019-10-26-ce82683-899\grpc\bin\grpc_cpp_plugin.exe -I. -I.. -I../grpc/protobuf/include ${QMAKE_FILE_NAME}
}
unix {
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

win32 {
#  QMAKE_CXXFLAGS *= -std:c++11 -I../protobuf/include
#  QMAKE_CXXFLAGS *= -std:c++11 -I${MUMBLE_PREFIX}/protobuf/include
#  QMAKE_CXXFLAGS *= -std:c++11 -I$${MUMBLE_PREFIX}/protobuf/include;$${MUMBLE_GRPC_PREFIX}/include
#  QMAKE_CXXFLAGS *= -std:c++11 -I$${MUMBLE_PREFIX}/protobuf/include;$${MUMBLE_PREFIX}/grpc-windows/grpc/include;$${MUMBLE_PREFIX}/grpc-windows/grpc/third_party/protobuf/src
#  QMAKE_CXXFLAGS *= --std:c++11 -I$${MUMBLE_PREFIX}/protobuf/include;$${MUMBLE_PREFIX}/grpc-windows/grpc/include;$${MUMBLE_PREFIX}/grpc-windows/grpc/third_party/protobuf/src
#  QMAKE_CXXFLAGS *= --std:c++11 -I$${MUMBLE_PREFIX}/protobuf/include/;$${MUMBLE_PREFIX}/grpc-windows/grpc/include/
#  QMAKE_CXXFLAGS *= -std:c++11 -I$${MUMBLE_PREFIX}/protobuf/include/ -I$${MUMBLE_PREFIX}/grpc-windows/grpc/include/ -D_WIN32_WINNT=0x600
#  QMAKE_CXXFLAGS *= -std:c++11 -I$${MUMBLE_PREFIX}/protobuf/include/ -I$${MUMBLE_PREFIX}/grpc/include/ -D_WIN32_WINNT=0x600
  QMAKE_CXXFLAGS *= -std:c++11 -I$${MUMBLE_PREFIX}/protobuf/src/ -I$${MUMBLE_PREFIX}/grpc/include/ -D_WIN32_WINNT=0x600
#  QMAKE_CXXFLAGS *= -std:c++11 -I$${MUMBLE_PREFIX}/grpc/protobuf/include/ -I$${MUMBLE_PREFIX}/grpc/include/ -D_WIN32_WINNT=0x600
#  QMAKE_CXXFLAGS *= -std:c++11 -I$${MUMBLE_PREFIX}/grpc/protobuf/include/ -I$${MUMBLE_PREFIX}/grpc/protobuf/src/ -I$${MUMBLE_PREFIX}/grpc/include/ -D_WIN32_WINNT=0x600
#  QMAKE_CXXFLAGS *= -std:c++11 -I$${MUMBLE_PREFIX}/grpc/third_party/protobuf/include/ -I$${MUMBLE_PREFIX}/grpc/third_party/protobuf/src/ -I$${MUMBLE_PREFIX}/grpc/include/ -D_WIN32_WINNT=0x600
#  QMAKE_CXXFLAGS *= -std:c++11 -I../../protobuf/include
}

include(../../../qmake/symbols.pri)
