/**
 * @file
 * VuoCompilerNode interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma once

#include "VuoBaseDetail.hh"
#include "VuoNode.hh"

class VuoCompilerEventPort;
class VuoCompilerConstantStringCache;
class VuoCompilerInstanceData;
class VuoCompilerNodeArgument;
class VuoCompilerType;
class VuoNode;
class VuoPort;

/**
 * The compiler detail class for @c VuoNode.
 */
class VuoCompilerNode : public VuoBaseDetail<VuoNode>
{
private:
	VuoCompilerInstanceData *instanceData;
	string graphvizIdentifier;  ///< The identifier that will appear in .vuo (Graphviz dot format) files. Defaults to the suggested Graphviz identifier prefix.
	VuoCompilerConstantStringCache *constantStrings;
	size_t indexInOrderedNodes;

	CallInst * generateFunctionCall(Function *functionSrc, Module *module, BasicBlock *block, Value *compositionStateValue, Value *nodeContextValue,
									const map<VuoCompilerEventPort *, Value *> &portContextForEventPort = (map<VuoCompilerEventPort *, Value *>()));
	bool isArgumentInFunction(VuoCompilerNodeArgument *argument, Function *function);
	size_t getArgumentIndexInFunction(VuoCompilerNodeArgument *argument, Function *function);

public:
	VuoCompilerNode(VuoNode *baseNode);
	void setIndexInOrderedNodes(size_t indexInOrderedNodes);
	size_t getIndexInOrderedNodes(void);
	void setConstantStringCache(VuoCompilerConstantStringCache *constantStrings);
	Value * generateIdentifierValue(Module *module);
	Value * generateSubcompositionIdentifierValue(Module *module, BasicBlock *block, Value *compositionIdentifierValue);
	Value * generateGetContext(Module *module, BasicBlock *block, Value *compositionStateValue);
	void generateAddMetadata(Module *module, BasicBlock *block, Value *compositionStateValue, const vector<VuoCompilerType *> &orderedTypes);
	Value * generateCreateContext(Module *module, BasicBlock *block, Value *compositionStateValue);
	void generateDestroyContext(Module *module, BasicBlock *block, Value *compositionStateValue, Value *nodeContextValue);
	void generateEventFunctionCall(Module *module, Function *function, BasicBlock *&currentBlock, Value *compositionStateValue);
	void generateInitFunctionCall(Module *module, BasicBlock *block, Value *compositionStateValue);
	void generateFiniFunctionCall(Module *module, BasicBlock *block, Value *compositionStateValue);
	void generateCallbackStartFunctionCall(Module *module, BasicBlock *block, Value *compositionStateValue);
	void generateCallbackUpdateFunctionCall(Module *module, BasicBlock *block, Value *compositionStateValue);
	void generateCallbackStopFunctionCall(Module *module, BasicBlock *block, Value *compositionStateValue);
	Value * generateReceivedEventCondition(Module *module, BasicBlock *block, Value *nodeContextValue);
	Value * generateReceivedEventCondition(Module *module, BasicBlock *block, Value *nodeContextValue, vector<VuoPort *> selectedInputPorts,
										   const map<VuoCompilerEventPort *, Value *> &portContextForEventPort = (map<VuoCompilerEventPort *, Value *>()));
	VuoCompilerInstanceData * getInstanceData(void);
	string getIdentifier(void);
	string getGraphvizIdentifierPrefix(void);
	void setGraphvizIdentifier(string graphvizIdentifier);
	string getGraphvizIdentifier(void);
	string getGraphvizDeclaration(bool shouldPrintPosition = false, double xPositionOffset = 0, double yPositionOffset = 0);
};
