# PyInline C Module
# Copyright (c)2001 Ken Simpson. All Rights Reserved.

from PyInline import BuildError
from PyInline import c_util
import os, string, re

from distutils.core import setup, Extension

def log(message):
    print(message)

class Builder:
    def __init__(self, **options):
        self._verifyOptions(options)
        self._options = options
        self._initDigest()
        self._initBuildNames()
        self._methods = []

    def _verifyOptions(self, options):
        pass

    def _initDigest(self):
        import hashlib, os, sys
        digester = hashlib.md5()
        digester.update(self._options.get('code').encode())
        self._digest = digester.hexdigest()

    def _initBuildNames(self):
        self._moduleName = "_PyInline_%s" % self._digest
        self._buildDir = os.path.join("_PyInline", self._moduleName)
        self._srcFileName = "%s.c" % self._moduleName
        self._moduleVersion = "1.0"
        self._homeDir = os.getcwd()

    def build(self):
        "Build a chunk of C source code."
        self._parse()

        try:
            if self._options.get("forceBuild"):
                raise ImportError("Build forced by user")
            return self._import()
        except ImportError:
            self._writeModule()
            self._compile()

            try:
                return self._import()
            except ImportError:
                raise BuildError("Build failed")

    def _import(self):
        "Import the new extension module into our client's namespace"
        from distutils.util import get_platform
        import sys, os
        
        # Add the module's lib directory to the Python path.
        plat_specifier = ".%s-%s" % (get_platform(), sys.version[0:3])
        build_platlib = os.path.join(self._buildDir,
                                     'build',
                                     'lib' + plat_specifier)
        sys.path.append(build_platlib)

        # Load the module.
        import imp
        fp, pathname, description = imp.find_module(self._moduleName)

        try:
            module = imp.load_module(self._moduleName, fp,
                                     pathname, description)
        finally:
            # Since we may exit via an exception, close fp explicitly.
            if fp:
                fp.close()

        if 'targetmodule' in self._options:
            # Load each of the module's methods into the caller's
            # global namespace.
            setattr(self._options.get('targetmodule'), self._moduleName, module)
            for method in self._methods:
                setattr(self._options.get('targetmodule'), method['name'],
                        getattr(module, method['name']))
                
        return module

    def _parse(self):
        code = c_util.preProcess(self._options.get('code'))

        defs = c_util.findFunctionDefs(code)
        for d in defs:
            d['params'] = self._parseParams(d['rawparams'])
            self._methods.append(d)

    _commaSpace = re.compile(",\s*")
    _space = re.compile("\s+")
    _spaceStars = re.compile("(?:\s*\*\s*)+")
    _void = re.compile("\s*void\s*")
    _blank = re.compile("\s+")

    def _parseParams(self, params):
        "Return a tuple of tuples describing a list of function params"
        import re, string
        rawparams = self._commaSpace.split(params)
        if self._void.match(params) or\
           self._blank.match(params) or\
           params == '':
            return []

        return [self._parseParam(p) for p in rawparams]

    def _parseParam(self, p):
        param = {}
        
        # Grab the parameter name and its type.
        m = c_util.c_pandm.match(p)
        if not m:
            raise BuildError("Error parsing parameter %s" % p)

        type = self._parseType(m.group(1))
        param['type'] = type['text']
        param['const'] = type['const']
        param['pointers'] = type['pointers']
        param['name'] = m.group(2)

        return param

    def _parseType(self, typeString):
        type = {}
        # Remove const from the type.
        if c_util.const.search(typeString):
            typeString = c_util.const.sub(" ", typeString)
            type['const'] = 1
        else:
            type['const'] = 0

        # Reformat asterisks in the type.
        type['pointers'] = typeString.count('*')
        type['text'] = c_util.trimWhite(c_util.star.sub("", typeString) +\
                                        ("*" * type['pointers']))

        return type
        
    def _makeBuildDirectory(self):
        try:
            os.mkdir("_PyInline")
        except OSError as e:
            # Maybe the _PyInline directory already exists?
            dummy = 42
        try:
            os.mkdir(self._buildDir)
        except OSError as e:
            # Maybe the build directory already exists?
            log("Couldn't create build directory %s" % self._buildDir)

    def _writeModule(self):
        self._makeBuildDirectory()
        try:
            srcFile = open(os.path.join(self._buildDir, self._srcFileName),
                           "w")
        except IOError as e:
            raise BuildError("Couldn't open source file for writing: %s" % e)

        import time
        srcFile.write("// Generated by PyInline\n")
        srcFile.write("// At %s\n\n" %\
	 time.asctime(time.localtime(time.time())))
        srcFile.write('#include "Python.h"\n\n')

        # First, write out the user's code.
        srcFile.write("/* User Code */\n")
        srcFile.write(self._options.get('code'))
        srcFile.write("\n\n")

        # Then add in marshalling methods.
        for method in self._methods:
            #if len(method['params']) == 2 and "".join(method['params'][0]['type'].split(" ")) == "PyObject*" and \
            #            "".join(method['params'][1]['type'].split(" ")) == "PyObject*" and method['return_type'] == "PyObject*":
            #    method['hashname'] = method['name']
            #    continue
            srcFile.write("static PyObject *\n")
            method['hashname'] = "_%s_%s" % (self._digest, method['name'])
            srcFile.write("%s(PyObject *self, PyObject *args)\n" %\
                          method['hashname'])
            self._writeMethodBody(srcFile, method)

        # Finally, write out the method table.
        moduleMethods = "%s_Methods" % self._moduleName
        srcFile.write("static PyMethodDef %s[] = {\n  " %\
                      moduleMethods)
        table = ",\n  ".join(map(lambda x: '{"%s", %s, METH_VARARGS, "%s(%s)"}' %\
                         (x['name'], x['hashname'], x['name'],
                       ', '.join(map(lambda y: '%s' % (y['name']),
                                x['params']))), self._methods))
        if len(self._methods) > 0:
            table += ","
        srcFile.write(table + "\n  ")
        srcFile.write("{NULL, NULL}\n};\n")

        srcFile.write("""
static struct PyModuleDef %s_Module = {
   PyModuleDef_HEAD_INIT, /* m_base */
   "%s",  /* m_name */
   0,  /* m_doc */
   0,  /* m_size */
   %s,  /* m_methods */
   0,  /* m_reload */
   0,  /* m_traverse */
   0,  /* m_clear */
   0,  /* m_free */
};
""" % (self._moduleName, self._moduleName, moduleMethods))
        # And finally an initialization method...
        srcFile.write("""
#ifdef _WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif
DLLEXPORT PyObject* PyInit_%s(void) {
  PyObject* res = PyModule_Create(&%s_Module);
  if (!res) return NULL;
  return res;
}
""" % (self._moduleName, self._moduleName))

        srcFile.close()

    def _writeMethodBody(self, srcFile, method):
        srcFile.write("{\n")

        # Don't write a return value for void functions.
        srcFile.write("  /* Return value */\n")
        if method['return_type'] != 'void':
            srcFile.write("  %s %s;\n\n" % (method['return_type'], "_retval"))
            
        srcFile.write("  /* Function parameters */\n")
        for param in method['params']:
            srcFile.write("  %s %s;\n" % (param['type'], param['name']));
        srcFile.write("\n")

        # Now marshal the input parameters, if there are any.
        if method['params']:
            ptString = _buildPTString(method['params'])
            ptArgs = ", ".join(
                map(lambda x: "&%s" % x['name'],
                    method['params']))
            srcFile.write('  if(!PyArg_ParseTuple(args, "%s", %s))\n' %\
                          (ptString, ptArgs))
            srcFile.write('    return NULL;\n');

        # And fill in the return value by calling the user's code
        # and then filling in the Python return object.
        retvalString = ""
        if method['return_type'] != 'void':
            retvalString = "_retval = "
            
        srcFile.write("  %s%s(%s);\n" %\
                      (retvalString,
                       method['name'],
                       ', '.join(map(lambda x: '%s' % (x['name']),
                                method['params']))))

        if method['return_type'] == 'void':
            srcFile.write("  /* void function. Return None.*/\n")
            srcFile.write("  Py_INCREF(Py_None);\n")
            srcFile.write("  return Py_None;\n")
        elif method['return_type'] == 'PyObject*':
            srcFile.write("  return _retval;\n")
        else:
            try:
                rt = self._parseType(method['return_type'])
                srcFile.write('  return Py_BuildValue("%s", _retval);\n' %\
                              ptStringMap[rt['text']])
            except KeyError:
                raise BuildError("Can't handle return type '%s' in function '%s'"%\
                                 (method['return_type'], method['name']))
        
        srcFile.write("}\n\n")

    def _compile(self):
        from distutils.core import setup, Extension
        os.chdir(self._buildDir)
        ext = Extension(self._moduleName,
                        [self._srcFileName],
                        library_dirs=self._options.get('library_dirs'),
                        libraries=self._options.get('libraries'),
                        define_macros=self._options.get('define_macros'),
                        undef_macros=self._options.get('undef_macros'),
                        extra_objects=self._options.get('extra_objects'),
                        extra_link_args=self._options.get('extra_link_args'))
        try:
            setup(name = self._moduleName,
                  version = self._moduleVersion,
                  ext_modules = [ext],
                  script_args = ["build"] + (self._options.get('distutils_args') or []),
                  script_name="C.py",
                  package_dir=self._buildDir)
        except SystemExit as e:
            raise BuildError(e)
            
        os.chdir(self._homeDir)

ptStringMap = {
    'unsigned': 'i',
    'unsigned int': 'i',
    'int': 'i',
    'long': 'l',
    'float': 'f',
    'double': 'd',
    'char': 'c',
    'short': 'h',
    'char*': 's',
    'PyObject*': 'O'}

def _buildPTString(params):
    ptString = ""
    for param in params:
        if param['type'] in ptStringMap:
            ptString += ptStringMap[param['type']]
        else:
            raise BuildError("Cannot map argument type '%s' for argument '%s'" %\
                             (param['type'], param['name']))

    return ptString


        

