import qbs 1.0

Product {
	type: 'VuoPrecompiledHeaderOutput'
	name: 'VuoPrecompiledHeader'

	Depends { name: 'VuoClang' }

	VuoClang.includePaths: base.concat([
		project.sourceDirectory + '/library',
	])

	Group {
		name: 'headers'
		fileTags: 'VuoPrecompiledHeader'

		/// @todo move vuo.pch into this folder
		files: project.sourceDirectory + '/vuo.pch'
	}
}
