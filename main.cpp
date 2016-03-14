extern "C" {
#include <Python.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
}

#include <cstdint>

static PyObject * lua_eval(PyObject *self, PyObject *args)
{
    const char *command;
    
    // args -- tuple
    PyObject* first_arg = PyTuple_GetItem(args, 0);
    
    command = PyUnicode_AsUTF8(first_arg);
    
    lua_State *lState = luaL_newstate();
    luaL_openlibs(lState);
    
    luaL_loadbuffer(lState, command, strlen(command), NULL);
    
    lua_pcall(lState, 0, 0, 0);
    lua_close(lState);
    
    Py_INCREF(Py_None);
    return Py_None;
}


static PyMethodDef LuaMethods[] = {
    {"eval",  lua_eval, METH_VARARGS, "Evaluate Lua code."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef lua_module = {
    PyModuleDef_HEAD_INIT,
    "lua",   /* name of module */
    "eval for lua", /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
               or -1 if the module keeps state in global variables. */
    LuaMethods
};

PyMODINIT_FUNC PyInit_lua(void)
{
    return PyModule_Create(&lua_module);
}