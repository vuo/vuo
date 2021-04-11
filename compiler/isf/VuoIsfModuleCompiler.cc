/**
 * @file
 * VuoIsfModuleCompiler implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoIsfModuleCompiler.hh"
#include <sstream>

/**
 * Registers this compiler.
 */
void __attribute__((constructor)) VuoIsfModuleCompiler::init()
{
	VuoModuleCompiler::registerModuleCompiler("isf", &VuoIsfModuleCompiler::newModuleCompiler);
}

/**
 * Constructs a module compiler, without yet checking if the provided shader is valid or attempting to compile it.
 *
 * Does not keep a reference to @a sourceFile.
 */
VuoIsfModuleCompiler::VuoIsfModuleCompiler(const string &moduleKey, VuoFileUtilities::File *sourceFile)
{
	this->moduleKey = moduleKey;
	this->shaderFile = new VuoShaderFile(*sourceFile);
}

/**
 * Returns a new VuoIsfModuleCompiler instance if @a sourcePath has an ISF file extension, otherwise null.
 */
VuoModuleCompiler *VuoIsfModuleCompiler::newModuleCompiler(const string &moduleKey, VuoFileUtilities::File *sourceFile)
{
	string ext = sourceFile->extension();
	if (VuoFileUtilities::isIsfSourceExtension(ext))
		return new VuoIsfModuleCompiler(moduleKey, sourceFile);

	return nullptr;
}

/**
 * Overrides the fragment shader, using @a sourceCode instead of the file contents.
 */
void VuoIsfModuleCompiler::overrideSourceCode(const string &sourceCode, VuoFileUtilities::File *sourceFile)
{
	delete shaderFile;
	shaderFile = new VuoShaderFile(*sourceFile, sourceCode);
}

/**
 * Compiles the shader to LLVM bitcode that can be loaded as a Vuo node class.
 *
 * If there are errors, they are reported in @a issues and the return value is null.
 */
Module * VuoIsfModuleCompiler::compile(std::function<VuoCompilerType *(const string &)> getVuoType, dispatch_queue_t llvmQueue,
									   VuoCompilerIssues *issues)
{
	// Check for GLSL syntax errors.

	VuoShader shader = VuoShader_makeFromFile(shaderFile);
	VuoLocal(shader);

	__block VuoShaderIssues shaderIssues;
	__block bool ok;
	VuoGlContext_perform(^(CGLContextObj cgl_ctx){
							 ok = VuoShader_upload(shader, VuoMesh_IndividualTriangles, cgl_ctx, &shaderIssues);
						 });

	if (! ok)
	{
		for (VuoShaderIssues::Issue shaderIssue : shaderIssues.issues())
		{
			string summary = "Failed to parse shader";
			string details = shaderIssue.message;
			VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling ISF node class", "", summary, details);
			issue.setLineNumber(shaderIssue.lineNumber);
			issues->append(issue);
		}
		return nullptr;
	}

	// Look up types (before getting on llvmQueue).

	vector<string> vuoTypesUsed;
	vuoTypesUsed.push_back("VuoShader");
	vuoTypesUsed.push_back("VuoImage");
	vuoTypesUsed.push_back("VuoImageColorDepth");
	vuoTypesUsed.push_back("VuoReal");
	vuoTypesUsed.push_back("VuoInteger");
	vuoTypesUsed.push_back("VuoBoolean");
	for (VuoShaderFile::Port shaderInputPort : shaderFile->inputPorts())
		vuoTypesUsed.push_back(shaderInputPort.vuoTypeName);

	map<string, VuoCompilerType *> vuoTypes;
	for (string t : vuoTypesUsed)
		vuoTypes[t] = getVuoType(t);

	// Generate bitcode.

	__block Module *module;
	dispatch_sync(llvmQueue, ^{

		module = new Module(moduleKey, *VuoCompiler::globalLLVMContext);
		VuoCompilerConstantsCache constantsCache(module);

		if (shaderFile->type() == VuoShaderFile::GLSLImageFilter
		 || shaderFile->type() == VuoShaderFile::GLSLImageGenerator
		 || shaderFile->type() == VuoShaderFile::GLSLImageTransition)
		{
			generateMetadata(module);
			generateNodeInstanceInitFunction(module, &constantsCache, vuoTypes, issues);
			generateNodeInstanceEventFunction(module, &constantsCache, vuoTypes, issues);
		}
		else if (shaderFile->type() == VuoShaderFile::GLSLObjectRenderer)
		{
			/// @todo https://b33p.net/kosada/node/13646
		}
		else if (shaderFile->type() == VuoShaderFile::GLSLObjectFilter)
		{
			/// @todo https://b33p.net/kosada/node/13646
		}

	});

	return module;
}

