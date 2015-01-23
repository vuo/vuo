/**
 * @file
 * VuoCompilerNode interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERNODE_H
#define VUOCOMPILERNODE_H

#include "VuoBaseDetail.hh"

#include "VuoNode.hh"

#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerPort.hh"
#include "VuoCompilerInputEventPort.hh"
#include "VuoCompilerInstanceData.hh"

/**
 * The compiler detail class for @c VuoNode.
 */
class VuoCompilerNode : public VuoBaseDetail<VuoNode>
{
private:
	VuoCompilerInstanceData *instanceData;
	string graphvizIdentifier;  ///< The identifier that will appear in .vuo (Graphviz dot format) files. Defaults to the suggested Graphviz identifier prefix.
	static const string RuntimeSuffix;

	CallInst * generateFunctionCall(Function *functionSrc, Module *module, BasicBlock *block);
	bool isArgumentInFunction(VuoCompilerNodeArgument *argument, Function *function);
	size_t getArgumentIndexInFunction(VuoCompilerNodeArgument *argument, Function *function);
	Value * generateReceivedEventCondition(BasicBlock *block, vector<VuoPort *> selectedInputPorts);
	string getGraphvizDeclarationWithOptions(bool shouldUsePlaceholders, bool shouldPrintPosition, double xPositionOffset, double yPositionOffset);
	string getSerializedFormatString(void);

public:
	VuoCompilerNode(VuoNode *baseNode);
	void generateAllocation(Module *module);
	void generateEventFunctionCall(Module *module, Function *function, BasicBlock *initialBlock, BasicBlock *finalBlock);
	void generateInitFunctionCall(Module *module, BasicBlock *block);
	void generateFiniFunctionCall(Module *module, BasicBlock *block);
	void generateCallbackStartFunctionCall(Module *module, BasicBlock *block);
	void generateCallbackUpdateFunctionCall(Module *module, BasicBlock *block);
	void generateCallbackStopFunctionCall(Module *module, BasicBlock *block);
	Value * generateReceivedEventCondition(BasicBlock *block);
	void generatePushedReset(BasicBlock *block);
	void generateFinalization(Module *module, BasicBlock *block, bool isInput);
	VuoCompilerInstanceData * getInstanceData(void);
	string getIdentifier(void);
	string getGraphvizIdentifierPrefix(void);
	void setGraphvizIdentifier(string graphvizIdentifier);
	string getGraphvizIdentifier(void);
	string getGraphvizDeclaration(bool shouldPrintPosition = false, double xPositionOffset = 0, double yPositionOffset = 0);
	Value * generateSerializedString(Module *module, BasicBlock *block);
	void generateUnserialization(Module *module, Function *function, BasicBlock *&block, Value *graphValue);
};

#endif
