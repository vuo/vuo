/**
 * @file
 * VuoShader uniform setters.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

/**
 * Helper for `VuoShader_setUniform_*()`.
 */
#define SET_UNIFORM(typeName, valueName)																	\
	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);						\
																											\
	/* Is there already a uniform with this identifier?  If so, overwrite it. */							\
	for (int i = 0; i < shader->uniformsCount; ++i)															\
		if (strcmp(shader->uniforms[i].name, uniformIdentifier) == 0)										\
		{																									\
			VuoRelease(shader->uniforms[i].type);															\
			shader->uniforms[i].type = VuoText_make(#typeName);												\
			VuoRetain(shader->uniforms[i].type);															\
																											\
			shader->uniforms[i].value.valueName = valueName;												\
																											\
			dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);									\
			return;																							\
		}																									\
																											\
	/* Otherwise, expand and add another uniform. */														\
	{																										\
		++shader->uniformsCount;																			\
		shader->uniforms = (VuoShaderUniform *)realloc(shader->uniforms, sizeof(VuoShaderUniform) * shader->uniformsCount);	\
																											\
		VuoShaderUniform u;																					\
																											\
		u.name = VuoText_make(uniformIdentifier);															\
		VuoRetain(u.name);																					\
																											\
		u.type = VuoText_make(#typeName);																	\
		VuoRetain(u.type);																					\
																											\
		u.value.valueName = valueName;																		\
																											\
		u.compiledTextureTarget = 0;                                                                        \
																											\
		shader->uniforms[shader->uniformsCount-1] = u;														\
	}																										\
																											\
	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);

/**
 * Helper for `VuoShader_setUniform_*()`.
 */
#define SET_UNIFORM_HEAP(typeName, valueName)                                                               \
	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);                     \
																											\
	/* Is there already a uniform with this identifier?  If so, overwrite it. */                            \
	for (int i = 0; i < shader->uniformsCount; ++i)                                                         \
		if (strcmp(shader->uniforms[i].name, uniformIdentifier) == 0)                                       \
		{                                                                                                   \
			VuoRelease(shader->uniforms[i].type);                                                           \
			shader->uniforms[i].type = VuoText_make(#typeName);                                             \
			VuoRetain(shader->uniforms[i].type);                                                            \
																											\
			VuoRelease(shader->uniforms[i].value.valueName);                                                \
			shader->uniforms[i].value.valueName = valueName;                                                \
			VuoRetain(shader->uniforms[i].value.valueName);                                                 \
																											\
			dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);                                  \
			return;                                                                                         \
		}                                                                                                   \
																											\
	/* Otherwise, expand and add another uniform. */                                                        \
	{                                                                                                       \
		++shader->uniformsCount;                                                                            \
		shader->uniforms = (VuoShaderUniform *)realloc(shader->uniforms, sizeof(VuoShaderUniform) * shader->uniformsCount); \
																											\
		VuoShaderUniform u;                                                                                 \
																											\
		u.name = VuoText_make(uniformIdentifier);                                                           \
		VuoRetain(u.name);                                                                                  \
																											\
		u.type = VuoText_make(#typeName);                                                                   \
		VuoRetain(u.type);                                                                                  \
																											\
		u.value.valueName = valueName;                                                                      \
		VuoRetain(u.value.valueName);                                                                       \
																											\
		u.compiledTextureTarget = 0;                                                                        \
																											\
		shader->uniforms[shader->uniformsCount-1] = u;                                                      \
	}                                                                                                       \
																											\
	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);


/**
 * Sets a `VuoImage` input value on the specified @c shader.
 *
 * @threadAny
 */
void VuoShader_setUniform_VuoImage(VuoShader shader, const char *uniformIdentifier, const VuoImage image)
{
	SET_UNIFORM_HEAP(VuoImage, image);
}

/**
 * Sets a @c bool uniform value on the specified @c shader.
 *
 * @threadAny
 */
void VuoShader_setUniform_VuoBoolean(VuoShader shader, const char *uniformIdentifier, const VuoBoolean boolean)
{
	SET_UNIFORM(VuoBoolean, boolean);
}

/**
 * Sets an @c int uniform value on the specified @c shader.
 *
 * @threadAny
 */
void VuoShader_setUniform_VuoInteger(VuoShader shader, const char *uniformIdentifier, const VuoInteger integer)
{
	SET_UNIFORM(VuoInteger, integer);
}

/**
 * Sets a @c float uniform value on the specified @c shader.
 *
 * @threadAny
 */
void VuoShader_setUniform_VuoReal(VuoShader shader, const char *uniformIdentifier, const VuoReal real)
{
	SET_UNIFORM(VuoReal, real);
}

/**
 * Sets a @c vec2 uniform value on the specified @c shader.
 *
 * @threadAny
 */
void VuoShader_setUniform_VuoPoint2d(VuoShader shader, const char *uniformIdentifier, const VuoPoint2d point2d)
{
	SET_UNIFORM(VuoPoint2d, point2d);
}

/**
 * Sets a @c vec3 uniform value on the specified @c shader.
 *
 * @threadAny
 */
void VuoShader_setUniform_VuoPoint3d(VuoShader shader, const char *uniformIdentifier, const VuoPoint3d point3d)
{
	SET_UNIFORM(VuoPoint3d, point3d);
}

/**
 * Sets a @c vec4 uniform value on the specified @c shader.
 *
 * @threadAny
 */
void VuoShader_setUniform_VuoPoint4d(VuoShader shader, const char *uniformIdentifier, const VuoPoint4d point4d)
{
	SET_UNIFORM(VuoPoint4d, point4d);
}

/**
 * Sets a @c color uniform value on the specified @c shader accepting a VuoColor.
 *
 * `color` should be a normal un-premultiplied VuoColor;
 * this function premultiplies its colors before passing it to the shader.
 *
 * @threadAny
 */
