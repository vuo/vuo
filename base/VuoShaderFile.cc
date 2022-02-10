/**
 * @file
 * VuoShaderFile implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoShaderFile.hh"

#include <regex.h>
#include <sstream>
#include "VuoCompositionMetadata.hh"
#include "VuoException.hh"
#include "VuoJsonUtilities.hh"
#include "VuoStringUtilities.hh"
#include "VuoShaderIssues.hh"

/**
 * Creates a new shader collection from a template.
 */
VuoShaderFile::VuoShaderFile(Type type)
{
	init(getTemplate(type));
}

/**
 * Returns the specified template's primary file (the one with the ISF JSON header).
 */
VuoFileUtilities::File VuoShaderFile::getTemplate(VuoShaderFile::Type type)
{
	string path = VuoFileUtilities::getVuoFrameworkPath() + "/Resources";

	string filename;
	if (type == GLSLImageFilter)
		filename = "GLSLImageFilter.fs";
	else if (type == GLSLImageGenerator)
		filename = "GLSLImageGenerator.fs";
	else if (type == GLSLImageTransition)
		filename = "GLSLImageTransition.fs";
	else if (type == GLSLObjectRenderer)
		filename = "GLSLObjectRenderer.fs";
	else // if (type == GLSLObjectFilter)
		filename = "GLSLObjectFilter.vs";

	return VuoFileUtilities::File(path, filename);
}

/**
 * Returns a display name for the specified shader stage.
 */
string VuoShaderFile::stageName(VuoShaderFile::Stage stage)
{
	if (stage == Vertex)
		return "Vertex";
	else if (stage == Geometry)
		return "Geometry";
	else if (stage == Fragment)
		return "Fragment";
	else // if (stage == Program)
		return "Program";
}

/**
 * Reads a shader collection from a set of files.
 *
 * The specified file can be any one in the shader collection
 * (i.e., you can pass the `.vs`, `.gs`, or `.fs` file).
 *
 * @throw VuoException The shader couldn't be parsed.
 */
VuoShaderFile::VuoShaderFile(VuoFileUtilities::File file, const string &overriddenFragmentSource)
{
	basename = file.basename();
	extension = file.extension();
	init(file, overriddenFragmentSource);
}

void VuoShaderFile::init(VuoFileUtilities::File file, const string &overriddenFragmentSource)
{
	_showsTime = false;

	json_object *isfMetadata = NULL;

	splitMetadataAndSource(readStage(file.fileWithDifferentExtension("vs")), &isfMetadata, _vertexSource);
	splitMetadataAndSource(readStage(file.fileWithDifferentExtension("gs")), &isfMetadata, _geometrySource);
	splitMetadataAndSource(! overriddenFragmentSource.empty() ? overriddenFragmentSource : readStage(file.fileWithDifferentExtension("fs")), &isfMetadata, _fragmentSource);

	parseMetadata(isfMetadata);
}

/**
 * Logs info about the shader file to the console.
 */
void VuoShaderFile::dump()
{
	VUserLog("\"%s\"", _name.c_str());
	VUserLog("    type: %s", typeName().c_str());
	VUserLog("    isfVersion: %s", isfVersion.c_str());
	VUserLog("    shaderVersion: %s", shaderVersion.c_str());
	VUserLog("    copyright: %s",copyright.c_str());
	VUserLog("    license: %s",license.c_str());
	VUserLog("    description: %s",description.c_str());
	VUserLog("    homepageLink: %s", homepageLink.c_str());
	VUserLog("    documentationLink: %s", documentationLink.c_str());
	VUserLog("    categories:");
	for (vector<string>::iterator i = categories.begin(); i != categories.end(); ++i)
		VUserLog("        %s", i->c_str());
	VUserLog("    keywords:");
	for (vector<string>::iterator i = keywords.begin(); i != keywords.end(); ++i)
		VUserLog("        %s", i->c_str());
	VUserLog("    inputs:");
	for (vector<Port>::iterator i = _inputPorts.begin(); i != _inputPorts.end(); ++i)
		VUserLog("        %s %s %s", i->vuoTypeName.c_str(), i->key.c_str(), json_object_to_json_string(i->vuoPortDetails));
	VUserLog("    showsTime       : %d", _showsTime);
	VUserLog("    outputKey : %s", outputKey.c_str());
	VUserLog("    outputName: %s", outputName.c_str());
	VUserLog("VS [%s]",_vertexSource.c_str());
	VUserLog("GS [%s]",_geometrySource.c_str());
	VUserLog("FS [%s]",_fragmentSource.c_str());
	VUserLog("VuoModuleMetadata %s",json_object_to_json_string(vuoModuleMetadata()));
}

/**
 * Returns the shader's display name.
 */
string VuoShaderFile::name()
{
	return _name;
}

/**
 * Returns this shader's type.
 */
VuoShaderFile::Type VuoShaderFile::type()
{
	return _type;
}

/**
 * Returns a text description of this shader's type.
 */
string VuoShaderFile::typeName()
{
	if (_type == VuoShaderFile::GLSLImageFilter)
		return "Image Filter";
	else if (_type == VuoShaderFile::GLSLImageGenerator)
		return "Image Generator";
	else if (_type == VuoShaderFile::GLSLImageTransition)
		return "Image Transition";
	else if (_type == VuoShaderFile::GLSLObjectRenderer)
		return "3D Object Renderer";
	else
		return "3D Object Filter";
}

