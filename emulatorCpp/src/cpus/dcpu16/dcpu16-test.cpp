#include <cstdlib>
#include <iostream>

#include "dcpu16-test.h"
#include "dcpu16-cpu.h"
#include "../../test.h"
#include "tests/dcpu16_tests.h"

//Loads the test identied by test_num into a dcpu16
//the last num_args words are treated as offsets into the program,
//replaces the words at each offset with the corresponding arg
//executes and returns state
void runTest(int test_num, int num_args, unsigned short* args) {
    //TODO: use test_num
    char* bin = dcpu16_SET_bin;
    size_t size = dcpu16_SET_bin_size - num_args*2;
    
    dcpu16* test_comp = new dcpu16(true);
    
    test_comp->loadImage(size, bin);
    
    
    test_comp->cycle(1);
    test_comp->cycle(test_comp->time-1);
    
    delete test_comp;
}

bool dcpu16_runTest() {
    char testBuffer[100];
    
    std::cout << "Testing operation SET: ";
    for (int i = 0; i < 10; i++) {
        unsigned short r = (32 + rand() % ((1 >> 16)-32)) & 0xFFFF;
        unsigned short r2 = (-1 + rand() % 32) & 0xFFFF;
        
        unsigned short args[2];
        args[0] = r;
        args[1] = 0x01 | (0x00 << 5) | (((r2+1) & 0xFFFF) << 10);   //SET A, <r2> (short literal form)
        
        /*state =*/ runTest(0, 2, args);
        /* if state[0].A != r:
            std::cout << TEST_FAIL << std::endl;
            std::cout << "\tCould not set register A to literal " << r << ". Register was " << state.A << " instead" << std::endl;
            return false;
        if state[1].A != r2:
            std::cout << TEST_FAIL << std::endl;
            std::cout << "\tCould not set register A to short literal " << r2 << ". Register was " << state.A << " instead" << std::endl;
            return false; */
    }
    std::cout << TEST_SUCCESS << std::endl;
    return true;
}
