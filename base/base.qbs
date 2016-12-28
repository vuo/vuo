import qbs 1.0

// @todo base:
//  √ list .h, .hh, premium/.h
//  √ define BASE_PREMIUM_AVAILABLE if available
//  √ turn .c, .cc, .m, .mm into .o
//  √ turn .o into libVuoBase.a
//    turn VuoCompositionStub.c into libVuoCompositionStub.dylib
//    generateFrameworkHeader.pl
//    generateCoreTypesHeader.sh — coreTypes.h is used by module.h, but the coreTypesStringify.h/hh headers are no longer used anywhere

/*
BASE_STUB_SOURCES += \
	VuoCompositionStub.c
*/

Product {
	type: 'VuoClangLinkStatic'
	name: 'VuoBase'

	Depends { name: 'VuoPrecompiledHeader' }
	Depends { name: 'VuoClang' }
	Export {
		Depends { name: 'VuoClang' }
		VuoClang.includePaths: base.concat([project.sourceDirectory + '/base'])
	}

	VuoClang.includePaths: base.concat([
		project.sourceDirectory + '/base',
		project.sourceDirectory + '/library',
		project.sourceDirectory + '/node/vuo.font',
		project.sourceDirectory + '/runtime',
		project.sourceDirectory + '/type',
		project.sourceDirectory + '/type/list',
	])

	Group {
		name: 'c'
		fileTags: ['VuoClangCWithPch', 'c']
		files: [
			'VuoTelemetry.c',
			'VuoCompositionStub.c',
			'miniz.c',
		]
	}

	Group {
		name: 'c++'
		fileTags: ['VuoClangCxxWithPch', 'cpp']
		files: [
			project.sourceDirectory + '/runtime/VuoLog.cc',
			'VuoBase.cc',
			'VuoBaseDetail.cc',
			'VuoNodeClass.cc',
			'VuoNode.cc',
			'VuoNodeSet.cc',
			'VuoPort.cc',
			'VuoPortClass.cc',
			'VuoProtocol.cc',
			'VuoCable.cc',
			'VuoRunner.cc',
			'VuoFileUtilities.cc',
			'VuoStringUtilities.cc',
			'VuoPublishedPort.cc',
			'VuoModule.cc',
			'VuoType.cc',
			'VuoGenericType.cc',
			'VuoTimeUtilities.cc',
			'VuoComposition.cc',
		]
	}

	Group {
		name: 'objective-c'
		fileTags: ['VuoClangObjCWithPch', 'objc']
		files: [
			project.sourceDirectory + '/runtime/VuoEventLoop.m',
		]
	}

	Group {
		name: 'objective-c++'
		fileTags: ['VuoClangObjCxxWithPch', 'objcpp']
		files: [
			'VuoMovieExporter.mm',
			'VuoRunnerCocoa.mm',
			'VuoRunnerCocoa+Conversion.mm',
		]
	}

	Group {
		name: 'headers'
		fileTags: ['hpp']
		files: [
			'VuoBase.hh',
			'VuoBaseDetail.hh',
			'VuoNodeClass.hh',
			'VuoNode.hh',
			'VuoNodeSet.hh',
			'VuoPort.hh',
			'VuoPortClass.hh',
			'VuoProtocol.hh',
			'VuoCable.hh',
			'VuoRunner.hh',
			'VuoRunnerCocoa.h',
			'VuoRunnerCocoa+Conversion.hh',
			'VuoFileUtilities.hh',
			'VuoTelemetry.h',
			'VuoStringUtilities.hh',
			'VuoPublishedPort.hh',
			'VuoModule.hh',
			'VuoMovieExporter.hh',
			'VuoType.hh',
			'VuoGenericType.hh',
			'VuoTimeUtilities.hh',
			'VuoComposition.hh',
			'miniz.h',
		]
	}

	Group {
		name: 'premium'
		condition: project.premiumAvailable
		fileTags: ['hpp']
		files: [
			'premium/VuoMovieExporterPremium.h',
		]
	}
}
