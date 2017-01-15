/**
 * @file
 * VuoCompilerNode interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERNODE_H
#define VUOCOMPILERNODE_H

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

	CallInst * generateFunctionCall(Function *functionSrc, Module *module, BasicBlock *block, Value *nodeIdentifierValue, Value *nodeContextValue,
									const map<VuoCompilerEventPort *, Value *> &portContextForEventPort = (map<VuoCompilerEventPort *, Value *>()));
	bool isArgumentInFunction(VuoCompilerNodeArgument *argument, Function *function);
	size_t getArgumentIndexInFunction(VuoCompilerNodeArgument *argument, Function *function);
	string getGraphvizDeclarationWithOptions(bool shouldUsePlaceholders, bool shouldPrintPosition, double xPositionOffset, double yPositionOffset);
	string getSerializedFormatString(void);

public:
	VuoCompilerNode(VuoNode *baseNode);
	void setConstantStringCache(VuoCompilerConstantStringCache *constantStrings);
	Value * generateIdentifierValue(Module *module, BasicBlock *block, Value *compositionIdentifierValue);
	Value * generateGetContext(Module *module, BasicBlock *block, Value *compositionIdentifierValue);
	Value * generateContextInit(Module *module, BasicBlock *block, Value *compositionIdentifierValue, unsigned long nodeIndex, const vector<VuoCompilerType *> &orderedTypes);
	void generateContextFini(Module *module, BasicBlock *block, BasicBlock *finiBlock, Value *compositionIdentifierValue, Value *nodeIdentifierValue, Value *nodeContextValue);
	void generateEventFunctionCall(Module *module, Function *function, BasicBlock *&currentBlock, Value *nodeIdentifierValue);
	void generateInitFunctionCall(Module *module, BasicBlock *block, Value *compositionIdentifierValue);
	void generateFiniFunctionCall(Module *module, BasicBlock *block, Value *compositionIdentifierValue);
	void generateCallbackStartFunctionCall(Module *module, BasicBlock *block, Value *compositionIdentifierValue);
	void generateCallbackUpdateFunctionCall(Module *module, BasicBlock *block, Value *compositionIdentifierValue);
	void generateCallbackStopFunctionCall(Module *module, BasicBlock *block, Value *compositionIdentifierValue);
	Value * generateReceivedEventCondition(Module *module, BasicBlock *block, Value *nodeContextValue);
	Value * generateReceivedEventCondition(Module *module, BasicBlock *block, Value *nodeContextValue, vector<VuoPort *> selectedInputPorts,
										   const map<VuoCompilerEventPort *, Value *> &portContextForEventPort = (map<VuoCompilerEventPort *, Value *>()));
	VuoCompilerInstanceData * getInstanceData(void);
	string getIdentifier(void);
	string getGraphvizIdentifierPrefix(void);
	void setGraphvizIdentifier(string graphvizIdentifier);
	string getGraphvizIdentifier(void);
	string getGraphvizDeclaration(bool shouldPrintPosition = false, double xPositionOffset = 0, double yPositionOffset = 0);
	Value * generateSerializedString(Module *module, BasicBlock *block, Value *compositionIdentifierValue);
	void generateUnserialization(Module *module, Function *function, BasicBlock *&block, Value *compositionIdentifierValue, Value *graphValue);
};

#endif