/**
 * Returns this shader's ISF type key.
 */
string VuoShaderFile::typeNameISF()
{
	if (_type == VuoShaderFile::GLSLImageFilter
	 || _type == VuoShaderFile::GLSLImageGenerator
	 || _type == VuoShaderFile::GLSLImageTransition)
		return "IMAGE";
	else if (_type == VuoShaderFile::GLSLObjectRenderer)
		return "OBJECT_RENDER";
	else
		return "OBJECT_FILTER";
}

/**
 * Returns true if this shader type can have a geometry shader phase.
 */
bool VuoShaderFile::typeAllowsGeometryShader()
{
	return _type == VuoShaderFile::GLSLObjectRenderer
		|| _type == VuoShaderFile::GLSLObjectFilter;
}

/**
 * Returns true if this shader type can have a fragment shader phase.
 */
bool VuoShaderFile::typeAllowsFragmentShader()
{
	return _type == VuoShaderFile::GLSLImageFilter
		|| _type == VuoShaderFile::GLSLImageGenerator
		|| _type == VuoShaderFile::GLSLImageTransition
		|| _type == VuoShaderFile::GLSLObjectRenderer;
}

/**
 * Returns the metadata for this shader. The caller takes ownership of the returned object.
 */
VuoCompositionMetadata *VuoShaderFile::metadata()
{
	VuoCompositionMetadata *metadata = new VuoCompositionMetadata();
	metadata->setDefaultName("VuoShader");
	metadata->setName(_name);
	metadata->setVersion(shaderVersion);
	metadata->setCopyright(copyright);
	metadata->setLicense(license);
	metadata->setDescription(description);
	metadata->setHomepageURL(homepageLink);
	metadata->setDocumentationURL(documentationLink);
	metadata->setKeywords(keywords);
	metadata->setCategories(categories);
	return metadata;
}

/**
 * Sets the metadata values for this shader. The caller keeps ownership of @a metadata.
 */
void VuoShaderFile::setMetadata(VuoCompositionMetadata *metadata)
{
	string customizedName = metadata->getCustomizedName();
	if (! customizedName.empty())
		_name = customizedName;

	shaderVersion = metadata->getVersion();
	copyright = metadata->getCopyright();
	license = metadata->getLicense();
	description = metadata->getDescription();
	homepageLink = metadata->getHomepageURL();
	documentationLink = metadata->getDocumentationURL();
	keywords = metadata->getKeywords();
	categories = metadata->getCategories();
}

/**
 * Returns a JSON object in @ref VuoModuleMetadata format.
 */
json_object *VuoShaderFile::vuoModuleMetadata()
{
	return _vuoModuleMetadata;
}

/**
 * Sets the shader's metadata, in @ref VuoModuleMetadata format.
 */
void VuoShaderFile::setVuoModuleMetadata(json_object *metadata)
{
	_vuoModuleMetadata = metadata;
}

/**
 * Returns this shader's published input ports.
 */
vector<VuoShaderFile::Port> VuoShaderFile::inputPorts()
{
	return _inputPorts;
}

/**
 * Sets this shader's published input ports.
 */
void VuoShaderFile::setInputPorts(vector<VuoShaderFile::Port> ports)
{
	_inputPorts = ports;
}

/**
 * Returns true if the node should show a Time port
 * and use its value to populate the `TIME` uniform.
 */
bool VuoShaderFile::showsTime()
{
	return _showsTime;
}

/**
 * Returns this shader's published output port.
 */
VuoShaderFile::Port VuoShaderFile::outputPort()
{
	json_object *vuoPortDetails = json_object_new_object();
	json_object_object_add(vuoPortDetails, "name", json_object_new_string(outputName.c_str()));
	return (Port){outputKey, "", vuoPortDetails};
}

/**
 * Returns the user-editable source code for this shader's vertex shader phase
 * (or empty string if the default passthru shader should be used).
 */
string &VuoShaderFile::vertexSource()
{
	return _vertexSource;
}

/**
 * Sets the user-edited source code for this shader's vertex shader phase.
 */
void VuoShaderFile::setVertexSource(const string &source)
{
	_vertexSource = source;
}

void VuoShaderFile::insertPreamble(ostringstream &oss, bool isFragment)
{
	oss << "#version 120\n"

		// Ensure errors in the preamble don't overlap with errors in user-entered code.
		<< "#line " << VuoShaderIssues::PreambleLine << "\n"

		   "\n"
		   "// Standard ISF variables\n"

		   "varying vec2 isf_FragNormCoord;\n"
//		   "varying vec3 isf_VertNorm;\n"
//		   "varying vec3 isf_VertPos;\n"

		   "uniform int PASSINDEX;\n"

//		   "uniform vec2 RENDERSIZE;\n"
		   "uniform vec2 viewportSize;\n"
		   "#define RENDERSIZE viewportSize\n"

		   "uniform float TIME;\n"
		   "uniform float TIMEDELTA;\n"
		   "uniform vec4 DATE;\n"
		   "uniform int FRAMEINDEX;\n"

		   // ISF v1
		   "#define vv_FragNormCoord isf_FragNormCoord\n"
		   "#define vv_vertShaderInit isf_vertShaderInit\n";

	oss << "\n"
		   "// Input ports\n";
	for (vector<Port>::iterator it = _inputPorts.begin(); it != _inputPorts.end(); ++it)
		oss << glslDeclarationForPort(*it);

	if (isFragment)
		oss << "\n"
			   "#include \"ISFGLMacro2D.txt\"\n"
//			   "#include \"ISFGLMacro2DBias.txt\"\n"
			   "#include \"ISFGLMacro2DRect.txt\"\n"
//			   "#include \"ISFGLMacro2DRectBias.txt\"\n"
			   "\n";
}


