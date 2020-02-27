# LLVM and Clang need to be in their own frameworks,
# so Vuo.framework clients can simply `#include <Vuo/Vuo.h>`
# without having to add separate header search paths
# for LLVM and Clang's subfolders.

if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/empty.c)
	file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/empty.c "")
endif()


add_library(VuoLLVMFramework SHARED
	empty.c
)
set_target_properties(VuoLLVMFramework PROPERTIES
	OUTPUT_NAME "llvm"
	FRAMEWORK TRUE

	# We just want cmake to generate the structure; we don't need to build/link empty.c.
	EXCLUDE_FROM_ALL TRUE
)
set(llvmFrameworkPath "${PROJECT_BINARY_DIR}/lib/llvm.framework")
set(llvmFrameworkVersionPath "${llvmFrameworkPath}/Versions/A")
if (NOT EXISTS "${llvmFrameworkVersionPath}/llvm")
file(GLOB llvmHeaders ${CONAN_LLVM_ROOT}/include/llvm/*)
file(COPY ${llvmHeaders}                           DESTINATION "${llvmFrameworkVersionPath}/Headers")
file(COPY ${CONAN_LLVM_ROOT}/lib/clang/3.3/include DESTINATION "${llvmFrameworkVersionPath}/lib/clang/3.3")
file(COPY ${CONAN_LLVM_ROOT}/include/c++           DESTINATION "${llvmFrameworkVersionPath}/Headers/lib")
file(CREATE_LINK "Versions/Current/Headers" "${llvmFrameworkPath}/Headers" SYMBOLIC)
file(CREATE_LINK "A" "${llvmFrameworkPath}/Versions/Current" SYMBOLIC)
file(TOUCH "${llvmFrameworkVersionPath}/llvm")
endif()
set_target_properties(VuoLLVMFramework PROPERTIES VuoGeneratedFiles "${llvmFrameworkVersionPath}/llvm")


add_library(VuoLLVMCFramework SHARED
	empty.c
)
set_target_properties(VuoLLVMCFramework PROPERTIES
	OUTPUT_NAME "llvm-c"
	FRAMEWORK TRUE

	# We just want cmake to generate the structure; we don't need to build/link empty.c.
	EXCLUDE_FROM_ALL TRUE
)
set(llvmcFrameworkPath "${PROJECT_BINARY_DIR}/lib/llvm-c.framework")
set(llvmcFrameworkVersionPath "${llvmcFrameworkPath}/Versions/A")
if (NOT EXISTS "${llvmcFrameworkVersionPath}/llvm-c")
file(GLOB llvmcHeaders ${CONAN_LLVM_ROOT}/include/llvm-c/*)
file(COPY ${llvmcHeaders} DESTINATION "${llvmcFrameworkVersionPath}/Headers")
file(CREATE_LINK "Versions/Current/Headers" "${llvmcFrameworkPath}/Headers" SYMBOLIC)
file(CREATE_LINK "A" "${llvmcFrameworkPath}/Versions/Current" SYMBOLIC)
file(TOUCH "${llvmcFrameworkVersionPath}/llvm-c")
endif()
set_target_properties(VuoLLVMCFramework PROPERTIES VuoGeneratedFiles "${llvmcFrameworkVersionPath}/llvm-c")


add_library(VuoClangFramework SHARED
	empty.c
)
set_target_properties(VuoClangFramework PROPERTIES
	OUTPUT_NAME "clang"
	FRAMEWORK TRUE

	# We just want cmake to generate the structure; we don't need to build/link empty.c.
	EXCLUDE_FROM_ALL TRUE
)
set(clangFrameworkPath "${PROJECT_BINARY_DIR}/lib/clang.framework")
set(clangFrameworkVersionPath "${clangFrameworkPath}/Versions/A")
if (NOT EXISTS "${clangFrameworkVersionPath}/clang")
file(GLOB llvmcHeaders ${CONAN_LLVM_ROOT}/include/clang/*)
file(COPY ${llvmcHeaders} DESTINATION "${clangFrameworkVersionPath}/Headers")
file(CREATE_LINK "Versions/Current/Headers" "${clangFrameworkPath}/Headers" SYMBOLIC)
file(CREATE_LINK "A" "${clangFrameworkPath}/Versions/Current" SYMBOLIC)
file(TOUCH "${clangFrameworkVersionPath}/clang")
endif()
set_target_properties(VuoClangFramework PROPERTIES VuoGeneratedFiles "${clangFrameworkVersionPath}/clang")
