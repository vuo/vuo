/**
 * @file
 * VuoBase interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOBASE_HH
#define VUOBASE_HH

/**
 * This class is intended to be inherited by a base class that can have compiler and/or renderer detail classes.
 *
 * @tparam CompilerClass The base class's compiler detail class.
 * @tparam RendererClass The base class's renderer detail class.
 *
 * @see VuoBaseDetail
 */
template<class CompilerClass, class RendererClass>
class VuoBase
{
public:
   VuoBase(string id);

   void setCompiler(CompilerClass *compiler);
   CompilerClass * getCompiler(void) const;
   bool hasCompiler(void) const;

   void setRenderer(RendererClass *renderer);
   RendererClass * getRenderer(void) const;
   bool hasRenderer(void) const;

private:
   string id;
   CompilerClass * compiler; ///< The base class instance's compiler detail class instance.
   RendererClass * renderer; ///< The base class instance's renderer detail class instance.
};

#endif // VUOBASE_HH