/**
 * Returns the source code for this shader's vertex shader phase,
 * with the ISF includes and uniforms inserted.
 */
string VuoShaderFile::expandedVertexSource()
{
	ostringstream oss;

	insertPreamble(oss, false);

	oss << "#include \"ISFGLSceneVertShaderIncludeVarDec.txt\"\n"
		<< "void isf_vertShaderInit(void)\n"
		<< "{\n"
			<< "\t#include \"ISFGLSceneVertShaderIncludeInitFunc.txt\"\n"
			// @@@ set _texCoord + _normTexCoord
		<< "}\n";

	if (_vertexSource.empty())
		oss << "#include \"ISFGLScenePassthru.vs\"";
	else
		oss << "#line 0\n"
			<< _vertexSource;

	return oss.str();
}

/**
 * Returns the user-editable source code for this shader's geometry shader phase
 * (or empty string if the geometry shader phase should be disabled).
 */
string &VuoShaderFile::geometrySource()
{
	return _geometrySource;
}

/**
 * Sets the user-edited source code for this shader's geometry shader phase.
 */
void VuoShaderFile::setGeometrySource(const string &source)
{
	_geometrySource = source;
}

/**
 * Returns the source code for this shader's geometry shader phase,
 * with the ISF includes and uniforms inserted
 * (or empty string if the geometry shader phase should be disabled).
 */
string VuoShaderFile::expandedGeometrySource()
{
	if (_geometrySource.empty())
		return "";

	ostringstream oss;

	insertPreamble(oss, false);

	oss << "#line 0\n"
		<< _geometrySource;

	return oss.str();
}

/**
 * Returns the user-editable source code for this shader's fragment shader phase
 * (or empty string if the fragment shader phase should be disabled).
 */
string &VuoShaderFile::fragmentSource()
{
	return _fragmentSource;
}

/**
 * Returns the user-edited source code for this shader's fragment shader phase.
 */
void VuoShaderFile::setFragmentSource(const string &source)
{
	_fragmentSource = source;
}

/**
 * Returns the source code for this shader's fragment shader phase,
 * with the ISF includes and uniforms inserted
 * (or empty string if the fragment shader phase should be disabled).
 */
string VuoShaderFile::expandedFragmentSource()
{
	if (_type == GLSLObjectFilter)
		return "";

	ostringstream oss;

	insertPreamble(oss, true);

	oss << "#line 0\n"
		<< _fragmentSource;

	return oss.str();
}

/**
 * Writes the shader to `filePath`.
 *
 * `filePath`'s extension is ignored.
 * A file is created for each non-disabled, non-default shader phase.
 * Stage files that are disabled or default are deleted.
 */
void VuoShaderFile::save(string filePath)
{
	string basename = filePath.substr(0, filePath.find_last_of('.'));

	string vertexFile   = basename + ".vs";
	string geometryFile = basename + ".gs";
	string fragmentFile = basename + ".fs";

	if (_type == GLSLImageFilter
	 || _type == GLSLImageGenerator
	 || _type == GLSLImageTransition
	 || _type == GLSLObjectRenderer)
	{
		saveStage(vertexFile, _vertexSource);
		saveStage(geometryFile, _geometrySource);
		saveStage(fragmentFile, _fragmentSource, _vuoModuleMetadata);
	}
	else // if (type == GLSLObjectFilter)
	{
		saveStage(vertexFile, _vertexSource, _vuoModuleMetadata);
		saveStage(geometryFile, _geometrySource);
		string empty = "";
		saveStage(fragmentFile, empty); // Delete stale file.
	}
}

/**
 * Returns the string that would be written to the fragment shader file by @ref save.
 */
string VuoShaderFile::fragmentFileContents()
{
	return stageFileContents(_fragmentSource, _vuoModuleMetadata);
}

string VuoShaderFile::readStage(VuoFileUtilities::File file)
{
	if (!file.exists())
		return "";

	return file.getContentsAsString();
}

