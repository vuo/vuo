/**
 * @file
 * vuo-render implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include <getopt.h>

#include <QtSvg/QSvgGenerator>

#include <Vuo/Vuo.h>

#include "VuoRendererCommon.hh"
#include "VuoRendererComposition.hh"

void printHelp(char *argv0)
{
	printf("Creates a visual representation of the specified Vuo composition source code or node class.\n"
		   "\n"
		   "Usage:\n"
		   "  %s [options] composition.vuo    Renders composition.vuo's source code (i.e., what the composition looks like in the Vuo editor).\n"
		   "  %s [options] node.class.name    Renders a single node class.\n"
		   "\n"
		   "Options:\n"
		   "  --help                       Display this information.\n"
		   "  --output <file>              Place the rendered image into <file>.\n"
		   "  --output-format=<format>     <format> can be 'png', 'pdf', or 'svg'. The default is 'png'.\n"
		   "  --draw-bounding-rects        Draws red bounding rectangles around items in the composition canvas.\n"
		   "  --render-missing-as-present  Render missing node classes as though they were present.\n"
		   "  --color-scheme=<mode>        Use either the \"light\" (default) or \"dark\" color scheme.\n"
		   "  --background=<mode>          Render nodes on either a \"transparent\" (default) or \"opaque\" background.\n"
		   "  --scale <factor>             Changes the resolution.  For example, to render a PNG at Retina resolution, use '2'.\n",
		   argv0, argv0);
}

/**
 * Entrypoint.
 */
