#!/usr/bin/env python

# Setup script for the Inline for Python package.

# created August 3, 2001 by Ken Simpson <ksimpson@ttul.org>

__revision__ = "$Id: setup.py,v 1.2 2001/08/29 17:51:04 ttul Exp $"

from distutils.core import setup

setup (name = "PyInline",
       version = "0.03",
       description = "Inline for Python",
       author = "Ken Simpson",
       author_email = "ksimpson@ttul.org",
       maintainer = "Ken Simpson",
       maintainer_email = 'ksimpson@ttul.org',
       url = "http://ttul.org/~ksimpson/PyInline",
       licence = "Artistic",
       long_description = """\
A system which allows you to "inline" code written for other
programming languages directly within your Python scripts.""",

       # This implies all pure Python modules in ./PyInline/.
       packages = ['PyInline'],
      )
