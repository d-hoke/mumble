# Copyright 2005-2019 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# Mumble source tree or at <https://www.mumble.info/LICENSE>.

include(../mumble.pri)

TEMPLATE = app
TARGET = protoc-gen-murmur-grpcwrapper
LANGUAGE = C++
SOURCES = main.cpp
unix {
LIBS = -lprotoc
}
win32 {
#at least for msvc version, not sure if will work for MXE...
  LIBS = -llibprotoc -llibprotobuf
  CONFIG *= console
}
CONFIG -= qt
CONFIG += c++11
#for windows build (with grpc) we're currently fetching acceptable protobuf version 
unix:must_pkgconfig(protobuf)

include(../../qmake/symbols.pri)