void VuoShaderFile::splitMetadataAndSource(string inputString, json_object **outputMetadata, string &outputSourceString)
{
	regex_t metadataRegex;
	regcomp(&metadataRegex, "/\\*[\\s\r\n]*{.*}[\\s\r\n]*\\*/", REG_EXTENDED);
	size_t nmatch = 1;
	regmatch_t pmatch[nmatch];
	bool metadataFound = !regexec(&metadataRegex, inputString.c_str(), nmatch, pmatch, 0);
	regfree(&metadataRegex);
	if (metadataFound)
	{
		string metadataString = inputString.substr(pmatch[0].rm_so + 2, (pmatch[0].rm_eo - 1) - pmatch[0].rm_so - 3);
		outputSourceString = inputString.substr(0, pmatch[0].rm_so) + inputString.substr(pmatch[0].rm_eo);

		json_tokener_error error;
		*outputMetadata = json_tokener_parse_verbose(metadataString.c_str(), &error);
		if (!*outputMetadata)
		{
			VUserLog("Error: The metadata header isn't valid JSON: %s", json_tokener_error_desc(error));
			return;
		}
	}
	else
		outputSourceString = inputString;

	outputSourceString = VuoStringUtilities::trim(outputSourceString);

	// Check whether the `TIME` uniform is used.
	if (!_showsTime)
	{
		string::size_type len = outputSourceString.length();
		string::size_type pos = 0;
		while ( (pos = outputSourceString.find("TIME", pos)) != string::npos )
			if ( (pos > 1) && !isalnum(outputSourceString[pos - 1])
			  && (pos + 4 < len) && !isalnum(outputSourceString[pos + 4]) )
			{
				_showsTime = true;
				break;
			}
			else
				pos += 4;
	}
}

