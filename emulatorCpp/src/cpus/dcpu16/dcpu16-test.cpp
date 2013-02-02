#include <cstdlib>
#include <iostream>
#include <sstream>
#include <vector>

#include "dcpu16-test.h"
#include "dcpu16-cpu.h"
#include "../../test.h"
#include "tests/dcpu16_tests.h"

class dcpu16_state {
public:
    unsigned short ram[0x10000];
    unsigned short registers[DCPU16_NUM_REGS];
    dcpu16_state(dcpu16* cpu) {
        for (int i = 0; i < 0x10000; i++)
            ram[i] = cpu->ram[i];
        for (int i = 0; i < DCPU16_NUM_REGS; i++)
            registers[i] = cpu->registers[i];
    }
};

//Loads the test identied by test_num into a dcpu16
//the last num_args words are treated as offsets into the program,
//replaces the words at each offset with the corresponding arg
//executes and returns state
void runTest(std::vector<dcpu16_state*>& stateList, size_t bin_size, char* bin, int num_args, unsigned short* args, int num_instructions) {
    
    dcpu16* test_comp = new dcpu16(false);
    //Load the image
    test_comp->loadImage(bin_size, bin);
    size_t size = bin_size >> 1; //Use the num of 16bit values instead of the num of 8bit values
    
    for (int i = 0; i < num_args; i++) {
        unsigned long pos = size-num_args+i;
        unsigned short val = test_comp->getRam(pos);
        test_comp->setRam(val, args[i]);
        test_comp->setRam(pos, 0);
    }
    
    unsigned long long initialTime = test_comp->time;
    for (int i = 0; i < num_instructions; i++) {
        test_comp->cycle(1);
        test_comp->cycle(test_comp->time-initialTime-1);
        initialTime = test_comp->time;
        stateList.push_back(new dcpu16_state(test_comp));
    }
    
    delete test_comp;
}

void clearStateList(std::vector<dcpu16_state*>& stateList) {
    for (int i = 0; i < stateList.size(); i++)
        delete stateList[i];
    stateList.clear();
}

