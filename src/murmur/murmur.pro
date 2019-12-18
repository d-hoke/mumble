# Copyright 2005-2019 The Mumble Developers. All rights reserved.
# Use of this source code is governed by a BSD-style license
# that can be found in the LICENSE file at the root of the
# Mumble source tree or at <https://www.mumble.info/LICENSE>.

include(../mumble.pri)
include(../../qmake/protoc.pri)

DEFINES *= MURMUR
TEMPLATE =app
CONFIG *= network
CONFIG -= gui
QT *= network sql xml
QT -= gui
TARGET = murmur
DBFILE = murmur.db
LANGUAGE = C++
FORMS =
HEADERS *= Server.h ServerUser.h Meta.h PBKDF2.h
SOURCES *= main.cpp Server.cpp ServerUser.cpp ServerDB.cpp Register.cpp Cert.cpp Messages.cpp Meta.cpp RPC.cpp PBKDF2.cpp

PRECOMPILED_HEADER = murmur_pch.h

!CONFIG(no-ice) {
  CONFIG *= ice
}

!CONFIG(no-dbus):!win32:!macx {
  CONFIG *= dbus
}

!CONFIG(no-bonjour) {
  CONFIG *= bonjour
}

win32 {
  RC_FILE = murmur.rc
  CONFIG *= gui
  QT *= gui
  isEqual(QT_MAJOR_VERSION, 5) {
    QT *= widgets
  }
  RESOURCES *= murmur.qrc
  SOURCES *= Tray.cpp About.cpp
  HEADERS *= Tray.h About.h
  LIBS *= -luser32 -ladvapi32
  win32-msvc* {
    QMAKE_POST_LINK = $$QMAKE_POST_LINK$$escape_expand(\\n\\t)$$quote(mt.exe -nologo -updateresource:$(DESTDIR_TARGET);1 -manifest ../mumble/mumble.appcompat.manifest)
  }
}

unix {
  contains(UNAME, Linux) {
    LIBS *= -lcap
  }

  CONFIG(static):!macx {
    QMAKE_LFLAGS *= -static
  }

  !macx:CONFIG(buildenv) {
    QMAKE_LFLAGS *= -Wl,-rpath,$$(MUMBLE_PREFIX)/lib:$$(MUMBLE_ICE_PREFIX)/lib
  }

  HEADERS *= UnixMurmur.h
  SOURCES *= UnixMurmur.cpp
  TARGET = murmurd
}

macx {
  CONFIG -= app_bundle
  LIBS *= -framework Security
  QMAKE_LFLAGS += -sectcreate __TEXT __info_plist murmur.plist
}

dbus {
  DEFINES *= USE_DBUS
  QT *= dbus
  HEADERS *= DBus.h
  SOURCES *= DBus.cpp
}

ice {
  SOURCES *= MurmurIce.cpp
  HEADERS *= MurmurIce.h

  win32:CONFIG(debug, debug|release) {
    LIBS *= -lIceD -lIceUtilD
  } else {
    # check Ice version, 3.7 merged IceUtil into Ice
    ICE_VERSION = $$system(slice2cpp --version 2>&1)
    ICE_MAJOR_VERSION = $$section(ICE_VERSION, ., 0, 0)
    ICE_MINOR_VERSION = $$section(ICE_VERSION, ., 1, 1)

    !equals(ICE_MAJOR_VERSION, 3) {
      error("Unsupported Ice version")
    }
    lessThan(ICE_MINOR_VERSION, 7) {
      # Ice < 3.7
      LIBS *= -lIce -lIceUtil
    }  else {
      # Ice 3.7+
      LIBS *= -lIce
    }
  }

  DEFINES *= USE_ICE

  win32 {
    INCLUDEPATH *= "$$ICE_PATH/include"
    !CONFIG(static) {
      QMAKE_LIBDIR *= "$$ICE_PATH/lib/vc100"
    } else {
      DEFINES *= ICE_STATIC_LIBS
      QMAKE_LIBDIR *= $$BZIP2_PATH/lib
      equals(MUMBLE_ARCH, x86) {
        QMAKE_LIBDIR *= $$ICE_PATH/lib
      }
      equals(MUMBLE_ARCH, x86_64) {
        QMAKE_LIBDIR *= $$ICE_PATH/lib/x64
      }
      CONFIG(release, debug|release): LIBS *= -llibbz2
      CONFIG(debug, debug|release):   LIBS *= -llibbz2d
      LIBS *= -ldbghelp -liphlpapi -lrpcrt4
    }
  }

  macx {
    INCLUDEPATH *= $$(MUMBLE_PREFIX)/Ice-3.4.2/include/
    QMAKE_LIBDIR *= $$(MUMBLE_PREFIX)/Ice-3.4.2/lib/
  }

  unix:!macx:CONFIG(buildenv) {
    INCLUDEPATH *= $$(MUMBLE_ICE_PREFIX)/include/
    QMAKE_LIBDIR *= $$(MUMBLE_ICE_PREFIX)/lib/
  }

  unix:!macx:CONFIG(static) {
    LIBS *= -lbz2
    QMAKE_CXXFLAGS *= -fPIC
  }

  macx:CONFIG(static) {
    LIBS *= -lbz2 -liconv
    QMAKE_CXXFLAGS *= -fPIC
  }

  LIBS *= -lmurmur_ice
  INCLUDEPATH *= murmur_ice

  unix {
    QMAKE_CFLAGS *= "-isystem murmur_ice"
    QMAKE_CXXFLAGS *= "-isystem murmur_ice"
  }
}

