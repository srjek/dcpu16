#include "cpu.h"

wxWindow* getTopLevelWindow() {
   return (*((wxApp*) (&wxGetApp()))).GetTopWindow();
}