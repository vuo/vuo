cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)

find_package(glfw3 REQUIRED)

set(CMAKE_FRAMEWORK_PATH ../../../framework)
find_library(VUO_FRAMEWORK Vuo)

function (buildVariant target)
	add_executable(${target} RunImageFilter.cc)
	target_compile_features(${target} PRIVATE cxx_std_11)
	target_compile_definitions(${target} PRIVATE
		SOURCE_DIR="${PROJECT_SOURCE_DIR}"
		${ARGN}
	)
	target_link_directories(${target} PRIVATE ${VUO_FRAMEWORK}/Modules)
	target_link_libraries(${target}
		"-framework CoreFoundation"
		"-framework OpenGL"
		glfw
		${VUO_FRAMEWORK}
	)
endfunction()

buildVariant(RunImageFilter-current-makeFromJson                RUNIMAGEFILTER_PROCESS_CURRENT  RUNIMAGEFILTER_USE_MAKEFROMJSON               )
buildVariant(RunImageFilter-current-makeFromJsonWithDimensions  RUNIMAGEFILTER_PROCESS_CURRENT  RUNIMAGEFILTER_USE_MAKEFROMJSONWITHDIMENSIONS )
buildVariant(RunImageFilter-separate-makeFromJson               RUNIMAGEFILTER_PROCESS_SEPARATE RUNIMAGEFILTER_USE_MAKEFROMJSON               )
buildVariant(RunImageFilter-separate-makeFromJsonWithDimensions RUNIMAGEFILTER_PROCESS_SEPARATE RUNIMAGEFILTER_USE_MAKEFROMJSONWITHDIMENSIONS )
buildVariant(RunImageFilter-separate-interprocessToFramebuffer  RUNIMAGEFILTER_PROCESS_SEPARATE RUNIMAGEFILTER_USE_INTERPROCESS_TO_FRAMEBUFFER)
buildVariant(RunImageFilter-separate-interprocessToTexture      RUNIMAGEFILTER_PROCESS_SEPARATE RUNIMAGEFILTER_USE_INTERPROCESS_TO_TEXTURE    )