bool dcpu16_runTest_inner(std::vector<dcpu16_state*>& stateList) {
    
    std::cout << "\tTesting operation SET: " << std::flush;
    for (int i = 0; i < 100; i++) {
        unsigned short r = (32 + rand() % ((1 << 16)-32)) & 0xFFFF;
        unsigned short r2 = (-1 + rand() % 32) & 0xFFFF;
        
        unsigned short args[2];
        args[0] = r;
        args[1] = 0x01 | (0x00 << 5) | ((((r2+1) & 0xFFFF) | 0x20) << 10);   //SET A, <r2> (short literal form)

        runTest(stateList, dcpu16_SET_bin_size, dcpu16_SET_bin, 2, args, 2);
        
        if (stateList[0]->registers[DCPU16_REG_A] != r) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << "\t\tCould not set register A to literal " << r << ". Register was " << stateList[0]->registers[DCPU16_REG_A] << " instead" << std::endl;
            return false;
        }
        if (stateList[1]->registers[DCPU16_REG_A] != r2) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << "\t\tCould not set register A to short literal " << r2 << ". Register was " << stateList[0]->registers[DCPU16_REG_A] << " instead" << std::endl;
            return false;
        }
        clearStateList(stateList);
    }
    std::cout << TEST_SUCCESS << std::endl;
    
    std::cout << "\tTesting various operands using SET..." << std::endl;
    std::cout << "\t\tLiterals and short literals: " << TEST_SUCCESS << " (included in test of the SET operation)" << std::endl;
    std::cout << "\t\tTesting registers: " << std::flush;
    int registerIndexes[] = {DCPU16_REG_A, DCPU16_REG_B, DCPU16_REG_C, DCPU16_REG_X, DCPU16_REG_Y, DCPU16_REG_Z, DCPU16_REG_I, DCPU16_REG_J};
    char registerNames[] = {'A', 'B', 'C', 'X', 'Y', 'Z', 'I', 'J'};
    for (int x = 0; x < 100; x++) {
        unsigned short args[9];
        for (int i = 0; i < 8; i++)
            args[i] = (rand() % (1 << 16)) & 0xFFFF;
        args[8] = args[1];
        
        runTest(stateList, dcpu16_SET_reg_bin_size, dcpu16_SET_reg_bin, 9, args, 17);
        
        for (int i = 0; i < 8; i++) {
            if (stateList[i]->registers[registerIndexes[i]] != args[i]) {
                std::cout << TEST_FAIL << std::endl;
                std::cout << "\t\t\tCould not set register " << registerNames[i] << " to " << args[i] << ". Register was " << stateList[i]->registers[registerIndexes[i]] << " instead" << std::endl;
                return false;
            }
        }
        if (stateList[8]->registers[DCPU16_REG_B] != args[0]) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << "\t\t\tCould not read register A. Register was read as " << stateList[8]->registers[DCPU16_REG_B] << " instead of " << args[0] << std::endl;
            return false;
        }
        for (int i = 1; i < 8; i++) {
            if (stateList[9+i]->registers[DCPU16_REG_A] != args[i]) {
                std::cout << TEST_FAIL << std::endl;
                std::cout << "\t\t\tCould not read register " << registerNames[i] << ". Register was read as " << stateList[9+i]->registers[DCPU16_REG_A] << " instead of " << args[i] << std::endl;
                return false;
            }
        }
        clearStateList(stateList);
    }
    std::cout << TEST_SUCCESS << std::endl;
    
    std::cout << "\t\tTesting special register PC: " << std::flush;
    for (int x = 0; x < 100; x++) {
        unsigned short r = (4 + rand() % ((1 << 16)-16)) & 0xFFFF;
        
        char* tmp = new char[dcpu16_SET_PC_bin_size];
        memcpy(tmp, dcpu16_SET_PC_bin, dcpu16_SET_PC_bin_size);
        memcpy(tmp+r*2, dcpu16_SET_PC2_bin, dcpu16_SET_PC2_bin_size);
        
        unsigned short args[] = {r};
        runTest(stateList, dcpu16_SET_PC_bin_size, tmp, 1, args, 5);
        
        delete tmp;
        
        std::ostringstream output; output << std::hex;
        bool failed = false;
        if (stateList[0]->registers[DCPU16_REG_PC] != r) {
            failed = true;
            output << "\t\t\tCould not set register PC to " << r << ". Register was " << stateList[0]->registers[DCPU16_REG_PC] << " instead" << std::endl;
        }
        if (stateList[1]->registers[DCPU16_REG_A] != 0x10c) {
            failed = true;
            output << "\t\t\tEmulator failed to follow the PC register after a \"SET PC, " << r << "\"" << std::endl;
        }
        if (stateList[4]->registers[DCPU16_REG_A] != 1) {
            failed = true;
            output << "\t\t\tEmulator failed to follow the PC register after a \"SET PC, 0xFFFE\"" << std::endl;
        }
        if (stateList[5]->registers[DCPU16_REG_PC] != 0) {
            failed = true;
            output << "\t\t\tEmulator failed to wrap the PC register back to 0 after executing an instruction at 0xFFFF" << std::endl;
        }
        if (failed) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << output.str();
            return false;
        }
        
        if (stateList[2]->registers[DCPU16_REG_B] != r+3) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << "\t\t\tCould not read register PC. Register was read as " << stateList[2]->registers[DCPU16_REG_B] << " instead of " << r+3 << std::endl;
            return false;
        
        }
        clearStateList(stateList);
    }
    std::cout << TEST_SUCCESS << std::endl;
    return true;
}
bool dcpu16_runTest() {
    std::vector<dcpu16_state*> stateList;
    
    std::cout << std::hex;
    bool result = dcpu16_runTest_inner(stateList);
    std::cout << std::dec;
    
    clearStateList(stateList);
    return result;
}

