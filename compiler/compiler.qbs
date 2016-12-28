import qbs 1.0

// @todo compiler:
//  √ turn .cc into .o
//  √ turn .o into libVuoCompiler.a
//    licensetools.pro
//    premiumnode.pro

Product {
	type: 'VuoClangLinkStatic'
	name: 'VuoCompiler'

	Depends { name: 'VuoPrecompiledHeader' }
	Depends { name: 'VuoClang' }
	Depends { name: 'VuoBase' }

	VuoClang.defines: {
		var defines = [
			'LEAP_ROOT="'   + project.sourceDirectory + '/node/vuo.leap/Leap"',
			'SYPHON_ROOT="' + project.sourceDirectory + '/node/vuo.syphon/Syphon"',
		];

		// Pass all the external dependencies (defined in vuo.qbs) to VuoCompiler::Environment::getBuiltInLibrarySearchPaths().
		for (var key in paths)
			defines.push(key.toUpperCase() + '_ROOT="' + paths[key] + '"');

		return defines;
	}

	Group {
		name: 'c++'
		fileTags: ['VuoClangCxxWithPch', 'cpp']
		files: [
			'VuoCompilerPortClass.cc',
			'VuoCompilerPort.cc',
			'VuoCompilerDataClass.cc',
			'VuoCompilerData.cc',
			'VuoCompilerOutputDataClass.cc',
			'VuoCompilerOutputData.cc',
			'VuoCompilerEventPortClass.cc',
			'VuoCompilerEventPort.cc',
			'VuoCompilerOutputEventPortClass.cc',
			'VuoCompilerOutputEventPort.cc',
			'VuoCompilerNodeClass.cc',
			'VuoCompilerNodeArgumentClass.cc',
			'VuoCompilerNodeArgument.cc',
			'VuoCompilerNode.cc',
			'VuoCompilerInstanceDataClass.cc',
			'VuoCompilerInstanceData.cc',
			'VuoCompilerInputDataClass.cc',
			'VuoCompilerInputData.cc',
			'VuoCompilerInputEventPortClass.cc',
			'VuoCompilerInputEventPort.cc',
			'VuoCompilerGraphvizParser.cc',
			'VuoCompilerBitcodeGenerator.cc',
			'VuoCompilerGraph.cc',
			'VuoCompilerTriggerPortClass.cc',
			'VuoCompilerTriggerPort.cc',
			'VuoCompilerChain.cc',
			'VuoCompilerBitcodeParser.cc',
			'VuoCompiler.cc',
			'VuoCompilerCable.cc',
			'VuoCompilerPublishedPort.cc',
			'VuoCompilerPublishedInputPort.cc',
			'VuoCompilerPublishedOutputPort.cc',
			'VuoCompilerCodeGenUtilities.cc',
			'VuoCompilerModule.cc',
			'VuoCompilerType.cc',
			'VuoCompilerGenericType.cc',
			'VuoCompilerComposition.cc',
			'VuoCompilerMakeListNodeClass.cc',
			'VuoCompilerPublishedInputNodeClass.cc',
			'VuoCompilerSpecializedNodeClass.cc',
			'VuoCompilerTargetSet.cc',
			'VuoCompilerDriver.cc',
			'VuoCompilerException.cc',
		]
	}

	Group {
		name: 'headers'
		fileTags: ['hpp']
		files: [
			'VuoCompilerPortClass.hh',
			'VuoCompilerPort.hh',
			'VuoCompilerDataClass.hh',
			'VuoCompilerData.hh',
			'VuoCompilerOutputDataClass.hh',
			'VuoCompilerOutputData.hh',
			'VuoCompilerEventPortClass.hh',
			'VuoCompilerEventPort.hh',
			'VuoCompilerOutputEventPortClass.hh',
			'VuoCompilerOutputEventPort.hh',
			'VuoCompilerNodeClass.hh',
			'VuoCompilerNodeArgumentClass.hh',
			'VuoCompilerNodeArgument.hh',
			'VuoCompilerNode.hh',
			'VuoCompilerInstanceDataClass.hh',
			'VuoCompilerInstanceData.hh',
			'VuoCompilerInputDataClass.hh',
			'VuoCompilerInputData.hh',
			'VuoCompilerInputEventPortClass.hh',
			'VuoCompilerInputEventPort.hh',
			'VuoCompilerGraphvizParser.hh',
			'VuoCompilerBitcodeGenerator.hh',
			'VuoCompilerGraph.hh',
			'VuoCompilerTriggerPortClass.hh',
			'VuoCompilerTriggerPort.hh',
			'VuoCompilerChain.hh',
			'VuoCompilerBitcodeParser.hh',
			'VuoCompiler.hh',
			'VuoCompilerCable.hh',
			'VuoCompilerPublishedPort.hh',
			'VuoCompilerPublishedInputPort.hh',
			'VuoCompilerPublishedOutputPort.hh',
			'VuoCompilerCodeGenUtilities.hh',
			'VuoCompilerModule.hh',
			'VuoCompilerType.hh',
			'VuoCompilerGenericType.hh',
			'VuoCompilerComposition.hh',
			'VuoCompilerMakeListNodeClass.hh',
			'VuoCompilerPublishedInputNodeClass.hh',
			'VuoCompilerSpecializedNodeClass.hh',
			'VuoCompilerTargetSet.hh',
			'VuoCompilerDriver.hh',
			'VuoCompilerException.hh',
		]
	}
}
