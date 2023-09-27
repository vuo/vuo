/**
 * @file
 * VuoShaderFile interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <vector>

#include "VuoFileUtilities.hh"

typedef struct _VuoShader *VuoShader; ///< See @ref VuoShader
class VuoShaderIssues;
class VuoCompositionMetadata;

/**
 * Reads and writes a graphics shader file collection.
 *
 * This class parses the commented JSON header at the top of ISF files
 * (since that metadata is needed to build a node around the shader).
 *
 * This class leaves \c \#include directives unresolved (to comply with the LGPL);
 * @ref VuoShader is responsible for resolving those.
 */
class VuoShaderFile
{
public:

	/**
	 * How the shader is intended to be used.
	 */
	enum Type
	{
		GLSLImageFilter,
		GLSLImageGenerator,
		GLSLImageTransition,
		GLSLObjectRenderer,
		GLSLObjectFilter,
	};

	/**
	 * The individual phases of a shader collection.
	 */
	enum Stage
	{
		Vertex,
		Geometry,
		Fragment,

		Program,
	};

	/**
	 * A published input/output port in a shader.
	 */
	struct Port
	{
		string key;
		string vuoTypeName;
		json_object *vuoPortDetails;
	};

	VuoShaderFile(Type type);
	VuoShaderFile(VuoFileUtilities::File file, const string &overriddenFragmentSource="");

	static VuoFileUtilities::File getTemplate(Type type);
	static string stageName(Stage stage);
	void dump();

	string name();

	Type type();
	string typeName();
	string typeNameISF();

//	bool typeAllowsVertexShader(); // All types allow a Vertex Shader.
	bool typeAllowsGeometryShader();
	bool typeAllowsFragmentShader();

	static set<string> supportedVuoTypes();

	VuoCompositionMetadata *metadata();
	void setMetadata(VuoCompositionMetadata *metadata);

	json_object *vuoModuleMetadata();
	void setVuoModuleMetadata(json_object *metadata);

	vector<Port> inputPorts();
	void setInputPorts(vector<Port> ports);
	bool showsTime();

	Port outputPort();

	string &vertexSource();
	void setVertexSource(const string &source);
	string expandedVertexSource();

	string &geometrySource();
	void setGeometrySource(const string &source);
	string expandedGeometrySource();

	string &fragmentSource();
	void setFragmentSource(const string &source);
	string expandedFragmentSource();

	void save(string filePath);
	string fragmentFileContents();

private:
	void init(VuoFileUtilities::File file, const string &overriddenFragmentSource="");
	string readStage(VuoFileUtilities::File path);
	void splitMetadataAndSource(string inputString, json_object **outputMetadata, string &outputSourceString);
	void parseMetadata(json_object *metadata);
	string vuoTypeForIsfType(string isfType);
	string isfTypeForVuoType(string vuoType);
	string glslDeclarationForPort(Port port);
	void saveStage(string filePath, string &source, json_object *vuoModuleMetadata=NULL);
	string stageFileContents(string &source, json_object *vuoModuleMetadata=NULL);
	string spacesToTabs(string &str);
	void insertPreamble(ostringstream &oss, bool isFragment);

	json_object *_vuoModuleMetadata;

	string basename;
	string extension;

	Type _type;

	vector<Port> _inputPorts;
	string outputKey;
	string outputName;

	string _vertexSource;
	string _geometrySource;
	string _fragmentSource;

	string isfVersion;
	string shaderVersion;
	string _name;
	string copyright;
	string license;
	string description;
	string homepageLink;
	string documentationLink;
	vector<string> categories;
	vector<string> keywords;
	vector<string> examples;
	bool _showsTime;
};