void VuoShader_setUniform_VuoColor(VuoShader shader, const char *uniformIdentifier, const VuoColor colorUnpremultiplied)
{
	VuoColor color = VuoColor_premultiply(colorUnpremultiplied);
	SET_UNIFORM(VuoColor, color);
}

/**
 * Sets a list of @c bool uniform values on the specified @c shader.
 *
 * @threadAny
 * @version200New
 */
void VuoShader_setUniform_VuoList_VuoBoolean(VuoShader shader, const char *uniformIdentifier, const VuoList_VuoBoolean booleans)
{
	SET_UNIFORM_HEAP(VuoList_VuoBoolean, booleans);
}

/**
 * Sets a list of @c int uniform values on the specified @c shader.
 *
 * @threadAny
 * @version200New
 */
void VuoShader_setUniform_VuoList_VuoInteger(VuoShader shader, const char *uniformIdentifier, const VuoList_VuoInteger integers)
{
	SET_UNIFORM_HEAP(VuoList_VuoInteger, integers);
}

/**
 * Sets a list of @c float uniform values on the specified @c shader.
 *
 * @threadAny
 * @version200New
 */
void VuoShader_setUniform_VuoList_VuoReal   (VuoShader shader, const char *uniformIdentifier, const VuoList_VuoReal reals)
{
	SET_UNIFORM_HEAP(VuoList_VuoReal, reals);
}

/**
 * Sets a list of @c vec2 uniform values on the specified @c shader.
 *
 * @threadAny
 * @version200New
 */
void VuoShader_setUniform_VuoList_VuoPoint2d(VuoShader shader, const char *uniformIdentifier, const VuoList_VuoPoint2d point2ds)
{
	SET_UNIFORM_HEAP(VuoList_VuoPoint2d, point2ds);
}

/**
 * Sets a list of @c vec3 uniform values on the specified @c shader.
 *
 * @threadAny
 * @version200New
 */
void VuoShader_setUniform_VuoList_VuoPoint3d(VuoShader shader, const char *uniformIdentifier, const VuoList_VuoPoint3d point3ds)
{
	SET_UNIFORM_HEAP(VuoList_VuoPoint3d, point3ds);
}

/**
 * Sets a list of @c vec4 uniform values on the specified @c shader.
 *
 * @threadAny
 * @version200New
 */
void VuoShader_setUniform_VuoList_VuoPoint4d(VuoShader shader, const char *uniformIdentifier, const VuoList_VuoPoint4d point4ds)
{
	SET_UNIFORM_HEAP(VuoList_VuoPoint4d, point4ds);
}

/**
 * Sets a list of color uniform values on the specified `shader`'s uniform `uniformIdentifier` of type `vec4`.
 *
 * `colors` should be a list of normal un-premultiplied VuoColors;
 * this function premultiplies each color before passing it to the shader.
 *
 * @threadAny
 * @version200New
 */
void VuoShader_setUniform_VuoList_VuoColor(VuoShader shader, const char *uniformIdentifier, const VuoList_VuoColor colorsP)
{
	VuoList_VuoColor colors = VuoListCopy_VuoColor(colorsP);
	size_t colorCount = VuoListGetCount_VuoColor(colors);
	VuoColor *colorData = VuoListGetData_VuoColor(colors);
	for (size_t i = 0; i < colorCount; ++i)
			colorData[i] = VuoColor_premultiply(colorData[i]);
	SET_UNIFORM_HEAP(VuoList_VuoColor, colors);
}

/**
 * Sets a `mat2` (column-major 2x2 matrix) uniform value on the specified `shader`.
 *
 * @threadAny
 * @version200New
 */
void VuoShader_setUniform_mat2(VuoShader shader, const char *uniformIdentifier, float *mat2)
{
	SET_UNIFORM_HEAP(mat2, mat2);
}

/**
 * Sets a `mat3` (column-major 3x3 matrix) uniform value on the specified `shader`.
 *
 * @threadAny
 * @version200New
 */
void VuoShader_setUniform_mat3(VuoShader shader, const char *uniformIdentifier, float *mat3)
{
	SET_UNIFORM_HEAP(mat3, mat3);
}

/**
 * Sets a `mat4` (column-major 4x4 matrix) uniform value on the specified `shader`.
 *
 * @threadAny
 * @version200New
 */
void VuoShader_setUniform_mat4(VuoShader shader, const char *uniformIdentifier, float *mat4)
{
	SET_UNIFORM_HEAP(mat4, mat4);
}

/**
 * Returns the `VuoImage` for the specified `uniformIdentifier`,
 * or NULL if none matches.
 */
VuoImage VuoShader_getUniform_VuoImage(VuoShader shader, const char *uniformIdentifier)
{
	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);

	for (int i = 0; i < shader->uniformsCount; ++i)
		if (strcmp(shader->uniforms[i].type, "VuoImage") == 0
		 && strcmp(shader->uniforms[i].name, uniformIdentifier) == 0)
		{
			VuoImage image = shader->uniforms[i].value.image;
			dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
			return image;
		}

	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
	return NULL;
}

/**
 * Returns the first-added, non-NULL `VuoImage` uniform value,
 * or NULL if there are no image uniforms.
 */
VuoImage VuoShader_getFirstImage(VuoShader shader)
{
	dispatch_semaphore_wait((dispatch_semaphore_t)shader->lock, DISPATCH_TIME_FOREVER);

	for (int i = 0; i < shader->uniformsCount; ++i)
		if (strcmp(shader->uniforms[i].type, "VuoImage") == 0)
		{
			VuoImage image = shader->uniforms[i].value.image;
			if (image)
			{
				dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
				return image;
			}
		}

	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);
	return NULL;
}
