cmake_minimum_required(VERSION 3.0)
set(CMAKE_VERBOSE_MAKEFILE on)
find_package(PythonLibs 3.2 REQUIRED)
find_package(Lua)
include_directories(${LUA_INCLUDE_DIR})
include_directories(${PYTHON_INCLUDE_DIRS})
python_add_module(lua main.cpp)
target_link_libraries(lua ${PYTHON_LIBRARIES} ${LUA_LIBRARY})