grpc {
  isEqual(QT_MAJOR_VERSION, 4) {
    error("Murmur's gRPC support requires Qt 5")
  }

  DEFINES *= USE_GRPC
  unix {
  INCLUDEPATH *= murmur_grpc
  }
  win32 {
#  INCLUDEPATH *= $$(MUMBLE_PREFIX)/src/murmur/murmur_grpc
  INCLUDEPATH *= murmur_grpc
  }
  LIBS *= -lmurmur_grpc

  HEADERS *= MurmurGRPCImpl.h
  SOURCES *= MurmurGRPCImpl.cpp

  GRPC_WRAPPER = MurmurRPC.proto
  grpc_wrapper.output = MurmurRPC.proto.Wrapper.cpp
#  win32 {
  win32-msvc* {
#  grpc_wrapper.commands = $${PROTOC} --plugin=$$system(cygpath -w ${DESTDIR}protoc-gen-murmur-grpcwrapper) -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=$$system(cygpath -w $${DESTDIR}protoc-gen-murmur-grpcwrapper) -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=$$system(cygpath -w ${DESTDIR}protoc-gen-murmur-grpcwrapper) -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=$$system(cygpath -w ${DESTDIR_TARGET}protoc-gen-murmur-grpcwrapper) -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=$$system(cygpath -w ${DESTDIR_TARGET}protoc-gen-murmur-grpcwrapper.exe) -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=$$system(cygpath -w ${DESTDIR}protoc-gen-murmur-grpcwrapper.exe) -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=$$system(cygpath -w $${DESTDIR}/protoc-gen-murmur-grpcwrapper.exe) -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=$$system(cygpath -w $${DESTDIR}/protoc-gen-murmur-grpcwrapper) -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=$${DESTDIR}/protoc-gen-murmur-grpcwrapper -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=$$system(cygpath -w $${DESTDIR}/protoc-gen-murmur-grpcwrapper) -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=protoc-gen-murmur-grpcwrapper -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=protoc-gen-murmur-grpcwrapper.exe -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#Appears that exact plugin name we pass it not honored, but only searched for on the path via commandline arg to CreateProcessA()...
#
  grpc_wrapper.commands = $${PROTOC} --plugin=protoc-gen-grpc=$$system(cygpath -w $${DESTDIR}/protoc-gen-murmur-grpcwrapper) -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=protoc-gen-grpc=$$system(cygpath -w protoc-gen-murmur-grpcwrapper) -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
#  grpc_wrapper.commands = $${PROTOC} --plugin=protoc-gen-grpc=$$system(cygpath -w protoc-gen-murmur-grpcwrapper.exe) -I. --murmur-grpcwrapper_out=. MurmurRPC.proto

#    INCLUDEPATH *= $${MUMBLE_PREFIX}/grpc-windows/grpc/include
    INCLUDEPATH *= $${MUMBLE_PREFIX}/grpc/include
	QMAKE_CFLAGS *= -WX-
	#TBD: maybe issues with compiler other than VC, but so far mxe build attempts have failed anyway...
#	LIBS *= -lgrpc++ -lgrpc -lgpr -laddress_sorting -lcrypto -lssl -lcares
#	LIBS *= -lgrpc++ -lgrpc -lgpr -laddress_sorting          -lssl -lcares
#	LIBS *= -lgrpc++ -lgrpc -lgpr -laddress_sorting -lssl -lcrypto -lcares
	LIBS *= -lgrpc++ -lgrpc -lgpr -laddress_sorting                -lcares
#	QMAKE_LIBDIR *= $${MUMBLE_GRPC_PREFIX}/vsprojects/x64/$${MUMBLE_CONFIGURATION}
#	QMAKE_LIBDIR *= $${MUMBLE_GRPC_PREFIX}/bin/grpc/$${MUMBLE_CONFIGURATION}
#	QMAKE_LIBDIR *= $${MUMBLE_GRPC_PREFIX}/bin/
	QMAKE_LIBDIR *= $${MUMBLE_PREFIX} # to ref grpc/bin
	#TBD: Will anything of this sort be needed...
	#QMAKE_LFLAGS *= -Wl,-rpath,$$(MUMBLE_GRPC_PREFIX)/vsprojects/x64/$$(MUMBLE_BUILD_CONFIGURATION)
	#"fatal error C1905: Front end and back end not compatible (must target same processor)."
	#QMAKE_LFLAGS *= /Libpath:$$(MUMBLE_GRPC_PREFIX)/vsprojects/x64/$$(MUMBLE_BUILD_CONFIGURATION)
#	QMAKE_LFLAGS *= /Libpath:$$(MUMBLE_GRPC_PREFIX)/bin/grpc/$$(MUMBLE_BUILD_CONFIGURATION) /NODEFAULTLIB:LIBCMT
#	QMAKE_LFLAGS *= /Libpath:$$(MUMBLE_GRPC_PREFIX)/bin/grpc/$$(MUMBLE_BUILD_CONFIGURATION)
	QMAKE_LFLAGS *= /Libpath:$$(MUMBLE_GRPC_PREFIX)/bin # to ref grpc/bin
	#QMAKE_LFLAGS *= /Libpath:$$(MUMBLE_GRPC_PREFIX)/vsprojects/Release #$$(MUMBLE_BUILD_CONFIGURATION)
#	CONFIG += staticlib
  }
  unix {
  grpc_wrapper.commands = $${PROTOC} --plugin=${DESTDIR}protoc-gen-murmur-grpcwrapper -I. --murmur-grpcwrapper_out=. MurmurRPC.proto
  }
  grpc_wrapper.input = GRPC_WRAPPER
  grpc_wrapper.variable_out =
  QMAKE_EXTRA_COMPILERS += grpc_wrapper

  unix {
    QMAKE_CXXFLAGS *= -std=c++11
    must_pkgconfig(grpc)
    must_pkgconfig(grpc++)
  }
}

