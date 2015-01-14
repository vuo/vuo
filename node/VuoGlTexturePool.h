/**
 * @file
 * VuoGlTexturePool interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <OpenGL/OpenGL.h>

void VuoGlTexturePool_retain(GLuint glTextureName);
void VuoGlTexturePool_release(GLuint glTextureName);

#ifdef __cplusplus
}
#endif
