/**
 * @file
 * VuoBase implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
/** \fn VuoBase<VuoCompilerNode, VuoRendererNode>::VuoBase
 * Creates a VuoNode base class.
 */
/** \fn VuoBase<VuoCompilerNode, VuoRendererNode>::setCompiler
 * Sets the VuoNode base class instance's VuoCompilerNode detail class instance.
 */
/** \fn VuoBase<VuoCompilerNode, VuoRendererNode>::getCompiler
 * Returns the VuoNode base class instance's VuoCompilerNode detail class instance.
 */
/** \fn VuoBase<VuoCompilerNode, VuoRendererNode>::hasCompiler
 * Returns true if this base class instance has a compiler detail.
 */
/** \fn VuoBase<VuoCompilerNode, VuoRendererNode>::setRenderer
 * Sets the VuoNode base class instance's VuoRendererNode detail class instance.
 */
/** \fn VuoBase<VuoCompilerNode, VuoRendererNode>::getRenderer
 * Returns the VuoNode base class instance's VuoRendererNode detail class instance.
 */
/** \fn VuoBase<VuoCompilerNode, VuoRendererNode>::hasRenderer
 * Returns true if this base class instance has a renderer detail.
 */

class VuoCompilerNodeClass;
template class VuoBase<VuoCompilerNodeClass, void>; // No equivalent Renderer class
/** \fn VuoBase<VuoCompilerNodeClass, void>::VuoBase
 * Creates a VuoNodeClass base class.
 */
/** \fn VuoBase<VuoCompilerNodeClass, void>::setCompiler
 * Sets the VuoNodeClass base class instance's VuoCompilerNodeClass detail class instance.
 */
/** \fn VuoBase<VuoCompilerNodeClass, void>::getCompiler
 * Returns the VuoNodeClass base class instance's VuoCompilerNodeClass detail class instance.
 */
/** \fn VuoBase<VuoCompilerNodeClass, void>::hasCompiler
 * Returns true if this base class instance has a compiler detail.
 */
/** \fn VuoBase<VuoCompilerNodeClass, void>::setRenderer
 * Does nothing.
 */
/** \fn VuoBase<VuoCompilerNodeClass, void>::getRenderer
 * Does nothing.
 */
/** \fn VuoBase<VuoCompilerNodeClass, void>::hasRenderer
 * Returns false.
 */

class VuoCompilerType;
template class VuoBase<VuoCompilerType, void>;
/** \fn VuoBase<VuoCompilerType, void>::VuoBase
 * Creates a VuoType base class.
 */
/** \fn VuoBase<VuoCompilerType, void>::setCompiler
 * Sets the VuoType base class instance's VuoCompilerType detail class instance.
 */
/** \fn VuoBase<VuoCompilerType, void>::getCompiler
 * Returns the VuoType base class instance's VuoCompilerType detail class instance.
 */
/** \fn VuoBase<VuoCompilerType, void>::hasCompiler
 * Returns true if this base class instance has a compiler detail.
 */
/** \fn VuoBase<VuoCompilerType, void>::setRenderer
 * Does nothing.
 */
/** \fn VuoBase<VuoCompilerType, void>::getRenderer
 * Does nothing.
 */
/** \fn VuoBase<VuoCompilerType, void>::hasRenderer
 * Returns false.
 */

class VuoCompilerCable;
class VuoRendererCable;
template class VuoBase<VuoCompilerCable, VuoRendererCable>;
/** \fn VuoBase<VuoCompilerCable, VuoRendererCable>::VuoBase
 * Creates a VuoCable base class.
 */
/** \fn VuoBase<VuoCompilerCable, VuoRendererCable>::setCompiler
 * Sets the VuoCable base class instance's VuoCompilerCable detail class instance.
 */
/** \fn VuoBase<VuoCompilerCable, VuoRendererCable>::getCompiler
 * Returns the VuoCable base class instance's VuoCompilerCable detail class instance.
 */
/** \fn VuoBase<VuoCompilerCable, VuoRendererCable>::hasCompiler
 * Returns true if this base class instance has a compiler detail.
 */
/** \fn VuoBase<VuoCompilerCable, VuoRendererCable>::setRenderer
 * Sets the VuoCable base class instance's VuoRendererCable detail class instance.
 */
/** \fn VuoBase<VuoCompilerCable, VuoRendererCable>::getRenderer
 * Returns the VuoCable base class instance's VuoRendererCable detail class instance.
 */
/** \fn VuoBase<VuoCompilerCable, VuoRendererCable>::hasRenderer
 * Returns true if this base class instance has a renderer detail.
 */

class VuoCompilerNodeArgument;
class VuoRendererPort;
template class VuoBase<VuoCompilerNodeArgument, VuoRendererPort>;
/** \fn VuoBase<VuoCompilerNodeArgument, VuoRendererPort>::VuoBase
 * Creates a VuoPort base class.
 */
/** \fn VuoBase<VuoCompilerNodeArgument, VuoRendererPort>::setCompiler
 * Sets the VuoPort base class instance's VuoCompilerNodeArgument detail class instance.
 */
