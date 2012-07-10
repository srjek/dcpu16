			  Inline for Python
			     release 0.03
			  September 18, 2001


Introduction
------------

The PyInline module allows you to put source code from other
programming languages directly "inline" in a Python script or
module. The code is automatically compiled as needed, and then loaded
for immediate access from Python. PyInline is the Python equivalent of
Brian Ingerson's Inline module for Perl (http://inline.perl.org);
indeed, this README file plagerizes Brian's documentation almost
verbatim.

PyInline saves you from the hassle of having to write and compile your
own glue code using facilities like SWIG or the Python API. Simply
type the code where you want it and run your script as usual. All the
hairy details are handled for you. The compilation and installation of
your code chunks all happen transparently; all you will notice is the
delay of compilation on the first run.


Using PyInline
--------------

Writing extensions with PyInline is easy. The following snippet of
will get you started:

--- cut-here ---
import PyInline

m = PyInline.build(code="""
  double my_add(double a, double b) {
    return a + b;
  }
""", language="C")

print m.my_add(4.5, 5.5) # Should print out "10.0"
----------------

The build function builds a chunk of source code written in a particular
language and returns an object containing the methods, classes, and whatever
other things resulted from the build.

Controlling Compilation
-----------------------

You can control how your PyInline extension is compiled by setting
arguments to the build() function.  For instance, to enable debugging
in the previous example, we can set the "-g" argument using the
"distutils_args" argument:

  ...
  targetmodule=__main__,
  distutils_args=['-g'])
  ...

All entries in the distutils_args list are passed to the distutils
package, which actually builds your inlined C source code. For more
information about the distutils package, see 
http://www.python.org/sigs/distutils-sig/.

To link your extension with additional libraries, use the libraries
argument. For example,

  ...
  targetmodule=__main__,
  libraries=["glib"])

And to specify library directories, use library_dirs, e.g.,

  ...
  targetmodule=__main__,
  library_dirs=["/usr/X11R6/lib"],
  libraries=["X11", "Xt"])

Support for additional distutils compilation controls (such as
include_dirs, extra_objects, extra_compile_args, extra_link_args, and
export_symbols) will be provided in a future release.


Future Work
-----------

PyInline is most certainly a work in progress.
Patches to ksimpson@ttul.org are always greatly appreciated :)

The PyInline TODO list currently includes the following:

 * Expose the entire distutils API so that compilation can be
   completely controlled.

 * Add lots more example code and documentation.

 * Write bindings for other languages, like Perl and C++.

 * And much, much, much more!


Regards,
Ken Simpson
ksimpson@ttul.org

