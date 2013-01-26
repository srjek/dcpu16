#define NO_WXMAIN
#include "test.h"

#include <iostream>
#include <cstdlib>

#include "wx/wx.h"
#include "cpus/cpus.h"

wxApp* wxGetApp() {
    return NULL;
}

int main() {
    if (!wxInitialize()) {
        std::cerr << "wxWidgets could not be initialized";
        return -1;
    }
    
    for (int i = 0; cpu_names[i] != NULL; i++) {
        std::cout << "Testing " << cpu_names[i] << "..." << std::endl;
        runCpuTest(cpu_names[i]);
    }

    return 0;
}
