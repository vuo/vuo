import qbs 1.0
import qbs.FileInfo
import qbs.Process
import 'VuoCompilerCommand.js' as VuoCompilerCommand

Module {
	property path path: project.paths.llvm + '/bin/clang'

	property stringList flags: [
		'-march=x86-64',
		'-g',
		'-fvisibility-inlines-hidden',
		'-Oz',
		'-fPIC',
		'-mmacosx-version-min=10.7',
		'-isysroot', '/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.10.sdk',
	]

	property stringList includePaths: {
		var includePaths = [
			project.paths.jsonc + '/include',
			project.paths.discount + '/include',
			project.paths.zmq + '/include',
			project.paths.llvm + '/include',
			project.paths.graphviz + '/include',
		];

		if (project.premiumAvailable)
			includePaths.push(project.sourceDirectory + '/licensetools');

		return includePaths;
	}

	property stringList defines: {
		var buildID = qbs.getEnv('BUILD_ID');
		if (!buildID)
			buildID = FileInfo.fileName(project.sourceDirectory);

		var defines = [
			'VUO_VERSION=' + project.version,
			'VUO_VERSION_STRING="' + project.version + '"',
			'VUO_VERSION_AND_BUILD_STRING="' + project.version + '.' + buildID + '"',
			'ENCRYPTED_DATE="oeoz9a.etf"', /// @todo
			'VUO_ROOT="' + project.sourceDirectory + '"',
			'DEBUG',
			'MAC',

			/// @todo remove these (and the #ifdefs in vuo.pch) since they'll always be available now.
			/// @todo alternatively, build separate precompiled headers for the 3 major stages (no LLVM/Qt, LLVM only, LLVM+Qt), since compilation time increases with the size of the precompiled header objects it references.
			'ZMQ',
			'LLVM',
		];

		if (project.premiumAvailable)
		{
			/// @todo consolidate into a single define (including the #ifdefs in various source files).
			defines.push('BASE_PREMIUM_AVAILABLE');
			defines.push('LIBRARY_PREMIUM_AVAILABLE');
			defines.push('PREMIUM_NODE_LOADER_ENABLED');
		}

		return defines;
	}

	property stringList warnings: [
		'documentation',
		'no-c++11-extensions',
		'no-disabled-macro-expansion',
		'no-documentation-deprecated-sync',
		'no-missing-prototypes',
		'no-missing-variable-declarations',
		'no-non-virtual-dtor',
		'no-padded',
		'no-shadow',
		'no-unused-parameter',
		'no-variadic-macros',
		'objc-property-no-attribute',
		'unreachable-code',
	]

	property path precompiledHeaderC:      project.buildDirectory  + '/vuo.pch-c'
	property path precompiledHeaderCxx:    project.buildDirectory  + '/vuo.pch-c++'
	property path precompiledHeaderObjC:   project.buildDirectory  + '/vuo.pch-objective-c'
	property path precompiledHeaderObjCxx: project.buildDirectory  + '/vuo.pch-objective-c++'



	// Compile project-wide precompiled headers.
	Rule {
		id: VuoClangPrecompiledHeaderC
		inputs: 'VuoPrecompiledHeader'
		Artifact {
			fileTags: 'VuoPrecompiledHeaderOutput'
			filePath: product.moduleProperty('VuoClang', 'precompiledHeaderC')
		}
		prepare: {
			return VuoCompilerCommand.generate(product, input, output, {generatePch: 'c'});
		}
	}
	Rule {
		id: VuoClangPrecompiledHeaderCxx
		inputs: 'VuoPrecompiledHeader'
		Artifact {
			fileTags: 'VuoPrecompiledHeaderOutput'
			filePath: product.moduleProperty('VuoClang', 'precompiledHeaderCxx')
		}
		prepare: {
			return VuoCompilerCommand.generate(product, input, output, {generatePch: 'c++'});
		}
	}
	Rule {
		id: VuoClangPrecompiledHeaderObjC
		inputs: 'VuoPrecompiledHeader'
		Artifact {
			fileTags: 'VuoPrecompiledHeaderOutput'
			filePath: product.moduleProperty('VuoClang', 'precompiledHeaderObjC')
		}
		prepare: {
			return VuoCompilerCommand.generate(product, input, output, {generatePch: 'objective-c'});
		}
	}
	Rule {
		id: VuoClangPrecompiledHeaderObjCxx
		inputs: 'VuoPrecompiledHeader'
		Artifact {
			fileTags: 'VuoPrecompiledHeaderOutput'
			filePath: product.moduleProperty('VuoClang', 'precompiledHeaderObjCxx')
		}
		prepare: {
			return VuoCompilerCommand.generate(product, input, output, {generatePch: 'objective-c++'});
		}
	}


	// Link object files into a static library.
	Rule {
	    id: VuoClangLinkStatic
	    inputs: 'VuoClangObject'
		multiplex: true

	    Artifact {
	        fileTags: ['VuoClangLinkStatic']
	        filePath: 'lib' + product.name + '.a'
	    }

	    prepare: {
			var commands = [];
			
		    var cmd = new Command('/bin/rm', ['-f', output.filePath]);
		    cmd.silent = true;
		    cmd.highlight = 'linker';
			commands.push(cmd)

			var args = ['-cq', output.filePath];
            for (var i = 0; i < inputs.VuoClangObject.length; ++i)
                args.push(inputs.VuoClangObject[i].filePath);
		    var cmd = new Command('/usr/bin/ar', args);
		    cmd.description = 'archiving ' + output.fileName;
		    cmd.highlight = 'linker';
			commands.push(cmd)
				
			return commands;
	    }
	}


	// Compile C / C++ / Objective-C / Objective-C++ source files, all using the project-wide precompiled headers.
	Rule {
	    id: VuoClangCWithPch
	    inputs: 'VuoClangCWithPch'
		explicitlyDependsOn: 'VuoPrecompiledHeaderOutput'

	    Artifact {
	        fileTags: ['VuoClangObject']
	        filePath: FileInfo.completeBaseName(input.fileName) + '.o'
	    }

	    prepare: {
			return VuoCompilerCommand.generate(product, input, output, {usePch: 'c'});
	    }
	}
	Rule {
		id: VuoClangCxxWithPch
		inputs: 'VuoClangCxxWithPch'
		explicitlyDependsOn: 'VuoPrecompiledHeaderOutput'

		Artifact {
			fileTags: ['VuoClangObject']
			filePath: FileInfo.completeBaseName(input.fileName) + '.o'
		}

		prepare: {
			return VuoCompilerCommand.generate(product, input, output, {usePch: 'c++'});
		}
	}
	Rule {
	    id: VuoClangObjCWithPch
	    inputs: 'VuoClangObjCWithPch'
		explicitlyDependsOn: 'VuoPrecompiledHeaderOutput'

	    Artifact {
	        fileTags: ['VuoClangObject']
	        filePath: FileInfo.completeBaseName(input.fileName) + '.o'
	    }

	    prepare: {
			return VuoCompilerCommand.generate(product, input, output, {usePch: 'objective-c'});
	    }
	}
	Rule {
	    id: VuoClangObjCxxWithPch
	    inputs: 'VuoClangObjCxxWithPch'
		explicitlyDependsOn: 'VuoPrecompiledHeaderOutput'

	    Artifact {
	        fileTags: ['VuoClangObject']
	        filePath: FileInfo.completeBaseName(input.fileName) + '.o'
	    }

	    prepare: {
			return VuoCompilerCommand.generate(product, input, output, {usePch: 'objective-c++'});
	    }
	}
}
