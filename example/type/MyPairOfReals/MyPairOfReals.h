/**
 * @file
 * MyPairOfReals C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef MYPAIROFREALS_H
#define MYPAIROFREALS_H

#include "VuoReal.h"

typedef struct
{
	VuoReal first,second;
} MyPairOfReals;

MyPairOfReals MyPairOfReals_makeFromJson(struct json_object * js);
struct json_object * MyPairOfReals_getJson(const MyPairOfReals value);
char * MyPairOfReals_getSummary(const MyPairOfReals value);

MyPairOfReals MyPairOfReals_makeFromString(const char *str);
char * MyPairOfReals_getString(const MyPairOfReals value);

static inline MyPairOfReals MyPairOfReals_make(VuoReal first, VuoReal second) __attribute__((const));
static inline MyPairOfReals MyPairOfReals_make(VuoReal first, VuoReal second)
{
	MyPairOfReals p = {first,second};
	return p;
}

#endif
