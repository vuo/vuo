/**
 * @file
 * Vuo32 prefix header.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUO32_H
#define VUO32_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json/json.h>
#pragma clang diagnostic pop

#include <zmq/zmq.h>


#ifdef __cplusplus

#include <string>
#include <vector>
#include <map>
#include <list>
#include <set>

using namespace std;

#include "VuoRunner32.hh"

#endif


#ifdef __cplusplus
extern "C"
{
#endif
    
#include "VuoGlContext.h"
#include "VuoGlPool.h"
#include "VuoHeap.h"
#include "VuoImageRenderer.h"
#include "VuoBoolean.h"
#include "VuoInteger.h"
#include "VuoImage.h"
#include "VuoReal.h"
#include "VuoText.h"
        
#ifdef __cplusplus
}
#endif


#endif
