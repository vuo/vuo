/**
 * @file
 * vuo-render implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include <getopt.h>
#include <Vuo/Vuo.h>

#include "VuoRendererComposition.hh"

/**
 * Entrypoint.
 */
int main (int argc, char * argv[])
{
	VuoRendererComposition::createAutoreleasePool();

	// https://bugreports.qt-project.org/browse/QTBUG-29197
	qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");

	// Tell Qt where to find its plugins.
	QApplication::setLibraryPaths(QStringList((VuoCompiler::getVuoFrameworkPath().str() + "/../resources/QtPlugins").c_str()));

	QApplication app(argc, argv);

	string outputPath = "";
	string outputFormat = "png";
	bool doPrintHelp = false;
	bool doRenderMissingAsPresent = false;
	string inputPath;
	VuoCompiler compiler;

	static struct option options[] = {
		{"help", no_argument, NULL, 0},
		{"output", required_argument, NULL, 0},
		{"output-format", required_argument, NULL, 0},
		{"render-missing-as-present", no_argument, NULL, 0},
		{"draw-bounding-rects", no_argument, NULL, 0},
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
			case 2:	 // --output-format
				outputFormat = optarg;
				break;
			case 3:	 // --render-missing-as-present
				doRenderMissingAsPresent = true;
				break;
			case 4:	 // --draw-bounding-rects
				VuoRendererItem::setDrawBoundingRects(true);
				break;
		}
	}

	if (doPrintHelp)
	{
		printf("Usage: %s [options] file\n"
			   "Options:\n"
			   "  --help                       Display this information.\n"
			   "  --output <file>              Place the rendered image into <file>.\n"
			   "  --output-format=<format>     <format> can be 'png' or 'pdf'.\n"
			   "  --draw-bounding-rects        Draws red bounding rectangles around items in the composition canvas.\n"
			   "  --render-missing-as-present  Render missing node classes as though they were present.\n",
			   argv[0]);
	}
	else
	{
		if (optind == argc)
		{
			fprintf(stderr, "%s: no input file\n", argv[0]);
			return 1;
		}
		inputPath = argv[optind];

		string inputDir, inputFile, inputExtension;
		VuoFileUtilities::splitPath(inputPath, inputDir, inputFile, inputExtension);

		if (outputPath.empty())
		{
			if (inputExtension == "vuo")
				outputPath = inputDir + inputFile + "." + outputFormat;
			else
				outputPath = inputDir + inputPath + "." + outputFormat;
		}

		VuoComposition *baseComposition = new VuoComposition();
		VuoCompilerComposition *compilerComposition;
		if (inputExtension == "vuo")
		{
			VuoCompilerGraphvizParser *parser = new VuoCompilerGraphvizParser(inputPath, &compiler);
			compilerComposition = new VuoCompilerComposition(baseComposition, parser);
			delete parser;
		}
		else if (inputPath == "-")
		{
			VuoCompilerGraphvizParser *parser = new VuoCompilerGraphvizParser(stdin, &compiler);
			compilerComposition = new VuoCompilerComposition(baseComposition, parser);
			delete parser;
		}
		else
		{
			compilerComposition = new VuoCompilerComposition(baseComposition, NULL);
		}
		VuoRendererComposition *composition = new VuoRendererComposition(baseComposition, doRenderMissingAsPresent);
		if (! (inputExtension == "vuo" || inputExtension == "-"))
		{
			VuoCompilerNodeClass *nodeClass;
			if (!(nodeClass = compiler.getNodeClass(inputPath)))
			{
				fprintf(stderr, "%s: input file '%s' must be a composition ('.vuo' extension) or a node class name (without a file extension)\n", argv[0], inputPath.c_str());
				return 1;
			}

			VuoNode * n = nodeClass->newNode();
			composition->addNode(n);
		}
		QRectF boundingRect = composition->itemsBoundingRect();

		QPaintDevice *outputDevice = NULL;
		if (outputFormat == "png")
		{
			QImage *i = new QImage(boundingRect.width(), boundingRect.height(), QImage::Format_ARGB32);
			i->fill(QColor(0,0,0,0));
			outputDevice = i;
		}
		else if (outputFormat == "pdf")
		{
			QPrinter *p = new QPrinter();
			p->setFullPage(true);
			p->setPaperSize(boundingRect.size(), QPrinter::DevicePixel);
			p->setOutputFormat(QPrinter::PdfFormat);
			p->setOutputFileName(QString::fromUtf8(outputPath.c_str()));
			outputDevice = p;
		}
		else
		{
			fprintf(stderr, "%s: unrecognized option '%s' for --outputFormat\n", argv[0], outputFormat.c_str());
			return 1;
		}

		QPainter *painter = new QPainter(outputDevice);
		painter->setRenderHint(QPainter::Antialiasing, true);
		painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
		painter->setRenderHint(QPainter::TextAntialiasing, true);

		composition->setBackgroundTransparent(true);
		composition->render(painter, QRectF(QPoint(),boundingRect.size()), boundingRect);

		if (outputFormat == "png")
			((QImage*)outputDevice)->save(QString::fromUtf8(outputPath.c_str()));

		delete painter;
		delete outputDevice;
		delete composition;
		delete compilerComposition;
		delete baseComposition;
	}

	return 0;
}