void VuoShaderFile::parseMetadata(json_object *metadata)
{
	string normalizedBasename = basename;
	while (VuoStringUtilities::endsWith(normalizedBasename, "." + extension))
		normalizedBasename = normalizedBasename.substr(0, normalizedBasename.length() - extension.length() - 1);

	isfVersion        = VuoJsonUtilities::parseString(metadata, "ISFVSN", "1.0");
	string typeString = VuoJsonUtilities::parseString(metadata, "TYPE", "IMAGE");
	_name             = VuoJsonUtilities::parseString(metadata, "LABEL");
	shaderVersion     = VuoJsonUtilities::parseString(metadata, "VSN");
	copyright         = VuoJsonUtilities::parseString(metadata, "CREDIT");
	license           = VuoJsonUtilities::parseString(metadata, "LICENSE");
	description       = VuoJsonUtilities::parseString(metadata, "DESCRIPTION");
	homepageLink      = VuoJsonUtilities::parseObjectString(metadata, "LINKS", "HOMEPAGE");
	documentationLink = VuoJsonUtilities::parseObjectString(metadata, "LINKS", "DOCUMENTATION");
	categories        = VuoJsonUtilities::parseArrayOfStrings(metadata, "CATEGORIES");
	keywords          = VuoJsonUtilities::parseArrayOfStrings(metadata, "KEYWORDS");
	examples          = VuoJsonUtilities::parseArrayOfStrings(metadata, "EXAMPLES");

	json_object *passes;
	if (json_object_object_get_ex(metadata, "PASSES", &passes)
		&& ((json_object_is_type(passes, json_type_array ) && json_object_array_length(passes))
		 || (json_object_is_type(passes, json_type_object) && json_object_object_length(passes))))
		throw VuoException("This shader uses ISF's multipass feature, which Vuo doesn't support yet.");

	json_object *imported;
	if (json_object_object_get_ex(metadata, "IMPORTED", &imported)
	 && ((json_object_is_type(imported, json_type_array ) && json_object_array_length(imported))
	  || (json_object_is_type(imported, json_type_object) && json_object_object_length(imported))))
		throw VuoException("This shader uses ISF's image-importing feature, which Vuo doesn't support yet.");

	bool hasInputImage = false;
	bool hasStartImage = false;
	bool hasEndImage = false;
	bool hasProgress = false;
	json_object *inputsArray;
	if (json_object_object_get_ex(metadata, "INPUTS", &inputsArray))
	{
		int inputCount = json_object_array_length(inputsArray);
		for (int i = 0; i < inputCount; ++i)
		{
			json_object *inputObject = json_object_array_get_idx(inputsArray, i);
			string key = VuoJsonUtilities::parseString(inputObject, "NAME");
			string type = VuoJsonUtilities::parseString(inputObject, "TYPE");
			string name = VuoJsonUtilities::parseString(inputObject, "LABEL");

			if (key == "inputImage")
				hasInputImage = true;
			else if (key == "startImage")
				hasStartImage = true;
			else if (key == "endImage")
				hasEndImage = true;
			else if (key == "progress")
				hasProgress = true;

			string vuoType = vuoTypeForIsfType(type);
			if (vuoType.empty())
			{
				VUserLog("Warning: Input port '%s' has unknown type '%s'.", key.c_str(), type.c_str());
				continue;
			}

			json_object *vuoPortDetails = json_object_new_object();

			if (name.empty() && key == "inputImage")
				name = "Image";

			if (!name.empty())
				json_object_object_add(vuoPortDetails, "name", json_object_new_string(name.c_str()));

			json_object *defaultValue = NULL;
			if (json_object_object_get_ex(inputObject, "DEFAULT", &defaultValue))
				json_object_object_add(vuoPortDetails, "default", defaultValue);

			json_object *minValue = NULL;
			if (json_object_object_get_ex(inputObject, "MIN", &minValue))
				json_object_object_add(vuoPortDetails, "suggestedMin", minValue);
			else
			{
				if (vuoType == "VuoReal")
					json_object_object_add(vuoPortDetails, "suggestedMin", json_object_new_double(0));
				else if (vuoType == "VuoPoint2d")
				{
					json_object_object_add(vuoPortDetails, "scaleToSamplerRect", json_object_new_boolean(true));
					json_object_object_add(vuoPortDetails, "suggestedMin", json_object_new_string("0,0"));
				}
			}

			json_object *maxValue = NULL;
			if (json_object_object_get_ex(inputObject, "MAX", &maxValue))
				json_object_object_add(vuoPortDetails, "suggestedMax", maxValue);
			else
			{
				if (vuoType == "VuoReal")
					json_object_object_add(vuoPortDetails, "suggestedMax", json_object_new_double(1));
				else if (vuoType == "VuoPoint2d")
				{
					json_object_object_add(vuoPortDetails, "scaleToSamplerRect", json_object_new_boolean(true));
					json_object_object_add(vuoPortDetails, "suggestedMax", json_object_new_string("1,1"));
				}
			}

			json_object *stepValue = NULL;
			if (json_object_object_get_ex(inputObject, "STEP", &stepValue))
				json_object_object_add(vuoPortDetails, "suggestedStep", stepValue);

			if (VuoStringUtilities::beginsWith(vuoType, "VuoList_"))
			{
				json_object *maxItemsValue = NULL;
				if (json_object_object_get_ex(inputObject, "MAX_ITEMS", &maxItemsValue))
					json_object_object_add(vuoPortDetails, "maxItems", maxItemsValue);
			}

			json_object *values = NULL;
			json_object *labels = NULL;
			if (json_object_object_get_ex(inputObject, "VALUES", &values)
			 && json_object_object_get_ex(inputObject, "LABELS", &labels))
			{
				json_object *menuItems = json_object_new_array();
				int valueCount = json_object_array_length(values);
				for (int i = 0; i < valueCount; ++i)
				{
					json_object *menuItem = json_object_new_object();
					json_object_object_add(menuItem, "value", json_object_array_get_idx(values, i));
					json_object_object_add(menuItem, "name", json_object_array_get_idx(labels, i));
					json_object_array_add(menuItems, menuItem);
				}
				json_object_object_add(vuoPortDetails, "menuItems", menuItems);
			}

			_inputPorts.push_back((Port){ key, vuoType, vuoPortDetails });
		}
	}

	json_object *outputsArray;
	if (json_object_object_get_ex(metadata, "OUTPUTS", &outputsArray))
	{
		int outputCount = json_object_array_length(outputsArray);
		for (int i = 0; i < outputCount; ++i)
		{
			json_object *outputObject = json_object_array_get_idx(outputsArray, i);
			outputKey  = VuoJsonUtilities::parseString(outputObject, "NAME");
			outputName = VuoJsonUtilities::parseString(outputObject, "LABEL");
		}
	}

	if (typeString == "IMAGE")
	{
		if (hasInputImage)
			_type = GLSLImageFilter;
		else if (hasStartImage && hasEndImage && hasProgress)
			_type = GLSLImageTransition;
		else
			_type = GLSLImageGenerator;

		if (outputKey.empty())
			outputKey = "outputImage";
		if (outputName.empty())
			outputName = VuoStringUtilities::expandCamelCase(outputKey);
	}
	else if (typeString == "OBJECT_RENDER")
	{
		_type = GLSLObjectRenderer;

		if (outputKey.empty())
			outputKey = "shader";
		if (outputName.empty())
			outputName = VuoStringUtilities::expandCamelCase(outputKey);
	}
	else if (typeString == "OBJECT_FILTER")
	{
		_type = GLSLObjectFilter;

		if (outputKey.empty())
			outputKey = "outputObject";
		if (outputName.empty())
			outputName = VuoStringUtilities::expandCamelCase(outputKey);
	}

	_vuoModuleMetadata = json_object_new_object();

	string title = (! _name.empty() ? _name : normalizedBasename);
	json_object_object_add(_vuoModuleMetadata, "title", json_object_new_string(title.c_str()));

	string metadataDescription = description;
	if (!copyright.empty())
		metadataDescription += "\n\n" + copyright;
	if (!metadataDescription.empty())
		json_object_object_add(_vuoModuleMetadata, "description", json_object_new_string(metadataDescription.c_str()));

	if (!shaderVersion.empty())
		json_object_object_add(_vuoModuleMetadata, "version", json_object_new_string(shaderVersion.c_str()));
	if (!keywords.empty())
		json_object_object_add(_vuoModuleMetadata, "keywords", VuoJsonUtilities::getJson(keywords));

	if (!examples.empty())
	{
		json_object *nodeMetadataJson = json_object_new_object();
		json_object_object_add(nodeMetadataJson, "exampleCompositions", VuoJsonUtilities::getJson(examples));
		json_object_object_add(_vuoModuleMetadata, "node", nodeMetadataJson);
	}

	if (_type == VuoShaderFile::GLSLImageFilter
	 || _type == VuoShaderFile::GLSLImageGenerator
	 || _type == VuoShaderFile::GLSLImageTransition
	 || _type == VuoShaderFile::GLSLObjectFilter)
	{
		json_object *dependenciesJson = json_object_new_array();

		if (_type == VuoShaderFile::GLSLImageFilter
		 || _type == VuoShaderFile::GLSLImageGenerator
		 || _type == VuoShaderFile::GLSLImageTransition)
			json_object_array_add(dependenciesJson, json_object_new_string("VuoImageRenderer"));
		else if (_type == VuoShaderFile::GLSLObjectFilter)
			json_object_array_add(dependenciesJson, json_object_new_string("VuoSceneObjectRenderer"));

		json_object_object_add(_vuoModuleMetadata, "dependencies", dependenciesJson);
	}
}