/**
 * Generates the `VuoModuleMetadata`.
 */
void VuoIsfModuleCompiler::generateMetadata(Module *module)
{
	json_object *metadata = shaderFile->vuoModuleMetadata();
	VuoCompilerCodeGenUtilities::generateModuleMetadata(module, json_object_to_json_string_ext(metadata, JSON_C_TO_STRING_PLAIN), "");
}

/**
 * Generates the `nodeInstanceInit` function and constants used by it.
 */
void VuoIsfModuleCompiler::generateNodeInstanceInitFunction(Module *module, VuoCompilerConstantsCache *constantsCache,
															map<string, VuoCompilerType *> vuoTypes, VuoCompilerIssues *issues)
{
	Constant *vertexSourceValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, shaderFile->expandedVertexSource(), "vertexSource");
	Constant *geometrySourceValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, shaderFile->expandedGeometrySource(), "geometrySource");
	Constant *fragmentSourceValue = VuoCompilerCodeGenUtilities::generatePointerToConstantString(module, shaderFile->expandedFragmentSource(), "fragmentSource");

	VuoCompilerType *vuoShaderType = vuoTypes["VuoShader"];
	if (!isTypeFound(vuoShaderType, issues))
		return;

	Type *llvmShaderType = getFunctionParameterType(vuoShaderType, issues);
	if (!llvmShaderType)
		return;

	map<VuoPort *, size_t> indexOfParameterInit;

	Function *function = VuoCompilerCodeGenUtilities::getNodeInstanceInitFunction(module, moduleKey, false,
																				  llvmShaderType, vector<VuoPort *>(),
																				  indexOfParameterInit, constantsCache);

	BasicBlock *block = &(function->getEntryBlock());

	// VuoShader s = VuoShader_make(sf->name());

	Function *makeFunction = VuoCompilerCodeGenUtilities::getVuoShaderMakeFunction(module);
	Value *shaderNameValue = constantsCache->get(shaderFile->name());
	Value *shaderValue = CallInst::Create(makeFunction, shaderNameValue, "s", block);

	// VuoShader_addSource(s, VuoMesh_IndividualTriangles, vertexSource, geometrySource, fragmentSource);

	Function *addSourceFunction = VuoCompilerCodeGenUtilities::getVuoShaderAddSourceFunction(module);
	vector<Value *> addSourceArgs;
	addSourceArgs.push_back(shaderValue);
	addSourceArgs.push_back(ConstantInt::get(addSourceFunction->getFunctionType()->getParamType(1), 0));
	addSourceArgs.push_back(vertexSourceValue);
	addSourceArgs.push_back(geometrySourceValue);
	addSourceArgs.push_back(fragmentSourceValue);
	CallInst::Create(addSourceFunction, addSourceArgs, "", block);

	// ISF shaders can potentially produce transparent output,
	// even if the input images and colors are opaque.
	// s->isTransparent = true;
	Function *setTransparentFunction = VuoCompilerCodeGenUtilities::getVuoShaderSetTransparentFunction(module);
	vector<Value *> setTransparentArgs;
	setTransparentArgs.push_back(shaderValue);
	setTransparentArgs.push_back(ConstantInt::get(setTransparentFunction->getFunctionType()->getParamType(1), 1));
	CallInst::Create(setTransparentFunction, setTransparentArgs, "", block);

	// return s;

	Value *shaderValueAsReturn = new BitCastInst(shaderValue, function->getReturnType(), "", block);
	ReturnInst::Create(module->getContext(), shaderValueAsReturn, block);
}

/**
 * Generates the `nodeInstanceEvent` function.
 */
