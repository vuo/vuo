/**
 * @file
 * vuo-export implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <getopt.h>
#include <Vuo/Vuo.h>

#include "VuoRendererComposition.hh"

int main (int argc, char * argv[])
{
	VuoRendererComposition::createAutoreleasePool();

	// https://bugreports.qt-project.org/browse/QTBUG-29197
	qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");

	// Tell Qt where to find its plugins.
	QApplication::setLibraryPaths(QStringList((VuoCompiler::getVuoFrameworkPath().str() + "/../resources/QtPlugins").c_str()));

	QApplication app(argc, argv);

	string outputPath = "";
	bool doListNodeClasses = false;
	string listNodeClassesOption = "";
	string target = "";
	bool doPrintHelp = false;
	bool isVerbose = false;
	vector<char *> librarySearchPaths;
	vector<char *> frameworkSearchPaths;
	string inputPath;
	VuoCompiler compiler;

	static struct option options[] = {
		{"help", no_argument, NULL, 0},
		{"output", required_argument, NULL, 0},
		{"list-node-classes", optional_argument, NULL, 0},
		{"target", required_argument, NULL, 0},
		{"library-search-path", required_argument, NULL, 0},
		{"framework-search-path", required_argument, NULL, 0},
		{"verbose", no_argument, NULL, 0},
		{NULL, no_argument, NULL, 0}
	};
	int optionIndex=-1;
	while((getopt_long(argc, argv, "", options, &optionIndex)) != -1)
	{
		switch(optionIndex)
		{
			case 0:  // --help
				doPrintHelp = true;
				break;
			case 1:	 // --output
				outputPath = optarg;
				break;
			case 2:  // --list-node-classes
				doListNodeClasses = true;
				if (optarg)
					listNodeClassesOption = optarg;
				break;
			case 3:  // --target
				target = optarg;
				break;
			case 4:  // --library-search-path
				librarySearchPaths.push_back(optarg);
				break;
			case 5:  // --framework-search-path
				frameworkSearchPaths.push_back(optarg);
				break;
			case 6:  // --verbose
				isVerbose = true;
				compiler.setVerbose(true);
				break;
		}
	}

	bool hasInputFile = (optind < argc);

	for (vector<char *>::iterator i = librarySearchPaths.begin(); i != librarySearchPaths.end(); ++i)
		compiler.addLibrarySearchPath(*i);

	for (vector<char *>::iterator i = frameworkSearchPaths.begin(); i != frameworkSearchPaths.end(); ++i)
		compiler.addFrameworkSearchPath(*i);

	if (doPrintHelp)
	{
		printf("Usage: %s [options] composition-file.vuo\n"
			   "Options:\n"
			   "  --help                         Display this information.\n"
			   "  --list-node-classes[=<arg>]    Display a list of all loaded node classes. <arg> can be 'path' or 'dot'.\n"
			   "  --output <file>                Place the compiled code into <file>.\n"
			   "  --target                       Target the given architecture, vendor, and OS (e.g. 'x86_64-apple-macosx10.6.0').\n"
			   "  --library-search-path <dir>    Search for libraries in <dir>. This option may be specified more than once.\n"
			   "  --framework-search-path <dir>  Search for Mac OS X frameworks in <dir>. This option may be specified more than once.\n"
			   "  --verbose                      Output diagnostic information.\n",
			   argv[0]);
	}
	else if (doListNodeClasses)
	{
		if (listNodeClassesOption == "" || listNodeClassesOption == "path" || listNodeClassesOption == "dot")
			compiler.listNodeClasses(listNodeClassesOption);
		else
		{
			fprintf(stderr, "%s: unrecognized option '%s' for --list-node-classes\n", argv[0], listNodeClassesOption.c_str());
			return 1;
		}
	}
	else if (isVerbose && ! hasInputFile)
	{
		compiler.print();
	}
	else
	{
		if (! hasInputFile)
		{
			fprintf(stderr, "%s: no input file\n", argv[0]);
			return 1;
		}
		inputPath = argv[optind];

		string inputDir, inputFile, inputExtension;
		VuoFileUtilities::splitPath(inputPath, inputDir, inputFile, inputExtension);

		if (outputPath.empty())
			outputPath = inputDir + inputFile + ".app";

		if (! target.empty())
			compiler.setTarget(target);

		VuoCompilerGraphvizParser *parser = new VuoCompilerGraphvizParser(inputPath, &compiler);
		VuoComposition *baseComposition = new VuoComposition();
		VuoCompilerComposition *compilerComposition = new VuoCompilerComposition(baseComposition, parser);
		VuoRendererComposition *rendererComposition = new VuoRendererComposition(baseComposition);

		string exportErrString;
		rendererComposition->exportApp(outputPath.c_str(), &compiler, exportErrString);

		delete parser;
		delete compilerComposition;
		delete rendererComposition;
		delete baseComposition;
	}

	return 0;
}
