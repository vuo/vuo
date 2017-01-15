/**
 * @file
 * VuoShader uniform setters.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
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
			typeName ## _release(shader->uniforms[i].value.valueName);										\
			shader->uniforms[i].value.valueName = valueName;												\
			typeName ## _retain(shader->uniforms[i].value.valueName);										\
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
		typeName ## _retain(u.value.valueName);																\
																											\
		shader->uniforms[shader->uniformsCount-1] = u;														\
	}																										\
																											\
	dispatch_semaphore_signal((dispatch_semaphore_t)shader->lock);

/**
 * Sets a `VuoImage` input value on the specified @c shader.
 *
 * @threadAny
 */
void VuoShader_setUniform_VuoImage(VuoShader shader, const char *uniformIdentifier, const VuoImage image)
{
	SET_UNIFORM(VuoImage, image);
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
 * @threadAny
 */
void VuoShader_setUniform_VuoColor(VuoShader shader, const char *uniformIdentifier, const VuoColor color)
{
	SET_UNIFORM(VuoColor, color);
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
