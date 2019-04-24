/**
 * @file
 * VuoBase implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoBase.hh"

#include <stdio.h>


/**
 * Creates a base class.
 *
 * @param id The name of the base class inheriting VuoBase.
 */
template<class CompilerClass, class RendererClass>
VuoBase<CompilerClass,RendererClass>::VuoBase(string id)
{
//	printf("VuoBase(%p)::VuoBase(%s)\n",this,id.c_str());
	this->id = id;
	compiler = NULL;
	renderer = NULL;
}

/**
 * Sets the base class instance's compiler detail class instance.
 */
template<class CompilerClass, class RendererClass>
void VuoBase<CompilerClass,RendererClass>::setCompiler(CompilerClass *compiler)
{
	this->compiler = compiler;
}

/**
 * Returns the base class instance's compiler detail class instance.
 */
template<class CompilerClass, class RendererClass>
CompilerClass * VuoBase<CompilerClass,RendererClass>::getCompiler(void) const
{
#ifdef DEBUG
	if (!compiler)
	{
		fprintf(stderr, "VuoBase<%s>(%p)::getCompiler() is null\n", id.c_str(), this);
		fflush(stderr);
		VuoLog_backtrace();
	}
#endif
	return compiler;
}

/**
 * Returns true if this base class instance has a compiler detail.
 */
template<class CompilerClass, class RendererClass>
bool VuoBase<CompilerClass,RendererClass>::hasCompiler(void) const
{
	return compiler;
}

/**
 * Sets the base class instance's renderer detail class instance.
 */
template<class CompilerClass, class RendererClass>
void VuoBase<CompilerClass,RendererClass>::setRenderer(RendererClass *renderer)
{
	this->renderer = renderer;
}

/**
 * Returns the base class instance's renderer detail class instance.
 */
template<class CompilerClass, class RendererClass>
RendererClass * VuoBase<CompilerClass,RendererClass>::getRenderer(void) const
{
#ifdef DEBUG
	if (!renderer)
	{
		fprintf(stderr, "VuoBase<%s>(%p)::getRenderer() is null\n", id.c_str(), this);
		fflush(stderr);
		VuoLog_backtrace();
	}
#endif
	return renderer;
}

/**
 * Returns true if this base class instance has a renderer detail.
 */
template<class CompilerClass, class RendererClass>
bool VuoBase<CompilerClass,RendererClass>::hasRenderer(void) const
{
	return renderer;
}


// Realm of Template Voodoo

class VuoCompilerNode;
class VuoRendererNode;
template class VuoBase<VuoCompilerNode, VuoRendererNode>;

class VuoCompilerNodeClass;
template class VuoBase<VuoCompilerNodeClass, void>; // No equivalent Renderer class

class VuoCompilerType;
template class VuoBase<VuoCompilerType, void>;

class VuoCompilerCable;
class VuoRendererCable;
template class VuoBase<VuoCompilerCable, VuoRendererCable>;

class VuoCompilerNodeArgument;
class VuoRendererPort;
template class VuoBase<VuoCompilerNodeArgument, VuoRendererPort>;

class VuoCompilerNodeArgumentClass;
template class VuoBase<VuoCompilerNodeArgumentClass, void>; // No equivalent Renderer class

class VuoCompilerPublishedPort;
class VuoRendererPublishedPort;
template class VuoBase<VuoCompilerPublishedPort, VuoRendererPublishedPort>;

class VuoCompilerComposition;
class VuoRendererComposition;
template class VuoBase<VuoCompilerComposition, VuoRendererComposition>;
