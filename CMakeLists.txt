cmake_minimum_required(VERSION 3.15)
project(CalcAppAPI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if (ANDROID)
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
    set(CMAKE_ANDROID_STL_TYPE c++_static)
endif()

find_package(DotFont CONFIG REQUIRED)
find_package(BitmapTool CONFIG REQUIRED)
find_package(SciCalc CONFIG REQUIRED)
find_package(MathRenderer CONFIG REQUIRED)
find_package(DrawGraph CONFIG REQUIRED)
find_package(PentominoBMP CONFIG REQUIRED)

set(SRC
    src/calc_app_api.cpp
)

if(MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic")
endif()

add_library(${PROJECT_NAME} STATIC ${SRC})
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(${PROJECT_NAME} PUBLIC
    DotFont::DotFont
    BitmapTool::BitmapTool
    SciCalc::SciCalc
    MathRenderer::MathRenderer
    DrawGraph::DrawGraph
    PentominoBMP::PentominoBMP
)

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include FILES_MATCHING PATTERN *.hpp)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ DESTINATION include FILES_MATCHING PATTERN *.h)
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION lib/cmake/${PROJECT_NAME}
)

include(CMakePackageConfigHelpers)
file(WRITE ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake.in
    "
@PACKAGE_INIT@
include(CMakeFindDependencyMacro)
find_dependency(SciCalc CONFIG REQUIRED)
find_dependency(MathRenderer CONFIG REQUIRED)
find_dependency(DrawGraph CONFIG REQUIRED)
find_dependency(PentominoBMP CONFIG REQUIRED)
include(\${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}Targets.cmake)
check_required_components(${PROJECT_NAME})
    ")  # edit
configure_package_config_file(
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake.in
    ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
)
install(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake/${PROJECT_NAME})