string VuoShaderFile::vuoTypeForIsfType(string isfType)
{
	if (isfType == "event")
		return "event";

	else if (isfType == "bool")
		return "VuoBoolean";
	else if (isfType == "long")
		return "VuoInteger";
	else if (isfType == "float")
		return "VuoReal";
	else if (isfType == "point2D")
		return "VuoPoint2d";
	else if (isfType == "point3D")
		return "VuoPoint3d";
	else if (isfType == "point4D")
		return "VuoPoint4d";
	else if (isfType == "color")
		return "VuoColor";

	else if (isfType == "bool[]")
		return "VuoList_VuoBoolean";
	else if (isfType == "long[]")
		return "VuoList_VuoInteger";
	else if (isfType == "float[]")
		return "VuoList_VuoReal";
	else if (isfType == "point2D[]")
		return "VuoList_VuoPoint2d";
	else if (isfType == "point3D[]")
		return "VuoList_VuoPoint3d";
	else if (isfType == "point4D[]")
		return "VuoList_VuoPoint4d";
	else if (isfType == "color[]")
		return "VuoList_VuoColor";

	else if (isfType == "image")
		return "VuoImage";

	else if (isfType == "size")
		return "VuoSize";
	else if (isfType == "colorDepth")
		return "VuoImageColorDepth";

	return "";
}

string VuoShaderFile::isfTypeForVuoType(string vuoType)
{
	if (vuoType == "event")
		return "event";
	else if (vuoType == "VuoBoolean")
		return "bool";
	else if (vuoType == "VuoInteger")
		return "long";
	else if (vuoType == "VuoReal")
		return "float";
	else if (vuoType == "VuoPoint2d")
		return "point2D";
	else if (vuoType == "VuoPoint3d")
		return "point3D";
	else if (vuoType == "VuoPoint4d")
		return "point4D";
	else if (vuoType == "VuoColor")
		return "color";

	else if (vuoType == "VuoList_VuoBoolean")
		return "bool[]";
	else if (vuoType == "VuoList_VuoInteger")
		return "long[]";
	else if (vuoType == "VuoList_VuoReal")
		return "float[]";
	else if (vuoType == "VuoList_VuoPoint2d")
		return "point2D[]";
	else if (vuoType == "VuoList_VuoPoint3d")
		return "point3D[]";
	else if (vuoType == "VuoList_VuoPoint4d")
		return "point4D[]";
	else if (vuoType == "VuoList_VuoColor")
		return "color[]";

	else if (vuoType == "VuoImage")
		return "image";

	else if (vuoType == "VuoSize")
		return "size";
	else if (vuoType == "VuoImageColorDepth")
		return "colorDepth";

	return "";
}

/**
 * Returns the allowed data types for input and output ports.
 */
set<string> VuoShaderFile::supportedVuoTypes()
{
	set<string> types;

	types.insert("VuoBoolean");
	types.insert("VuoInteger");
	types.insert("VuoReal");
	types.insert("VuoPoint2d");
	types.insert("VuoPoint3d");
	types.insert("VuoPoint4d");
	types.insert("VuoColor");

	types.insert("VuoList_VuoBoolean");
	types.insert("VuoList_VuoInteger");
	types.insert("VuoList_VuoReal");
	types.insert("VuoList_VuoPoint2d");
	types.insert("VuoList_VuoPoint3d");
	types.insert("VuoList_VuoPoint4d");
	types.insert("VuoList_VuoColor");

	types.insert("VuoImage");
	types.insert("VuoSize");
	types.insert("VuoImageColorDepth");

	return types;
}

