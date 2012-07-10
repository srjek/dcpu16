import sys#, os
#os.environ['PATH'] = "C:\\MinGW\\bin;" + os.environ['PATH']
from PyInline import C
import PyInline
sys.stderr.errors = 'unknown'
sys.stdout.errors = 'unknown'

m = PyInline.build(code="""
  double my_add(double a, double b) {
    return a + b;
  }
  """,
  language="C")
print("3.5 + 5.5: " + repr(m.my_add(3.5, 5.5)))