bonjour {
  DEFINES *= USE_BONJOUR

  HEADERS *= \
    ../../3rdparty/qqbonjour-src/BonjourRecord.h \
    ../../3rdparty/qqbonjour-src/BonjourServiceRegister.h \
    BonjourServer.h
  SOURCES *= \
    ../../3rdparty/qqbonjour-src/BonjourServiceRegister.cpp \
    BonjourServer.cpp
  INCLUDEPATH *= ../../3rdparty/qqbonjour-src
  win32 {
    INCLUDEPATH *= "$$BONJOUR_PATH/include"
    QMAKE_LIBDIR *= "$$BONJOUR_PATH/lib/win32"
    LIBS *= -lDNSSD
  }
  unix:!macx {
    system($$PKG_CONFIG --exists avahi-compat-libdns_sd avahi-client) {
      must_pkgconfig(avahi-compat-libdns_sd)
      must_pkgconfig(avahi-client)
    } else {
      LIBS *= -ldns_sd
    }
  }
}

# Check for QSslDiffieHellmanParameters availability, and define
# USE_QSSLDIFFIEHELLMANPARAMETERS preprocessor if available.
#
# Can be disabled with no-qssldiffiehellmanparameters.
!CONFIG(no-qssldiffiehellmanparameters):exists($$[QT_INSTALL_HEADERS]/QtNetwork/QSslDiffieHellmanParameters) {
  DEFINES += USE_QSSLDIFFIEHELLMANPARAMETERS
}

include(../../qmake/symbols.pri)
