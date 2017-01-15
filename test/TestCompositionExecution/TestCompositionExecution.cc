/**
 * @file
 * TestCompositionExecution implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "TestCompositionExecution.hh"

#include <fcntl.h>
#include <libgen.h>
#include <malloc/malloc.h>
#include <sstream>

/**
 * Sets up a compiler, adding both regular and for-testing-only node classes to the search path.
 */
VuoCompiler * TestCompositionExecution::initCompiler(void)
{
	VuoCompiler *c = new VuoCompiler();
	c->addModuleSearchPath("node-" + QDir::current().dirName().toStdString());
	return c;
}

/**
 * Returns the directory containing the test compositions.
 */
QDir TestCompositionExecution::getCompositionDir(void)
{
	QDir compositionDir = QDir::current();
	compositionDir.cd("composition");
	return compositionDir;
}

/**
 * Returns the path of the test composition (which should include the file extension).
 */
string TestCompositionExecution::getCompositionPath(string compositionFileName)
{
	return getCompositionDir().filePath(QString(compositionFileName.c_str())).toStdString();
}

/**
 * Returns a string containing a composition that consists of an instance of a node class
 * with all its input and output ports published.
 */
string TestCompositionExecution::wrapNodeInComposition(VuoCompilerNodeClass *nodeClass, VuoCompiler *compiler)
{
	ostringstream oss;
	oss << "digraph G {" << endl;

	VuoNode *node = compiler->createNode(nodeClass);
	oss << node->getCompiler()->getGraphvizDeclaration() << endl;
	string nodeIdentifier = node->getCompiler()->getGraphvizIdentifier();

	oss << "PublishedInputs [type=\"vuo.in\" label=\"PublishedInputs";
	foreach (VuoPort *port, node->getInputPorts())
	{
		string portName = port->getClass()->getName();
		oss << "|<" << portName << ">" << portName << "\\r";
	}
	oss << "\"";
	foreach (VuoPort *port, node->getInputPorts())
	{
		VuoCompilerInputData *data = dynamic_cast<VuoCompilerInputEventPort *>( port->getCompiler() )->getData();
		if (data)
		{
			string initialValue = VuoStringUtilities::transcodeToGraphvizIdentifier( data->getInitialValue() );
			string portName = port->getClass()->getName();
			oss << " _" << portName << "=\"" << initialValue << "\"";
		}
	}
	oss << "];" << endl;

	oss << "PublishedOutputs [type=\"vuo.out\" label=\"PublishedOutputs";
	foreach (VuoPort *port, node->getOutputPorts())
	{
		string portName = port->getClass()->getName();
		oss << "|<" << portName << ">" << portName << "\\l";
	}
	oss << "\"];" << endl;

	foreach (VuoPort *port, node->getInputPorts())
	{
		string portName = port->getClass()->getName();
		oss << "PublishedInputs:" << portName << " -> " << nodeIdentifier << ":" << portName << ";" << endl;
	}

	foreach (VuoPort *port, node->getOutputPorts())
	{
		string portName = port->getClass()->getName();
		oss << nodeIdentifier << ":" << portName << " -> " << "PublishedOutputs:" << portName << ";" << endl;
	}

	oss << "}" << endl;

	return oss.str();
}

/**
 * Prints the amount of memory dynamically allocated by the current process.
 */
void TestCompositionExecution::printMemoryUsage(string label)
{
	// http://www.opensource.apple.com/source/Libc/Libc-763.12/include/malloc/malloc.h
	malloc_statistics_t mzs;
	malloc_zone_statistics(NULL,&mzs);

	printf("=== Memory usage %s ===\n", label.c_str());
	printf("%lu bytes used\n", mzs.size_in_use);
	printf("%lu bytes used at high-water mark\n", mzs.max_size_in_use);
}
