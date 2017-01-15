/**
 * @file
 * VuoCompilerPublishedPortClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERPUBLISHEDPORTCLASS_HH
#define VUOCOMPILERPUBLISHEDPORTCLASS_HH

#include "VuoPortClass.hh"
#include "VuoCompilerPortClass.hh"

class VuoCompilerPort;
class VuoType;

/**
 * A published port type.
 */
class VuoCompilerPublishedPortClass : public VuoCompilerPortClass
{
public:
	VuoCompilerPublishedPortClass(string name, VuoPortClass::PortType portType, Type *type);
	VuoCompilerPort * newPort(void);
	VuoCompilerPort * newPort(VuoPort *port);
	VuoType * getDataVuoType(void);
	void setDataVuoType(VuoType *type);
	string getDisplayName(void);
	void setDetail(string key, string value);
	void unsetDetail(string key);

private:
	VuoType *vuoType;
};

#endif // VUOCOMPILERPUBLISHEDPORTCLASS_HH