/** \fn VuoBase<VuoCompilerNodeArgument, VuoRendererPort>::getCompiler
 * Returns the VuoPort base class instance's VuoCompilerNodeArgument detail class instance.
 */
/** \fn VuoBase<VuoCompilerNodeArgument, VuoRendererPort>::hasCompiler
 * Returns true if this base class instance has a compiler detail.
 */
/** \fn VuoBase<VuoCompilerNodeArgument, VuoRendererPort>::setRenderer
 * Sets the VuoPort base class instance's VuoRendererPort detail class instance.
 */
/** \fn VuoBase<VuoCompilerNodeArgument, VuoRendererPort>::getRenderer
 * Returns the VuoPort base class instance's VuoRendererPort detail class instance.
 */
/** \fn VuoBase<VuoCompilerNodeArgument, VuoRendererPort>::hasRenderer
 * Returns true if this base class instance has a renderer detail.
 */

class VuoCompilerNodeArgumentClass;
template class VuoBase<VuoCompilerNodeArgumentClass, void>; // No equivalent Renderer class
/** \fn VuoBase<VuoCompilerNodeArgumentClass, void>::VuoBase
 * Creates a VuoPortClass base class.
 */
/** \fn VuoBase<VuoCompilerNodeArgumentClass, void>::setCompiler
 * Sets the VuoPortClass base class instance's VuoCompilerNodeArgumentClass detail class instance.
 */
/** \fn VuoBase<VuoCompilerNodeArgumentClass, void>::getCompiler
 * Returns the VuoPortClass base class instance's VuoCompilerNodeArgumentClass detail class instance.
 */
/** \fn VuoBase<VuoCompilerNodeArgumentClass, void>::hasCompiler
 * Returns true if this base class instance has a compiler detail.
 */
/** \fn VuoBase<VuoCompilerNodeArgumentClass, void>::setRenderer
 * Does nothing.
 */
/** \fn VuoBase<VuoCompilerNodeArgumentClass, void>::getRenderer
 * Does nothing.
 */
/** \fn VuoBase<VuoCompilerNodeArgumentClass, void>::hasRenderer
 * Returns false.
 */

class VuoCompilerPublishedPort;
class VuoRendererPublishedPort;
template class VuoBase<VuoCompilerPublishedPort, VuoRendererPublishedPort>;
/** \fn VuoBase<VuoCompilerPublishedPort, VuoRendererPublishedPort>::VuoBase
 * Creates a VuoPublishedPort base class.
 */
/** \fn VuoBase<VuoCompilerPublishedPort, VuoRendererPublishedPort>::setCompiler
 * Sets the VuoPublishedPort base class instance's VuoCompilerPublishedPort detail class instance.
 */
/** \fn VuoBase<VuoCompilerPublishedPort, VuoRendererPublishedPort>::getCompiler
 * Returns the VuoPublishedPort base class instance's VuoCompilerPublishedPort detail class instance.
 */
/** \fn VuoBase<VuoCompilerPublishedPort, VuoRendererPublishedPort>::hasCompiler
 * Returns true if this base class instance has a compiler detail.
 */
/** \fn VuoBase<VuoCompilerPublishedPort, VuoRendererPublishedPort>::setRenderer
 * Sets the VuoPublishedPort base class instance's VuoRendererPublishedPort detail class instance.
 */
/** \fn VuoBase<VuoCompilerPublishedPort, VuoRendererPublishedPort>::getRenderer
 * Returns the VuoPublishedPort base class instance's VuoRendererPublishedPort detail class instance.
 */
/** \fn VuoBase<VuoCompilerPublishedPort, VuoRendererPublishedPort>::hasRenderer
 * Returns true if this base class instance has a renderer detail.
 */

class VuoCompilerComposition;
class VuoRendererComposition;
template class VuoBase<VuoCompilerComposition, VuoRendererComposition>;
/** \fn VuoBase<VuoCompilerComposition, VuoRendererComposition>::VuoBase
 * Creates a VuoComposition base class.
 */
/** \fn VuoBase<VuoCompilerComposition, VuoRendererComposition>::setCompiler
 * Sets the VuoComposition base class instance's VuoCompilerComposition detail class instance.
 */
/** \fn VuoBase<VuoCompilerComposition, VuoRendererComposition>::getCompiler
 * Returns the VuoComposition base class instance's VuoCompilerComposition detail class instance.
 */
/** \fn VuoBase<VuoCompilerComposition, VuoRendererComposition>::hasCompiler
 * Returns true if this base class instance has a compiler detail.
 */
/** \fn VuoBase<VuoCompilerComposition, VuoRendererComposition>::setRenderer
 * Sets the VuoComposition base class instance's VuoRendererComposition detail class instance.
 */
/** \fn VuoBase<VuoCompilerComposition, VuoRendererComposition>::getRenderer
 * Returns the VuoComposition base class instance's VuoRendererComposition detail class instance.
 */
/** \fn VuoBase<VuoCompilerComposition, VuoRendererComposition>::hasRenderer
 * Returns true if this base class instance has a renderer detail.
 */
