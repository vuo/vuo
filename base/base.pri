SOURCES += \
	../runtime/VuoLog.cc \
	VuoBase.cc \
	VuoBaseDetail.cc \
	VuoNodeClass.cc \
	VuoNode.cc \
	VuoNodeSet.cc \
	VuoPort.cc \
	VuoPortClass.cc \
	VuoProtocol.cc \
	VuoCable.cc \
	VuoRunner.cc \
	VuoFileUtilities.cc \
	VuoTelemetry.c \
	VuoStringUtilities.cc \
	VuoPublishedPort.cc \
	VuoModule.cc \
	VuoType.cc \
	VuoGenericType.cc \
	VuoTimeUtilities.cc \
	VuoCompositionStub.c \
	VuoComposition.cc \
	miniz.c

OBJECTIVE_SOURCES += \
	../runtime/VuoEventLoop.m \
	VuoFileUtilitiesCocoa.mm \
	VuoMovieExporter.mm \
	VuoRunnerCocoa.mm \
	VuoRunnerCocoa+Conversion.mm

HEADERS += \
	VuoBase.hh \
	VuoBaseDetail.hh \
	VuoNodeClass.hh \
	VuoNode.hh \
	VuoNodeSet.hh \
	VuoPort.hh \
	VuoPortClass.hh \
	VuoProtocol.hh \
	VuoCable.hh \
	VuoRunner.hh \
	VuoRunnerCocoa.h \
	VuoRunnerCocoa+Conversion.hh \
	VuoFileUtilities.hh \
	VuoFileUtilitiesCocoa.hh \
	VuoTelemetry.h \
	VuoStringUtilities.hh \
	VuoPublishedPort.hh \
	VuoModule.hh \
	VuoMovieExporter.hh \
	VuoType.hh \
	VuoGenericType.hh \
	VuoTimeUtilities.hh \
	VuoComposition.hh \
	miniz.h

exists($$ROOT/base/premium) {
	DEFINES += BASE_PREMIUM_AVAILABLE
	HEADERS += premium/VuoMovieExporterPremium.h
}

BASE_STUB_SOURCES += \
	VuoCompositionStub.c
