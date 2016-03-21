extern "C" {
#include <Python.h>
#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <lualib.h>
#include <string.h>
}

#include <cstdint>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <vector>
#include <string>



static void RefreshDict(lua_State *lState, PyObject *dictionary) {
    Py_ssize_t position = 0;
    PyObject *key, *value;
    if (dictionary == Py_None) {
        return;
    }
    while (PyDict_Next(dictionary, &position, &key, &value)) {
        if (PyUnicode_Check(key)) {
            lua_getglobal(lState, PyUnicode_AsUTF8(key));
            if (!lua_isboolean(lState, -1) &&
                !lua_isnumber(lState, -1) &&
                !lua_isstring(lState, -1)) {
            } else {
                PyObject *real_value;
                if (lua_isboolean(lState, -1)) {
                    real_value = PyBool_FromLong(lua_toboolean(lState, -1));
                } else if (lua_isnumber(lState, -1)) {
                    real_value = PyFloat_FromDouble(lua_tonumber(lState, -1));
                } else {
                    real_value = PyUnicode_FromString(lua_tostring(lState, -1));
                }
                PyDict_SetItem(dictionary, key, real_value);
                Py_DECREF(real_value);
            }
            lua_pop(lState, 1);
        }
    }
}

static bool CheckVar(lua_State *lState, PyObject *value) {
    if (value == Py_None) {
        lua_pushnil(lState);
        return true;
    }
    if (PyLong_Check(value)) {
        double real_value = PyLong_AsDouble(value);
        lua_pushnumber(lState, real_value);
        return true;
    }
    if (PyFloat_Check(value)) {
        double real_value = PyFloat_AsDouble(value);
        lua_pushnumber(lState, real_value);
        return true;
    }
    if (PyUnicode_Check(value)) {
        lua_pushstring(lState, PyUnicode_AsUTF8(value));
        return true;
    }
    return false;
}

void KMP(std::string custom_str) {
    
    int length = static_cast<int>(custom_str.size());
    std::vector<int> prefix_func(length, 0);
    std::cout << "0 ";
    for (int position = 1; position < length; ++position) {
        int answer = prefix_func[position - 1];
        while (answer > 0 && custom_str[position] != custom_str[answer]) {
            answer = prefix_func[answer - 1];
        }
        if (custom_str[position] == custom_str[answer]) {
            ++answer;
        }
        prefix_func[position] = answer;
        std::cout << answer << " ";
    }
}

static void put_variables(lua_State *lState, PyObject *dictionary) {
    Py_ssize_t position = 0;
    PyObject *key, *value;
    while (PyDict_Next(dictionary, &position, &key, &value)) {
        if (PyUnicode_Check(key) && CheckVar(lState, value)) {
            lua_setglobal(lState, PyUnicode_AsUTF8(key));
        }
    }
}

void set_exception(const char *error_name) {
    PyErr_SetString(PyExc_ValueError, error_name);
}

static PyObject *lua_eval(PyObject *self, char *prog, PyObject *glob, PyObject *loc) {
    lua_State *lState = luaL_newstate();
    luaL_openlibs(lState);
    
    put_variables(lState, glob);
    if (loc != Py_None) {
        put_variables(lState, loc);
    }
    
    if (luaL_loadstring(lState, prog)) {
        set_exception(lua_tostring(lState, -1));
        lua_close(lState);
        return NULL;
    }
    
    if (lua_pcall(lState, 0, 1, 0) == 0) {
        RefreshDict(lState, glob);
        RefreshDict(lState, loc);
        if (lua_isnil(lState, -1)) {
            lua_close(lState);
            Py_RETURN_NONE;
        } else {
            if (!lua_isboolean(lState, -1) &&
                !lua_isnumber(lState, -1) &&
                !lua_isstring(lState, -1)) {
                set_exception("This isn't a scalar result!");
                lua_close(lState);
                return NULL;
            } else {
                PyObject *output;
                if (lua_isboolean(lState, -1)) {
                    output = PyBool_FromLong(lua_toboolean(lState, -1));
                } else if (lua_isnumber(lState, -1)) {
                    output = PyFloat_FromDouble(lua_tonumber(lState, -1));
                } else {
                    output = PyUnicode_FromString(lua_tostring(lState, -1));
                }
                lua_close(lState);
                return output;
            }
        }
    } else {
        set_exception(lua_tostring(lState, -1));
        lua_close(lState);
        return NULL;
    }
}

static PyObject *eval(PyObject *self, PyObject *args, PyObject *key_args) {
    PyObject *glob = Py_None, *loc = Py_None;
    const char *expr;
    static char *params[4];
    params[0] = const_cast<char*>("expr");
    params[1] = const_cast<char*>("glob");
    params[2] = const_cast<char*>("loc");
    params[3] = NULL;
    
    if (PyArg_ParseTupleAndKeywords(args, key_args, "s|OO", params, &expr, &glob, &loc)) {
        char *prog;
        if (strchr(expr, '\n') == NULL) {
            prog = reinterpret_cast<char*>(calloc(strlen(expr) + 8, 1));
            strcpy(prog, const_cast<char*>("return "));
            strcat(prog, expr);
        } else {
            prog = reinterpret_cast<char*>(calloc(strlen(expr) + 1, 1));
            strcpy(prog, expr);
        }
        if (glob == Py_None)
            glob = PyEval_GetGlobals();
        PyObject *result = lua_eval(self, prog, glob, loc);
        free(prog);
        return result;
    } else {
        return NULL;
    }
}

static PyMethodDef LuaMethods[] = {
    {"eval", (PyCFunction)eval, METH_VARARGS | METH_KEYWORDS, "Evaluate lua expressions"},
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

PyMODINIT_FUNC PyInit_lua(void) {
    return PyModule_Create(&lua_module);
}
