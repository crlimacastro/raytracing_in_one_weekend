cmake_minimum_required(VERSION 3.31.1)
project(raytracing_in_one_weekend)

file(GLOB_RECURSE SRCS CONFIGURE_DEPENDS "src/*.h" "src/*.hpp" "src/*.cpp")

add_executable(${PROJECT_NAME} ${SRCS})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PRIVATE STB_IMAGE_WRITE_IMPLEMENTATION)
target_include_directories(${PROJECT_NAME} PRIVATE src)