string VuoShaderFile::glslDeclarationForPort(Port port)
{
	string maxItems = to_string(VuoJsonUtilities::parseInt(port.vuoPortDetails, "maxItems", 16));

	if (port.vuoTypeName == "event"
	 || port.vuoTypeName == "VuoBoolean")
		return "uniform bool " + port.key + ";\n";
	else if (port.vuoTypeName == "VuoInteger")
		return "uniform int " + port.key + ";\n";
	else if (port.vuoTypeName == "VuoReal")
		return "uniform float " + port.key + ";\n";
	else if (port.vuoTypeName == "VuoPoint2d")
		return "uniform vec2 " + port.key + ";\n";
	else if (port.vuoTypeName == "VuoPoint3d")
		return "uniform vec3 " + port.key + ";\n";
	else if (port.vuoTypeName == "VuoPoint4d")
		return "uniform vec4 " + port.key + ";\n";
	else if (port.vuoTypeName == "VuoColor")
		return "uniform vec4 " + port.key + ";\n";

	else if (port.vuoTypeName == "VuoList_VuoBoolean")
		return "uniform bool " + port.key + "[" + maxItems + "];\n"
			   "uniform int _" + port.key + "_length;\n";
	else if (port.vuoTypeName == "VuoList_VuoInteger")
		return "uniform int " + port.key + "[" + maxItems + "];\n"
			   "uniform int _" + port.key + "_length;\n";
	else if (port.vuoTypeName == "VuoList_VuoReal")
		return "uniform float " + port.key + "[" + maxItems + "];\n"
			   "uniform int _" + port.key + "_length;\n";
	else if (port.vuoTypeName == "VuoList_VuoPoint2d")
		return "uniform vec2 " + port.key + "[" + maxItems + "];\n"
			   "uniform int _" + port.key + "_length;\n";
	else if (port.vuoTypeName == "VuoList_VuoPoint3d")
		return "uniform vec3 " + port.key + "[" + maxItems + "];\n"
			   "uniform int _" + port.key + "_length;\n";
	else if (port.vuoTypeName == "VuoList_VuoPoint4d")
		return "uniform vec4 " + port.key + "[" + maxItems + "];\n"
			   "uniform int _" + port.key + "_length;\n";
	else if (port.vuoTypeName == "VuoList_VuoColor")
		return "uniform vec4 " + port.key + "[" + maxItems + "];\n"
			   "uniform int _" + port.key + "_length;\n";

	else if (port.vuoTypeName == "VuoImage")
		return "//uniform VuoImage " + port.key + ";\n" // We don't know the sampler type until execution time, so use a placeholder.
			   "uniform vec4 _" + port.key + "_imgRect;\n"
			   "uniform vec2 _" + port.key + "_imgSize;\n"
			   "uniform bool _" + port.key + "_flip;\n";

	return "";
}

void VuoShaderFile::saveStage(string filePath, string &source, json_object *vuoModuleMetadata)
{
	if (source.empty())
	{
		// This stage has been disabled.  Delete the file,
		// to avoid leaving stale files around.
		if (VuoFileUtilities::fileExists(filePath))
			VuoFileUtilities::moveFileToTrash(filePath);
		return;
	}

	string fileContents = stageFileContents(source, vuoModuleMetadata);

	VuoFileUtilities::writeStringToFile(fileContents, filePath);
}

