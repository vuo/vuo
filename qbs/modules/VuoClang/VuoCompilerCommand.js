/**
 * `params` can contain:
 *    - `usePch: 'objective-c++'`
 *    - `generatePch: 'objective-c++'`
 */
function generate(product, input, output, params)
{
    var args = [];

	if (params.generatePch)
	{
		args.push('-x'); args.push(params.generatePch + '-header');
	}

	args.push('-c');
    args.push(input.filePath);
    args.push('-o'); args.push(output.filePath);

	if (params.usePch)
	{
		// Use project-wide precompiled header
		args.push('-Xclang'); args.push('-include-pch');
		var variable;
		if (params.usePch == 'c')
			variable = 'precompiledHeaderC';
		else if (params.usePch == 'c++')
			variable = 'precompiledHeaderCxx';
		else if (params.usePch == 'objective-c')
			variable = 'precompiledHeaderObjC';
		else if (params.usePch == 'objective-c++')
			variable = 'precompiledHeaderObjCxx';
		args.push('-Xclang'); args.push(product.moduleProperty('VuoClang', variable));
	}

	args = args.concat(product.moduleProperties('VuoClang', 'flags'));

    var includePaths = product.moduleProperties('VuoClang', 'includePaths');
    for (i in includePaths)
        args.push('-I' + includePaths[i]);

    var defines = product.moduleProperties('VuoClang', 'defines');
    for (i in defines)
        args.push('-D' + defines[i]);

    var warnings = product.moduleProperties('VuoClang', 'warnings');
    for (i in warnings)
        args.push('-W' + warnings[i]);

    var cmd = new Command(product.moduleProperty('VuoClang', 'path'), args);
    cmd.description = 'compiling ' + input.fileName;
    cmd.highlight = 'compiler';
    return cmd;
}
