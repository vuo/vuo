/**
 * @file
 * VuoCodeHighlighterGLSL implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCodeHighlighterGLSL.hh"

#include "VuoCodeEditor.hh"

/**
 * Creates a highlighter.
 */
VuoCodeHighlighterGLSL::VuoCodeHighlighterGLSL(QTextDocument *parent, VuoCodeEditor *codeEditor)
	: QSyntaxHighlighter(parent), codeEditor(codeEditor)
{
}

void VuoCodeHighlighterGLSL::highlightBlock(const QString &text)
{
	// From https://www.khronos.org/registry/OpenGL/specs/gl/GLSLangSpec.1.20.pdf

	// Keywords
	{
		QTextCharFormat format;
		format.setFontWeight(QFont::Bold);
		format.setFontItalic(true);
		format.setForeground(codeEditor->keywordColor);

		QRegularExpression expression("\\b(attribute|const|uniform|varying|centroid|break|continue|do|for|while|if|else|in|out|inout|float|int|void|bool|true|false|invariant|discard|return|mat[234]|mat[234]x[234]|[ib]?vec[234]|sampler[123]D|samplerCube|sampler[12]DShadow|struct)\\b");
		QRegularExpressionMatchIterator i = expression.globalMatch(text);
		while (i.hasNext())
		{
			QRegularExpressionMatch match = i.next();
			setFormat(match.capturedStart(), match.capturedLength(), format);
		}
	}

	// Built-in variables
	{
		QTextCharFormat format;
		format.setFontWeight(QFont::Bold);
		format.setForeground(codeEditor->builtinVariableColor);

		QRegularExpression expression("\\bgl_(BackColor|BackLightModelProduct|BackLightProduct|BackMaterial|BackSecondaryColor|ClipPlane|ClipVertex|Color|Color|DepthRange|DepthRangeParameters|DepthRangeParameters\\.diff|DepthRangeParameters\\.far|DepthRangeParameters\\.near|EyePlaneQ|EyePlaneR|EyePlaneS|EyePlaneT|Fog|FogCoord|FogFragCoord|FogFragCoord|FogParameters|FogParameters\\.color|FogParameters\\.density|FogParameters\\.end|FogParameters\\.scale|FogParameters\\.start|FragColor|FragCoord|FragData|FragDepth|FrontColor|FrontFacing|FrontLightModelProduct|FrontLightProduct|FrontMaterial|FrontSecondaryColor|LightModel|LightModelParameters|LightModelParameters\\.ambient|LightModelProducts|LightModelProducts\\.sceneColor|LightProducts|LightProducts\\.ambient|LightProducts\\.diffuse|LightProducts\\.specular|LightSource|LightSourceParameters|LightSourceParameters\\.ambient|LightSourceParameters\\.antAttenuation|LightSourceParameters\\.diffuse|LightSourceParameters\\.halfVector|LightSourceParameters\\.linearAttenuation|LightSourceParameters\\.position|LightSourceParameters\\.quadraticAttenuation|LightSourceParameters\\.specular|LightSourceParameters\\.spotCosCutoff|LightSourceParameters\\.spotCutoff|LightSourceParameters\\.spotDirection|LightSourceParameters\\.spotExponent|MaterialParameters|MaterialParameters\\.ambient|MaterialParameters\\.diffuse|MaterialParameters\\.emission|MaterialParameters\\.shininess|MaterialParameters\\.specular|MaxClipPlanes|MaxCombinedTextureImageUnits|MaxDrawBuffers|MaxFragmentComponents|MaxLights|MaxTextureCoords|MaxTextureImageUnits|MaxTextureUnits|MaxVaryingFloats|MaxVertexAttribs|MaxVertexTextureImageUnits|MaxVertexUniformComponents|ModelViewMatrix|ModelViewMatrixInverse|ModelViewMatrixInverseTranspose|ModelViewMatrixTranspose|ModelViewProjectionMatrix|ModelViewProjectionMatrixInverse|ModelViewProjectionMatrixInverseTranspose|ModelViewProjectionMatrixTranspose|MultiTexCoord0|MultiTexCoord1|MultiTexCoord2|MultiTexCoord3|MultiTexCoord4|MultiTexCoord5|MultiTexCoord6|MultiTexCoord7|Normal|NormalMatrix|NormalScale|ObjectPlaneQ|ObjectPlaneR|ObjectPlaneS|ObjectPlaneT|PoSize|Point|PointCoord|PointParameters|PointParameters\\.distanceLinearAttenuation|PointParameters\\.distanceQuadraticAttenuation|PointParameters\\.distanceantAttenuation|PointParameters\\.fadeThresholdSize|PointParameters\\.size|PointParameters\\.sizeMax|PointParameters\\.sizeMin|Position|ProjectionMatrix|ProjectionMatrixInverse|ProjectionMatrixInverseTranspose|ProjectionMatrixTranspose|SecondaryColor|SecondaryColor|TexCoord|TexCoord|TextureEnvColor|TextureMatrix|TextureMatrixInverse|TextureMatrixInverseTranspose|TextureMatrixTranspose|Vertex)\\b");
		QRegularExpressionMatchIterator i = expression.globalMatch(text);
		while (i.hasNext())
		{
			QRegularExpressionMatch match = i.next();
			setFormat(match.capturedStart(), match.capturedLength(), format);
		}
	}

	// Built-in functions
	{
		QTextCharFormat format;
		format.setFontWeight(QFont::Bold);
		format.setForeground(codeEditor->builtinFunctionColor);

		QRegularExpression expression("\\b(abs|acos|all|any|asin|atan|ceil|clamp|cos|cross|dFdx|dFdy|degrees|distance|dot|equal|exp|exp2|faceforward|floor|fract|ftransform|fwidth|greaterThan|greaterThanEqual|inversesqrt|length|lessThan|lessThanEqual|log|log2|matrixCompMult|max|min|mix|mod|noise1|noise2|noise3|noise4|normalize|not|notEqual|outerProduct|pow|radians|reflect|refract|shadow1D|shadow1DLod|shadow1DProj|shadow1DProjLod|shadow2D|shadow2DLod|shadow2DProj|shadow2DProjLod|sign|sin|smoothstep|sqrt|step|tan|texture1D|texture1DLod|texture1DProj|texture1DProjLod|texture2D|texture2DLod|texture2DProj|texture2DProjLod|texture3D|texture3DLod|texture3DProj|texture3DProjLod|textureCube|textureCubeLod|transpose)\\b");
		QRegularExpressionMatchIterator i = expression.globalMatch(text);
		while (i.hasNext())
		{
			QRegularExpressionMatch match = i.next();
			setFormat(match.capturedStart(), match.capturedLength(), format);
		}
	}

	// Operators and other punctuation
	{
		QTextCharFormat format;
		format.setFontWeight(QFont::Bold);
		format.setForeground(codeEditor->operatorColor);

		QRegularExpression expression("[{}\\[\\]()+\\-~!*/%<>&^=;|.,]");
		QRegularExpressionMatchIterator i = expression.globalMatch(text);
		while (i.hasNext())
		{
			QRegularExpressionMatch match = i.next();
			setFormat(match.capturedStart(), match.capturedLength(), format);
		}
	}

	// Constants
	{
		QTextCharFormat format;
		format.setFontWeight(QFont::Bold);
		format.setForeground(codeEditor->constantColor);

		QString integer("\\d+|0[xX]\\d+");
		QString floatingPoint("[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?");
		QRegularExpression expression("\\b(true|false|" + floatingPoint + "|" + integer + ")\\b");
		QRegularExpressionMatchIterator i = expression.globalMatch(text);
		while (i.hasNext())
		{
			QRegularExpressionMatch match = i.next();
			setFormat(match.capturedStart(), match.capturedLength(), format);
		}
	}

	// Preprocessor
	{
		QTextCharFormat format;
		format.setFontWeight(QFont::Bold);
		format.setForeground(codeEditor->preprocessorColor);

		QRegularExpression expression("^\\s*#.+|__LINE__|__FILE__|__VERSION__");
		QRegularExpressionMatchIterator i = expression.globalMatch(text);
		while (i.hasNext())
		{
			QRegularExpressionMatch match = i.next();
			setFormat(match.capturedStart(), match.capturedLength(), format);
		}
	}

	// Comments are last, so the style for commented parts of the line overrides any above styles.

	// C++-style Comments
	{
		QTextCharFormat format;
		format.setFontItalic(true);
		format.setForeground(codeEditor->commentColor);

		QRegularExpression expression("//.+");
		QRegularExpressionMatchIterator i = expression.globalMatch(text);
		while (i.hasNext())
		{
			QRegularExpressionMatch match = i.next();
			setFormat(match.capturedStart(), match.capturedLength(), format);
		}
	}

	// C-style Comments
	{
		QTextCharFormat format;
		format.setFontItalic(true);
		format.setForeground(codeEditor->commentColor);

		// From https://doc.qt.io/qt-5/qsyntaxhighlighter.html#details

		QRegularExpression startExpression("/\\*");
		QRegularExpression endExpression("\\*/");

		setCurrentBlockState(0);

		int startIndex = 0;
		if (previousBlockState() != 1)
			startIndex = text.indexOf(startExpression);

		while (startIndex >= 0)
		{
		   QRegularExpressionMatch endMatch;
		   int endIndex = text.indexOf(endExpression, startIndex, &endMatch);
		   int commentLength;
		   if (endIndex == -1)
		   {
			   setCurrentBlockState(1);
			   commentLength = text.length() - startIndex;
		   }
		   else
			   commentLength = endIndex - startIndex + endMatch.capturedLength();

		   setFormat(startIndex, commentLength, format);
		   startIndex = text.indexOf(startExpression, startIndex + commentLength);
		}

		// Don't highlight anything else in comment blocks.
		if (currentBlockState() == 1)
			return;
	}
}