string VuoShaderFile::stageFileContents(string &source, json_object *vuoModuleMetadata)
{
	ostringstream ss;
	if (vuoModuleMetadata)
	{
		ss << "/*";

		json_object *isfMetadata = json_object_new_object();

		if (isfVersion.empty())
			json_object_object_add(isfMetadata, "ISFVSN", json_object_new_string("2.0"));
		else
			json_object_object_add(isfMetadata, "ISFVSN", json_object_new_string(isfVersion.c_str()));

		json_object_object_add(isfMetadata, "TYPE", json_object_new_string(typeNameISF().c_str()));

		if (!_name.empty())
			json_object_object_add(isfMetadata, "LABEL", json_object_new_string(_name.c_str()));

		if (!shaderVersion.empty())
			json_object_object_add(isfMetadata, "VSN", json_object_new_string(shaderVersion.c_str()));

		if (!copyright.empty())
			json_object_object_add(isfMetadata, "CREDIT", json_object_new_string(copyright.c_str()));

		if (!license.empty())
			json_object_object_add(isfMetadata, "LICENSE", json_object_new_string(license.c_str()));

		if (!description.empty())
			json_object_object_add(isfMetadata, "DESCRIPTION", json_object_new_string(description.c_str()));

		if (!homepageLink.empty() || !documentationLink.empty())
		{
			json_object *links = json_object_new_object();
			if (!homepageLink.empty())
				json_object_object_add(links, "HOMEPAGE", json_object_new_string(homepageLink.c_str()));
			if (!documentationLink.empty())
				json_object_object_add(links, "DOCUMENTATION", json_object_new_string(documentationLink.c_str()));
			json_object_object_add(isfMetadata, "LINKS", links);
		}

		if (!categories.empty())
			json_object_object_add(isfMetadata, "CATEGORIES", VuoJsonUtilities::getJson(categories));

		if (!keywords.empty())
			json_object_object_add(isfMetadata, "KEYWORDS", VuoJsonUtilities::getJson(keywords));

		if (!examples.empty())
			json_object_object_add(isfMetadata, "EXAMPLES", VuoJsonUtilities::getJson(examples));

		// INPUTS
		if (!_inputPorts.empty())
		{
			json_object *inputs = json_object_new_array();
			for (vector<Port>::iterator i = _inputPorts.begin(); i != _inputPorts.end(); ++i)
			{
				json_object *input = json_object_new_object();

				if (!i->key.empty())
					json_object_object_add(input, "NAME", json_object_new_string(i->key.c_str()));

				json_object_object_add(input, "TYPE", json_object_new_string(isfTypeForVuoType(i->vuoTypeName).c_str()));

				json_object *nameValue = NULL;
				if (json_object_object_get_ex(i->vuoPortDetails, "name", &nameValue))
					json_object_object_add(input, "LABEL", nameValue);

				json_object *defaultValue = NULL;
				if (json_object_object_get_ex(i->vuoPortDetails, "default", &defaultValue))
				{
					// Convert Vuo serialization to ISF serialization.
					if (VuoStringUtilities::beginsWith(i->vuoTypeName, "VuoPoint")
					 && json_object_get_type(defaultValue) == json_type_object)
					{
						json_object *x = nullptr;
						json_object *y = nullptr;
						json_object *z = nullptr;
						json_object *w = nullptr;
						json_object_object_get_ex(defaultValue, "x", &x);
						json_object_object_get_ex(defaultValue, "y", &y);
						json_object_object_get_ex(defaultValue, "z", &z);
						json_object_object_get_ex(defaultValue, "w", &w);

						defaultValue = json_object_new_array();
						json_object_array_add(defaultValue, x);
						json_object_array_add(defaultValue, y);
						if (i->vuoTypeName == "VuoPoint3d" || i->vuoTypeName == "VuoPoint4d")
							json_object_array_add(defaultValue, z);
						if (i->vuoTypeName == "VuoPoint4d")
							json_object_array_add(defaultValue, w);
					}

					json_object_object_add(input, "DEFAULT", defaultValue);
				}

				json_object *scaleToSamplerRectValue = NULL;
				if (!(json_object_object_get_ex(i->vuoPortDetails, "scaleToSamplerRect", &scaleToSamplerRectValue)
				   && json_object_get_boolean(scaleToSamplerRectValue)))
				{
					json_object *minValue = NULL;
					if (json_object_object_get_ex(i->vuoPortDetails, "suggestedMin", &minValue))
						json_object_object_add(input, "MIN", minValue);

					json_object *maxValue = NULL;
					if (json_object_object_get_ex(i->vuoPortDetails, "suggestedMax", &maxValue))
						json_object_object_add(input, "MAX", maxValue);
				}

				json_object *stepValue = NULL;
				if (json_object_object_get_ex(i->vuoPortDetails, "suggestedStep", &stepValue))
					json_object_object_add(input, "STEP", stepValue);

				json_object *maxItemsValue = NULL;
				if (json_object_object_get_ex(i->vuoPortDetails, "maxItems", &maxItemsValue))
					json_object_object_add(input, "MAX_ITEMS", maxItemsValue);

				json_object *menuItemsValue = NULL;
				if (json_object_object_get_ex(i->vuoPortDetails, "menuItems", &menuItemsValue))
				{
					json_object *valueItems = json_object_new_array();
					json_object *labelItems = json_object_new_array();
					int len = json_object_array_length(menuItemsValue);
					for (int mi = 0; mi < len; ++mi)
					{
						json_object *menuItem = json_object_array_get_idx(menuItemsValue, mi);

						json_object *valueItem = nullptr;
						json_object_object_get_ex(menuItem, "value", &valueItem);
						json_object_array_add(valueItems, valueItem);

						json_object *nameItem = nullptr;
						json_object_object_get_ex(menuItem, "name", &nameItem);
						json_object_array_add(labelItems, nameItem);
					}
					json_object_object_add(input, "VALUES", valueItems);
					json_object_object_add(input, "LABELS", labelItems);
				}

				json_object_array_add(inputs, input);
			}
			json_object_object_add(isfMetadata, "INPUTS", inputs);
		}

		// OUTPUTS
		{
			json_object *output = json_object_new_object();

			if (_type == VuoShaderFile::GLSLImageFilter
			 || _type == VuoShaderFile::GLSLImageGenerator
			 || _type == VuoShaderFile::GLSLImageTransition)
			{
				if (outputKey != "outputImage")
					json_object_object_add(output, "NAME", json_object_new_string(outputKey.c_str()));
				if (outputName != "Output Image")
					json_object_object_add(output, "LABEL", json_object_new_string(outputName.c_str()));
			}
			else if (_type == VuoShaderFile::GLSLObjectRenderer)
			{
				if (outputKey != "shader")
					json_object_object_add(output, "NAME", json_object_new_string(outputKey.c_str()));
				if (outputName != "Shader")
					json_object_object_add(output, "LABEL", json_object_new_string(outputName.c_str()));
			}
			else
			{
				if (outputKey != "outputObject")
					json_object_object_add(output, "NAME", json_object_new_string(outputKey.c_str()));
				if (outputName != "Output Object")
					json_object_object_add(output, "LABEL", json_object_new_string(outputName.c_str()));
			}

			if (json_object_object_length(output))
			{
				json_object *outputs = json_object_new_array();
				json_object_array_add(outputs, output);
				json_object_object_add(isfMetadata, "OUTPUTS", outputs);
			}
		}

		string json = json_object_to_json_string_ext(isfMetadata, JSON_C_TO_STRING_PRETTY);
		ss << spacesToTabs(json);
		ss << "*/\n\n";
	}

	ss << source;

	if (source[source.length() - 1] != '\n')
		ss << "\n";

	return ss.str();
}

/**
 * Converts each leading pair of spaces to tabs.
 */
string VuoShaderFile::spacesToTabs(string &str)
{
	stringstream strStream(str);
	string ln;
	ostringstream out;
	while (getline(strStream, ln))
	{
		size_t spaces = ln.find_first_not_of(' ');
		out << string(spaces / 2, '\t');
		out << ln.substr(spaces);
		if (!strStream.eof())
			out << '\n';
	}
	return out.str();
}