int main (int argc, char * argv[])
{
	VuoRendererComposition::createAutoreleasePool();

	// https://bugreports.qt-project.org/browse/QTBUG-29197
	qputenv("QT_MAC_DISABLE_FOREGROUND_APPLICATION_TRANSFORM", "1");

	qInstallMessageHandler(VuoRendererCommon::messageHandler);

	// Tell Qt where to find its plugins.
	QApplication::setLibraryPaths(QStringList{
		QString::fromStdString(VuoFileUtilities::getVuoFrameworkPath() + "/../resources/QtPlugins"),                           // In the SDK tree
		QString::fromStdString(VuoFileUtilities::getVuoFrameworkPath() + "/../../bin/Vuo.app/Contents/Frameworks/QtPlugins"),  // In the build tree
	});

	QApplication app(argc, argv);

	bool hasInputFile = false;
	string inputPath;
	VuoCompilerIssues *issues = new VuoCompilerIssues();
	int ret = 0;

	try
	{
		string outputPath = "";
		string outputFormat = "png";
		bool doPrintHelp = false;
		bool doRenderMissingAsPresent = false;
		double scale = 1;
		bool backgroundTransparent = true;
		VuoCompiler compiler;

		static struct option options[] = {
			{"help", no_argument, NULL, 0},
			{"output", required_argument, NULL, 0},
			{"output-format", required_argument, NULL, 0},
			{"render-missing-as-present", no_argument, NULL, 0},
			{"draw-bounding-rects", no_argument, NULL, 0},
			{"scale", required_argument, NULL, 0},
			{"color-scheme", required_argument, NULL, 0},
			{"background", required_argument, NULL, 0},
			{NULL, no_argument, NULL, 0}
		};
		int optionIndex=-1;
		int ret;
		while ((ret = getopt_long(argc, argv, "", options, &optionIndex)) != -1)
		{
			if (ret == '?')
				continue;

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
				case 5:	 // --scale
					scale = atof(optarg);
					break;
				case 6:	 // --color-scheme
					VuoRendererColors::setDark(strcmp(optarg, "dark") == 0);
					break;
				case 7:	 // --background
					backgroundTransparent = strcmp(optarg, "opaque") != 0;
					break;
				default:
					VUserLog("Error: Unknown option %d.", optionIndex);
					break;
			}
		}

		hasInputFile = (optind < argc) && ! doPrintHelp;

		if (doPrintHelp)
			printHelp(argv[0]);
		else
		{
			if (! hasInputFile)
				throw VuoException("no input composition file or node class name");
			inputPath = argv[optind];

			string inputDir, inputFile, inputExtension;
			VuoFileUtilities::splitPath(inputPath, inputDir, inputFile, inputExtension);

			compiler.setCompositionPath(inputPath);

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
				VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionFile(inputPath, &compiler);
				compilerComposition = new VuoCompilerComposition(baseComposition, parser);
				delete parser;
			}
			else if (inputPath == "-")
			{
				string compositionAsString = VuoFileUtilities::readStdinToString();
				VuoCompilerGraphvizParser *parser = VuoCompilerGraphvizParser::newParserFromCompositionString(compositionAsString, &compiler);
				compilerComposition = new VuoCompilerComposition(baseComposition, parser);
				delete parser;
			}
			else
			{
				compilerComposition = new VuoCompilerComposition(baseComposition, NULL);
			}
			VuoRendererComposition *composition = new VuoRendererComposition(baseComposition, doRenderMissingAsPresent);
			if (! (inputExtension == "vuo" || inputPath == "-"))
			{
				VuoCompilerNodeClass *nodeClass;
				if (!(nodeClass = compiler.getNodeClass(inputPath)))
					throw VuoException("input file must be a composition ('.vuo' extension) or a node class name (without a file extension)");

				VuoNode * n = nodeClass->newNode();
				composition->addNode(n);
			}
			QRectF boundingRect = composition->itemsBoundingRect();
			boundingRect.setWidth(boundingRect.width() * scale);
			boundingRect.setHeight(boundingRect.height() * scale);

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
				p->setPageSize(QPageSize(boundingRect.size(), QPageSize::Point, QString(), QPageSize::ExactMatch));
				p->setOutputFormat(QPrinter::PdfFormat);
				p->setOutputFileName(QString::fromUtf8(outputPath.c_str()));
				outputDevice = p;
			}
			else if (outputFormat == "svg")
			{
				QSvgGenerator *p = new QSvgGenerator();
				p->setSize(boundingRect.size().toSize());
				p->setViewBox(boundingRect.translated(-boundingRect.topLeft()));
				p->setFileName(QString::fromStdString(outputPath));
				outputDevice = p;
			}
			else
				throw VuoException("unrecognized option '" + outputFormat + "' for --outputFormat");

			QPainter *painter = new QPainter(outputDevice);
			painter->setRenderHint(QPainter::Antialiasing, true);
			painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
			painter->setRenderHint(QPainter::TextAntialiasing, true);

			painter->scale(scale, scale);

			composition->setBackgroundTransparent(backgroundTransparent);
			composition->render(painter, QRectF(QPoint(),boundingRect.size()), boundingRect);

			if (outputFormat == "png")
			{
				bool successful = ((QImage*)outputDevice)->save(QString::fromUtf8(outputPath.c_str()));
				if (!successful)
					throw VuoException("saving failed");
			}

			delete painter;
			delete outputDevice;
			delete composition;
			delete compilerComposition;
			delete baseComposition;
		}
	}
	catch (VuoCompilerException &e)
	{
		if (issues != e.getIssues())
			issues->append(e.getIssues());

		ret = 1;
	}
	catch (VuoException &e)
	{
		fprintf(stderr, "%s: error: %s\n", hasInputFile ? inputPath.c_str() : argv[0], e.what());
		if (!hasInputFile)
		{
			fprintf(stderr, "\n");
			printHelp(argv[0]);
		}
		fprintf(stderr, "\n");

		ret = 1;
	}

	vector<VuoCompilerIssue> issueList = issues->getList();
	for (vector<VuoCompilerIssue>::iterator i = issueList.begin(); i != issueList.end(); ++i)
		fprintf(stderr, "%s: %s: %s\n\n", hasInputFile ? inputPath.c_str() : argv[0],
		        (*i).getIssueType() == VuoCompilerIssue::Error ? "error" : "warning", (*i).getShortDescription(false).c_str());

	return ret;
}