void VuoIsfModuleCompiler::generateNodeInstanceEventFunction(Module *module, VuoCompilerConstantsCache *constantsCache,
															 map<string, VuoCompilerType *> vuoTypes, VuoCompilerIssues *issues)
{
	vector<VuoPort *> inputPorts;
	vector<VuoPort *> outputPorts;
	map<VuoPort *, json_object *> detailsForPorts;

	auto addInputPort = [&inputPorts, &detailsForPorts, &vuoTypes] (const string &portName, const string &typeName, json_object *details)
	{
		VuoCompilerType *type = typeName == "event" ? NULL : vuoTypes[typeName];
		VuoCompilerInputEventPort *inputPort = VuoCompilerInputEventPort::newPort(portName, type ? type->getBase() : NULL);
		inputPorts.push_back(inputPort->getBase());
		detailsForPorts[inputPort->getBase()] = details;
	};

	auto addOutputPort = [&outputPorts, &detailsForPorts, &vuoTypes] (const string &portName, const string &typeName, json_object *details)
	{
		VuoCompilerType *type = vuoTypes[typeName];
		VuoCompilerOutputEventPort *outputPort = VuoCompilerOutputEventPort::newPort(portName, type->getBase());
		outputPorts.push_back(outputPort->getBase());
		detailsForPorts[outputPort->getBase()] = details;
	};

	VuoPort *widthInputPort = nullptr;
	VuoPort *heightInputPort = nullptr;
	auto addWidthHeightInputPorts = [&inputPorts, &addInputPort, &widthInputPort, &heightInputPort] (void)
	{
		// VuoInputData(VuoInteger, {"name":"Width", "default":640, "suggestedMin":1, "suggestedStep":32}) vuoWidth,
		// VuoInputData(VuoInteger, {"name":"Height", "default":480, "suggestedMin":1, "suggestedStep":32}) vuoHeight

		json_object *widthDetails = json_object_new_object();
		json_object_object_add(widthDetails, "name", VuoText_getJson("Width"));
		json_object_object_add(widthDetails, "default", VuoInteger_getJson(640));
		json_object_object_add(widthDetails, "suggestedMin", VuoInteger_getJson(1));
		json_object_object_add(widthDetails, "suggestedStep", VuoInteger_getJson(32));
		json_object *heightDetails = json_object_new_object();
		json_object_object_add(heightDetails, "name", VuoText_getJson("Height"));
		json_object_object_add(heightDetails, "default", VuoInteger_getJson(480));
		json_object_object_add(heightDetails, "suggestedMin", VuoInteger_getJson(1));
		json_object_object_add(heightDetails, "suggestedStep", VuoInteger_getJson(32));

		addInputPort("vuoWidth", "VuoInteger", widthDetails);
		if (! widthInputPort)
			widthInputPort = inputPorts.back();
		addInputPort("vuoHeight", "VuoInteger", heightDetails);
		if (! heightInputPort)
			heightInputPort = inputPorts.back();
	};

	VuoPort *timeInputPort = nullptr;
	if (shaderFile->showsTime())
	{
		// VuoInputData(VuoReal, {"name":"Time", "default":0.0, "suggestedStep":0.1}) vuoTime

		json_object *details = json_object_new_object();
		json_object_object_add(details, "name", VuoText_getJson("Time"));
		json_object_object_add(details, "default", VuoReal_getJson(0.0));
		json_object_object_add(details, "suggestedStep", VuoReal_getJson(0.1));

		addInputPort("vuoTime", "VuoReal", details);
		timeInputPort = inputPorts.back();
	}

	VuoPort *colorDepthInputPort = nullptr;
	bool hasImageInputPort = false;
	for (VuoShaderFile::Port shaderInputPort : shaderFile->inputPorts())
	{
		if (shaderInputPort.vuoTypeName == "VuoSize")
		{
			addWidthHeightInputPorts();
		}
		else if (shaderInputPort.vuoTypeName == "VuoImageColorDepth")
		{
			// VuoInputData(VuoImageColorDepth, {"name":"Color Depth", "default":"8bpc"}) vuoColorDepth

			json_object *details = shaderInputPort.vuoPortDetails;
			json_object_object_add(details, "name", VuoText_getJson("Color Depth"));
			json_object_object_add(details, "default", VuoImageColorDepth_getJson(VuoImageColorDepth_8));

			addInputPort("vuoColorDepth", shaderInputPort.vuoTypeName, details);

			if (! colorDepthInputPort)
				colorDepthInputPort = inputPorts.back();
		}
		else
		{
			// VuoInputData(type) name

			if (shaderInputPort.vuoTypeName == "VuoBoolean")
			{
				json_object *defaultValue = NULL;
				json_object_object_get_ex(shaderInputPort.vuoPortDetails, "default", &defaultValue);
				defaultValue = VuoBoolean_getJson(VuoBoolean_makeFromJson(defaultValue));
				json_object_object_add(shaderInputPort.vuoPortDetails, "default", defaultValue);
			}

			addInputPort(shaderInputPort.key, shaderInputPort.vuoTypeName, shaderInputPort.vuoPortDetails);

			if (shaderInputPort.vuoTypeName == "VuoImage")
				hasImageInputPort = true;
		}
	}

	if (! hasImageInputPort && ! widthInputPort && ! heightInputPort)
	{
		addWidthHeightInputPorts();
	}

	{
		// VuoOutputData(type) name

		VuoShaderFile::Port shaderOutputPort = shaderFile->outputPort();
		addOutputPort(shaderOutputPort.key, "VuoImage", shaderOutputPort.vuoPortDetails);
	}

	VuoCompilerType *vuoShaderType = vuoTypes["VuoShader"];
	if (!isTypeFound(vuoShaderType, issues))
		return;

	Type *llvmShaderType = getFunctionParameterType(vuoShaderType, issues);
	if (!llvmShaderType)
		return;

	map<VuoPort *, size_t> indexOfParameter;
	map<VuoPort *, size_t> indexOfEventParameter;

	Function *function = VuoCompilerCodeGenUtilities::getNodeEventFunction(module, moduleKey, false, true,
																		   llvmShaderType, inputPorts, outputPorts,
																		   detailsForPorts, map<VuoPort *, string>(),
																		   map<VuoPort *, string>(), map<VuoPort *, VuoPortClass::EventBlocking>(),
																		   indexOfParameter, indexOfEventParameter, constantsCache);

	BasicBlock *initialBlock = &(function->getEntryBlock());

	Value *outputImagePointer = VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, indexOfParameter[outputPorts.at(0)]);
	Value *instanceDataPointerValue = VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, 0);
	Value *instanceDataValue = new LoadInst(instanceDataPointerValue, "instanceData", false, initialBlock);

	// Find the first non-NULL input image.

	BasicBlock *findImageBlock = initialBlock;
	BasicBlock *widthHeightBlock = BasicBlock::Create(module->getContext(), "widthHeight", function, nullptr);

	VuoCompilerType *vuoImageType = vuoTypes["VuoImage"];
	if (!isTypeFound(vuoImageType, issues))
		return;

	Type *llvmImageType = getFunctionParameterType(vuoImageType, issues);
	if (!llvmImageType)
		return;

	ConstantPointerNull *nullImageValue = ConstantPointerNull::get(static_cast<PointerType *>(llvmImageType));

	AllocaInst *foundImageVariable = new AllocaInst(llvmImageType, 0, "foundImageVar", findImageBlock);
	new StoreInst(nullImageValue, foundImageVariable, findImageBlock);

	BasicBlock *currBlock = findImageBlock;
	for (VuoPort *inputPort : inputPorts)
	{
		VuoType *type = static_cast<VuoCompilerPort *>(inputPort->getCompiler())->getDataVuoType();
		if (type && type->getModuleKey() == "VuoImage")
		{
			BasicBlock *imageNotNullBlock = BasicBlock::Create(module->getContext(), "imageNotNull", function, nullptr);
			BasicBlock *imageNullBlock = BasicBlock::Create(module->getContext(), "imageNull", function, nullptr);

			Value *imageValue = VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, indexOfParameter[inputPort]);
			ICmpInst *imageNotNull = new ICmpInst(*currBlock, ICmpInst::ICMP_NE, imageValue, nullImageValue, "");
			BranchInst::Create(imageNotNullBlock, imageNullBlock, imageNotNull, currBlock);

			new StoreInst(imageValue, foundImageVariable, imageNotNullBlock);
			BranchInst::Create(widthHeightBlock, imageNotNullBlock);

			currBlock = imageNullBlock;
		}
	}

	BranchInst::Create(widthHeightBlock, currBlock);

	// Set width and height…

	BasicBlock *colorDepthBlock = nullptr;
	BasicBlock *finalBlock = BasicBlock::Create(module->getContext(), "final", function, nullptr);

	Value *widthValue = nullptr;
	Value *heightValue = nullptr;
	if (widthInputPort && heightInputPort)
	{
		// … from input port values if they exist.

		widthValue = VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, indexOfParameter[widthInputPort]);
		heightValue = VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, indexOfParameter[heightInputPort]);

		colorDepthBlock = widthHeightBlock;
	}
	else
	{
		// … to the first non-NULL image's pixelsWide/High.
		// If no non-NULL image, set the output image to NULL and return.

		BasicBlock *imageNotNullBlock = BasicBlock::Create(module->getContext(), "foundImageNotNull", function, nullptr);
		BasicBlock *imageNullBlock = BasicBlock::Create(module->getContext(), "foundImageNull", function, nullptr);
		colorDepthBlock = BasicBlock::Create(module->getContext(), "colorDepth", function, nullptr);

		Value *imageValue = new LoadInst(foundImageVariable, "foundImage", false, widthHeightBlock);
		ICmpInst *imageNotNull = new ICmpInst(*widthHeightBlock, ICmpInst::ICMP_NE, imageValue, nullImageValue, "");
		BranchInst::Create(imageNotNullBlock, imageNullBlock, imageNotNull, widthHeightBlock);

		widthValue = VuoCompilerCodeGenUtilities::generateGetStructPointerElement(module, imageNotNullBlock, imageValue, 3);
		heightValue = VuoCompilerCodeGenUtilities::generateGetStructPointerElement(module, imageNotNullBlock, imageValue, 4);
		BranchInst::Create(colorDepthBlock, imageNotNullBlock);

		new StoreInst(nullImageValue, outputImagePointer, imageNullBlock);
		BranchInst::Create(finalBlock, imageNullBlock);
	}

	// Set colorDepth…

	Value *colorDepthValue = nullptr;
	if (colorDepthInputPort)
	{
		// … from the input port value if it exists.

		colorDepthValue = VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, indexOfParameter[colorDepthInputPort]);
	}
	else
	{
		// … to the first non-NULL image's VuoImage_getColorDepth().
		// If no non-NULL image, default to VuoImageColorDepth_8.

		VuoCompilerType *vuoColorDepthType = vuoTypes["VuoImageColorDepth"];
		if (!isTypeFound(vuoColorDepthType, issues))
			return;

		Type *llvmColorDepthType = getFunctionParameterType(vuoColorDepthType, issues);
		if (! llvmColorDepthType)
			return;

		AllocaInst *colorDepthVariable = new AllocaInst(llvmColorDepthType, 0, "colorDepthVar", colorDepthBlock);

		Value *defaultColorDepthValue = ConstantInt::get(llvmColorDepthType, 0);
		new StoreInst(defaultColorDepthValue, colorDepthVariable, colorDepthBlock);

		BasicBlock *imageNotNullBlock = BasicBlock::Create(module->getContext(), "foundImageNotNull", function, nullptr);
		BasicBlock *loadColorDepthBlock = BasicBlock::Create(module->getContext(), "loadColorDepth", function, nullptr);

		Value *imageValue = new LoadInst(foundImageVariable, "foundImage", false, colorDepthBlock);
		ICmpInst *imageNotNull = new ICmpInst(*colorDepthBlock, ICmpInst::ICMP_NE, imageValue, nullImageValue, "");
		BranchInst::Create(imageNotNullBlock, loadColorDepthBlock, imageNotNull, colorDepthBlock);

		Function *colorDepthFunction = VuoCompilerCodeGenUtilities::getVuoImageGetColorDepthFunction(module);
		Value *imageValueArg = new BitCastInst(imageValue, colorDepthFunction->getFunctionType()->getParamType(0), "", imageNotNullBlock);
		Value *imageColorDepthValue = CallInst::Create(colorDepthFunction, imageValueArg, "colorDepth", imageNotNullBlock);
		new StoreInst(imageColorDepthValue, colorDepthVariable, imageNotNullBlock);
		BranchInst::Create(loadColorDepthBlock, imageNotNullBlock);

		colorDepthValue = new LoadInst(colorDepthVariable, "colorDepth", false, loadColorDepthBlock);

		colorDepthBlock = loadColorDepthBlock;
	}

	// Set uniforms.

	BasicBlock *setUniformsBlock = colorDepthBlock;

	for (VuoPort *inputPort : inputPorts)
	{
		// Don't provide uniforms for inputs applied when rendering the image (vuoWidth, vuoHeight, vuoColorDepth).
		if (inputPort == widthInputPort || inputPort == heightInputPort || inputPort == colorDepthInputPort)
			continue;

		// VuoShader_setUniform_<type>(*instance, key, value);

		VuoType *portDataType = static_cast<VuoCompilerPort *>(inputPort->getCompiler())->getDataVuoType();
		Function *setUniformFunction = VuoCompilerCodeGenUtilities::getVuoShaderSetUniformFunction(module, portDataType ? portDataType->getCompiler() : vuoTypes["VuoBoolean"]);

		Value *instanceDataValue_setUniform = new BitCastInst(instanceDataValue, setUniformFunction->getFunctionType()->getParamType(0), "", setUniformsBlock);

		string portName = (inputPort == timeInputPort ? "TIME" : inputPort->getClass()->getName());
		Value *portNameValue = constantsCache->get(portName);

		Value *portDataArg = VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, indexOfParameter[inputPort]);
		Value *secondPortDataArg = nullptr;
		bool hasSecondParam = (setUniformFunction->getFunctionType()->getNumParams() == 4);
		if (hasSecondParam)
			secondPortDataArg = VuoCompilerCodeGenUtilities::getArgumentAtIndex(function, indexOfParameter[inputPort] + 1);

		// Some ISF shaders expect their `point2D` inputs in shader2DRect (pixel) coordinates.
		if (portDataType && portDataType->getModuleKey() == "VuoPoint2d")
		{
			bool scaleToSamplerRect = false;
			json_object *j;
			if (json_object_object_get_ex(detailsForPorts[inputPort], "scaleToSamplerRect", &j))
				scaleToSamplerRect = json_object_get_boolean(j);

			if (scaleToSamplerRect)
			{
				// value = VuoShader_samplerRectCoordinatesFromNormalizedCoordinates(value, width, height);
				Function *coordinateFunction = VuoCompilerCodeGenUtilities::getVuoSamplerRectCoordinatesFromNormalizedCoordinatesFunction(module);

				vector<Value *> coordinateArgs;
				coordinateArgs.push_back(portDataArg);
				coordinateArgs.push_back(widthValue);
				coordinateArgs.push_back(heightValue);
				portDataArg = CallInst::Create(coordinateFunction, coordinateArgs, "", setUniformsBlock);
			}
		}

		portDataArg = VuoCompilerCodeGenUtilities::generateTypeCast(module, setUniformsBlock, portDataArg, setUniformFunction->getFunctionType()->getParamType(2));

		vector<Value *> setUniformArgs;
		setUniformArgs.push_back(instanceDataValue_setUniform);
		setUniformArgs.push_back(portNameValue);
		setUniformArgs.push_back(portDataArg);
		if (hasSecondParam)
			setUniformArgs.push_back(secondPortDataArg);
		CallInst::Create(setUniformFunction, setUniformArgs, "", setUniformsBlock);
	}

	// *outputImage = VuoImageRenderer_render(*instance, width, height, colorDepth);

	BasicBlock *renderBlock = setUniformsBlock;

	Function *renderFunction = VuoCompilerCodeGenUtilities::getVuoImageRendererRenderFunction(module);

	Value *instanceDataValue_render = new BitCastInst(instanceDataValue, renderFunction->getFunctionType()->getParamType(0), "", renderBlock);

	vector<Value *> renderArgs;
	renderArgs.push_back(instanceDataValue_render);
	renderArgs.push_back(widthValue);
	renderArgs.push_back(heightValue);
	renderArgs.push_back(colorDepthValue);
	Value *renderedImageValue = CallInst::Create(renderFunction, renderArgs, "", renderBlock);

	Type *outputImageType = static_cast<PointerType *>( outputImagePointer->getType() )->getElementType();
	Value *renderedImageValueArg = new BitCastInst(renderedImageValue, outputImageType, "", renderBlock);

	new StoreInst(renderedImageValueArg, outputImagePointer, renderBlock);

	BranchInst::Create(finalBlock, renderBlock);

	ReturnInst::Create(module->getContext(), finalBlock);
}

bool VuoIsfModuleCompiler::isTypeFound(VuoCompilerType *type, VuoCompilerIssues *issues)
{
	if (!type)
	{
		string moduleKey = type->getBase()->getModuleKey();
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling ISF node class", "", "Couldn't find type " + moduleKey + ".", "");
		issues->append(issue);
		return false;
	}

	return true;
}

Type * VuoIsfModuleCompiler::getFunctionParameterType(VuoCompilerType *type, VuoCompilerIssues *issues)
{
	vector<Type *> params = type->getFunctionParameterTypes();
	if (params.size() != 1)
	{
		string moduleKey = type->getBase()->getModuleKey();
		ostringstream details;
		details << moduleKey << " is lowered to " << params.size() << " parameters; expected 1.";
		VuoCompilerIssue issue(VuoCompilerIssue::Error, "compiling ISF node class", "", moduleKey + " has an unsupported format.", details.str());
		issues->append(issue);
		return nullptr;
	}

	return params[0];
}
