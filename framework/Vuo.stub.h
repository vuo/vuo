/**
 * @file
 * Vuo prefix header.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if (__clang_major__ == 3 && __clang_minor__ >= 2) || __clang_major__ > 3
	#define VUO_CLANG_32_OR_LATER
#endif

#pragma clang diagnostic push
#ifdef VUO_CLANG_32_OR_LATER
	#pragma clang diagnostic ignored "-Wdocumentation"
#endif
#include "json-c/json.h"
#pragma clang diagnostic pop
#undef hexdigit

#include "zmq/zmq.h"

#ifdef __cplusplus

#include <string>
#include <vector>
#include <map>
#include <list>
#include <set>

using namespace std;

#include "VuoRunner.hh"
#include "VuoComposition.hh"
#include "VuoFileUtilities.hh"
#include "VuoPort.hh"
#include "VuoProtocol.hh"

#define __STDC_CONSTANT_MACROS
#define __STDC_FORMAT_MACROS
#define __STDC_LIMIT_MACROS

#ifdef NO
	#define VUO_NO_ALREADY_DEFINED
	#undef NO
#endif
#define NO VUO_LLVM_NO_RENAMED

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnull-dereference"
#ifdef VUO_CLANG_32_OR_LATER
	#pragma clang diagnostic ignored "-Wunused-private-field"
#endif
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#include <clang/Basic/Version.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <llvm/Option/ArgList.h>
#include <clang/Driver/Compilation.h>
#include <clang/Driver/Driver.h>
#include <clang/Driver/ToolChain.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/FrontendDiagnostic.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Object/Archive.h>
#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/ValueSymbolTable.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Linker/Linker.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/Utils/Cloning.h>
#pragma clang diagnostic pop

#undef NO
#ifdef VUO_NO_ALREADY_DEFINED
	#undef VUO_NO_ALREADY_DEFINED

	// From /usr/include/objc/objc.h line 58
	#if __has_feature(objc_bool)
		#define NO              __objc_no
	#else
		#define NO              ((BOOL)0)
	#endif
#endif

using namespace llvm;

@INCLUDE_VUO_CXX_HEADERS@

#endif // ifdef __cplusplus

@INCLUDE_VUO_C_HEADERS@
