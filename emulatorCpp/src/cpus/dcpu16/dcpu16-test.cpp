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
    unsigned short* ram;
    unsigned short* registers;
    
    unsigned short A;
    unsigned short B;
    unsigned short C;
    unsigned short X;
    unsigned short Y;
    unsigned short Z;
    unsigned short I;
    unsigned short J;
    unsigned short PC;
    unsigned short SP;
    unsigned short EX;
    
    dcpu16_state(dcpu16* cpu) {
        ram = new unsigned short[0x10000];
        registers = new unsigned short[13];
        for (int i = 0; i < 0x10000; i++)
            ram[i] = cpu->ram[i];
        for (int i = 0; i < DCPU16_NUM_REGS; i++)
            registers[i] = cpu->registers[i];
        A = registers[DCPU16_REG_A];
        B = registers[DCPU16_REG_B];
        C = registers[DCPU16_REG_C];
        X = registers[DCPU16_REG_X];
        Y = registers[DCPU16_REG_Y];
        Z = registers[DCPU16_REG_Z];
        I = registers[DCPU16_REG_I];
        J = registers[DCPU16_REG_J];
        PC = registers[DCPU16_REG_PC];
        SP = registers[DCPU16_REG_SP];
        EX = registers[DCPU16_REG_EX];
    }
    ~dcpu16_state() {
        delete[] ram;
        delete[] registers;
    }
    
    unsigned short peek() {
        return ram[registers[DCPU16_REG_SP]];
    }
    unsigned short pick(int offset) {
        return ram[(registers[DCPU16_REG_SP]+offset)&0xFFFF];
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
    for (int i = 0; i < stateList.size(); i++) {
        delete stateList[i];
        stateList[i] = NULL;
    }
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
        
        if (stateList[0]->A != r) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << "\t\tCould not set register A to literal " << r << ". Register was " << stateList[0]->A << " instead" << std::endl;
            return false;
        }
        if (stateList[1]->A != r2) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << "\t\tCould not set register A to short literal " << r2 << ". Register was " << stateList[0]->A << " instead" << std::endl;
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
        if (stateList[8]->B != args[0]) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << "\t\t\tCould not read register A. Register was read as " << stateList[8]->B << " instead of " << args[0] << std::endl;
            return false;
        }
        for (int i = 1; i < 8; i++) {
            if (stateList[9+i]->A != args[i]) {
                std::cout << TEST_FAIL << std::endl;
                std::cout << "\t\t\tCould not read register " << registerNames[i] << ". Register was read as " << stateList[9+i]->A << " instead of " << args[i] << std::endl;
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
        runTest(stateList, dcpu16_SET_PC_bin_size, tmp, 1, args, 6);
        
        delete tmp;
        std::ostringstream output; output << std::hex;
        bool failed = false;
        
        if (stateList[0]->PC != r) {
            failed = true;
            output << "\t\t\tCould not set register PC to " << r << ". Register was " << stateList[0]->PC << " instead" << std::endl;
        }
        if (stateList[1]->A != 0x10c) {
            failed = true;
            output << "\t\t\tEmulator failed to follow the PC register after a \"SET PC, " << r << "\"" << std::endl;
        }
        if (stateList[4]->A != 1) {
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
        
        if (stateList[2]->B != r+3) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << "\t\t\tCould not read register PC. Register was read as " << stateList[2]->B << " instead of " << r+3 << std::endl;
            return false;
        }
        clearStateList(stateList);
    }
    std::cout << TEST_SUCCESS << std::endl;
    
    std::cout << "\t\tTesting special register SP, PUSH, PEEK, POP, PICK: " << std::flush;
    for (int x = 0; x < 100; x++) {
        unsigned short r = (20 + rand() % ((1 << 16)-30)) & 0xFFFF;
        unsigned short r2 = (20 + rand() % ((1 << 16)-30)) & 0xFFFF;
        r2 = (r2 - r) & 0xFFFF;
        
        unsigned short args[] = {r, 0x10000-r, r2};
        runTest(stateList, dcpu16_SET_stack_bin_size, dcpu16_SET_stack_bin, 3, args, 9);
        
        std::ostringstream output; output << std::hex;
        bool failed = false;
        
        //Write to SP, read SP
        if (stateList[0]->SP != r) {
            failed = true;
            output << "\t\t\tCould not set register SP to " << r << ". Register was " << stateList[0]->registers[DCPU16_REG_SP] << " instead" << std::endl;
        }
        if (stateList[1]->A != stateList[0]->SP) {
            failed = true;
            output << "\t\t\tCould not read register SP. Register was read as " << stateList[1]->A << " instead of " << stateList[0]->SP << std::endl;
        }
        //Push to stack, read SP
        if (stateList[2]->peek() != 0x10c) {
            failed = true;
            output << "\t\t\tStack operation \"SET PUSH, 0x10c\" failed, last item on stack was " << stateList[2]->peek() << std::endl;
        }
        if (stateList[2]->SP != stateList[0]->SP-1) {
            failed = true;
            output << "\t\t\tStack operation \"SET PUSH, 0x10c\" failed, stack pointer was " << stateList[2]->SP << " instead of " << stateList[0]->SP-1 << std::endl;
        }
        if (stateList[3]->A != stateList[2]->SP) {
            failed = true;
            output << "\t\t\tCould not read register SP. Register was read as " << stateList[3]->A << " instead of " << stateList[2]->SP << std::endl;
        }
        //Peek from stack
        if (stateList[4]->SP != stateList[3]->SP) {
            failed = true;
            output << "\t\t\tStack operation \"SET A, PEEK\" failed, stack pointer changed to " << stateList[4]->SP << " from " << stateList[3]->SP << std::endl;
        }
        if (stateList[4]->A != stateList[3]->peek()) {
            output << "\t\t\tCould not peek. Last item on stack was read as " << stateList[4]->A << " instead of " << stateList[3]->peek() << std::endl;
        }
        //Pop from stack
        if (stateList[5]->SP != stateList[4]->SP+1) {
            failed = true;
            output << "\t\t\tStack operation \"SET A, POP\" failed, stack pointer was " << stateList[5]->SP << " instead of " << stateList[4]->SP+1 << std::endl;
        }
        if (stateList[5]->ram[stateList[4]->SP] != stateList[4]->peek()) {
            failed = true;
            output << "\t\t\tStack operation \"SET A, POP\" failed, pop-ed item on stack was changed in ram from " << stateList[4]->peek() << " to " << stateList[5]->ram[stateList[4]->SP] << std::endl;
        }
        if (stateList[5]->A != stateList[4]->peek()) {
            failed = true;
            output << "\t\t\tCould not pop. Last item on stack was pop-ed as " << stateList[5]->A << " instead of " << stateList[4]->peek() << std::endl;
        }
        //Read stack offset (TWICE!)
        if (stateList[6]->SP != stateList[5]->SP) {
            failed = true;
            output << "\t\t\tStack operation \"SET A, [SP-1]; SET B, [SP-" << r << "]\" failed, stack pointer changed to " << stateList[6]->SP << " from " << stateList[5]->SP << std::endl;
        }
        if (stateList[6]->pick(-1) != stateList[5]->pick(-1)) {
            failed = true;
            output << "\t\t\tStack operation \"SET A, [SP-1]\" failed, Pick-ed item on stack was changed in ram from " << stateList[5]->pick(-1) << " to " << stateList[6]->pick(-1) << std::endl;
        }
        if (stateList[7]->pick(-r) != stateList[6]->pick(-r)) {
            failed = true;
            output << "\t\t\tStack operation \"SET B, [SP-" << r << "]\" failed, Pick-ed item on stack was changed in ram from " << stateList[6]->pick(-r) << " to " << stateList[7]->pick(-r) << std::endl;
        }
        if (stateList[6]->A != stateList[5]->pick(-1)) {
            failed = true;
            output << "\t\t\tCould not pick. Offset -1 was pick-ed as " << stateList[6]->A << " instead of " << stateList[5]->pick(-1) << std::endl;
        }
        if (stateList[7]->B != stateList[6]->pick(-r)) {
            failed = true;
            output << "\t\t\tCould not pick. Offset -" << r << " was pick-ed as " << stateList[7]->B << " instead of " << stateList[6]->pick(-r) << std::endl;
        }
        //Write to stack offset
        if (stateList[8]->ram[(r+r2)&0xFFFF] != 0xBEEF) {
            failed = true;
            output << "\t\t\tCould not write to stack offset. [SP+" << r2 << "] (" << r << "+" << r2 << ") was written as " << stateList[7]->ram[(r+r2)&0xFFFF] << " instead of 0xbeef" << std::endl;
        }
        
        clearStateList(stateList);
        args[0] = r; args[1] = r;
        runTest(stateList, dcpu16_SET_stack2_bin_size, dcpu16_SET_stack2_bin, 2, args, 5);
        
        //Push to stack when SP is 0
        if (stateList[0]->peek() != 0x10c) {
            failed = true;
            output << "\t\t\tStack operation \"SET PUSH, 0x10c\" failed, last item on stack was " << stateList[0]->peek() << std::endl;
        }
        if (stateList[0]->SP != 0xFFFF) {
            failed = true;
            output << "\t\t\tStack operation \"SET PUSH, 0x10c\" failed, stack pointer was " << stateList[0]->SP << " instead of " << 0xFFFF << std::endl;
        }
        //Pick from stack when SP is 0xFFFF
        if (stateList[1]->A != stateList[0]->pick(1)) {
            failed = true;
            output << "\t\t\tCould not pick. Offset +1 was pick-ed as " << stateList[1]->A << " instead of " << stateList[0]->pick(1) << std::endl;
        }
        //Write to stack offset when SP is 0xFFFF
        if (stateList[2]->pick(r) != 0xBEEF) {
            failed = true;
            output << "\t\t\tCould not write to stack offset. Offset +" << r << " was written as " << stateList[2]->pick(r) << " instead of 0xbeef" << std::endl;
        }
        //Pick from stack offset when SP is 0xFFFF
        if (stateList[3]->A != stateList[2]->pick(r)) {
            failed = true;
            output << "\t\t\tCould not pick. Offset +" << r << " was pick-ed as " << stateList[3]->A << " instead of " << stateList[2]->pick(r) << std::endl;
        }
        //Pop from stack when SP is 0xFFFF
        if (stateList[4]->SP != ((stateList[3]->SP+1)&0xFFFF)) {
            failed = true;
            output << "\t\t\tStack operation \"SET A, POP\" failed, stack pointer was " << stateList[4]->SP << " instead of " << ((stateList[3]->SP+1)&0xFFFF) << std::endl;
        }
        if (stateList[4]->ram[stateList[3]->SP] != stateList[3]->peek()) {
            failed = true;
            output << "\t\t\tStack operation \"SET A, POP\" failed, pop-ed item on stack was changed in ram from " << stateList[3]->peek() << " to " << stateList[4]->ram[stateList[3]->SP] << std::endl;
        }
        if (stateList[4]->A != stateList[3]->peek()) {
            failed = true;
            output << "\t\t\tCould not pop. Last item on stack was pop-ed as " << stateList[4]->A << " instead of " << stateList[3]->peek() << std::endl;
        }

        clearStateList(stateList);
        if (failed) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << output.str();
            return false;
        }
    }
    std::cout << TEST_SUCCESS << std::endl;
    
    std::cout << "\t\tTesting special register EX: " << std::flush;
    for (int x = 0; x < 100; x++) {
        unsigned short r = (10 + rand() % ((1 << 16)-20)) & 0xFFFF;
        
        unsigned short args[] = {r};
        runTest(stateList, dcpu16_SET_EX_bin_size, dcpu16_SET_EX_bin, 1, args, 2);
        
        std::ostringstream output; output << std::hex;
        bool failed = false;
        
        if (stateList[0]->EX != r) {
            failed = true;
            output << "\t\t\tCould not set special register EX to " << r << ". Register was " << stateList[0]->EX << " instead" << std::endl;
        }
        if (stateList[1]->A != stateList[0]->EX) {
            failed = true;
            output << "\t\t\tCould not read special register EX. Register was read as " << stateList[1]->A << " instead of " << stateList[0]->EX << std::endl;
        }
        
        clearStateList(stateList);
        if (failed) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << output.str();
            return false;
        }
    }
    std::cout << TEST_SUCCESS << std::endl;
    
    std::cout << "\t\tTesting dereferencing: " << std::flush;
    for (int x = 0; x < 100; x++) {
        unsigned short r = (10 + rand() % ((1 << 16)-20)) & 0xFFFF;
        unsigned short r2 = (rand() % (1 << 16)) & 0xFFFF;
        
        unsigned short args[] = {r, r2, r};
        runTest(stateList, dcpu16_SET_deref_bin_size, dcpu16_SET_deref_bin, 3, args, 2);
        
        std::ostringstream output; output << std::hex;
        bool failed = false;
        
        if (stateList[0]->ram[r] != r2) {
            failed = true;
            output << "\t\t\tCould not set [" << r << "] to " << r2 << ". Ram was " << stateList[0]->ram[r] << " instead" << std::endl;
        }
        if (stateList[1]->ram[r] != stateList[0]->ram[r]) {
            failed = true;
            output << "\t\t\t\"SET A, [" << r << "]\" failed. Ram changed from " << stateList[0]->ram[r] << " to " << stateList[1]->ram[r] << std::endl;
        }
        if (stateList[1]->A != stateList[0]->ram[r]) {
            failed = true;
            output << "\t\t\tCould not read [" << r << "]. Ram was read as " << stateList[1]->A << " instead of " << stateList[0]->ram[r] << std::endl;
        }
        
        clearStateList(stateList);
        if (failed) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << output.str();
            return false;
        }
    }
    std::cout << TEST_SUCCESS << std::endl;
    
    std::cout << "\t\tTesting register dereferencing: " << std::flush;
    for (int x = 0; x < 100; x++) {
        unsigned short args[16];
        for (int i = 0; i < 16; i += 2) {
            args[i] = (50 + rand() % ((1 << 16)-60)) & 0xFFFF;
            args[i+1] = (rand() % (1 << 16)) & 0xFFFF;
        }
        
        runTest(stateList, dcpu16_SET_deref_reg_bin_size, dcpu16_SET_deref_reg_bin, 16, args, 24);
        
        std::ostringstream output; output << std::hex;
        bool failed = false;
        
        for (int i = 0; i < 8; i++) {
            if (stateList[i*3+1]->ram[args[i*2]] != args[i*2+1]) {
                failed = true;
                output << "\t\t\tCould not set [" << registerNames[i] << "] ([" << args[i*2] << "]) to " << args[i*2+1] << ". Ram was " << stateList[i*3+1]->ram[args[i*2]] << " instead" << std::endl;
            }
            if (stateList[i*3+2]->ram[args[i*2]] != stateList[i*3+1]->ram[args[i*2]]) {
                failed = true;
                output << "\t\t\t\"SET ";
                
                if (i == 0) output << "B";
                else        output << "A";
                
                output << ", [" << registerNames[i] << "]\" ([" << args[i*2] << "]) failed. Ram changed from " << stateList[i*3+1]->ram[args[i*2]] << " to " << stateList[i*3+2]->ram[args[i*2]] << std::endl;
            }
            int read_reg = DCPU16_REG_A;
            if (i == 0) read_reg = DCPU16_REG_B;
            if (stateList[i*3+2]->registers[read_reg] != stateList[i*3+1]->ram[args[i*2]]) {
                failed = true;
                output << "\t\t\tCould not read [" << registerNames[i] << "] ([" << args[i*2] << "]). Ram was read as " << stateList[i*3+2]->registers[read_reg] << " instead of " << stateList[i*3+1]->ram[args[i*2]] << std::endl;
            }
        }
        
        clearStateList(stateList);
        if (failed) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << output.str();
            return false;
        }
    }
    std::cout << TEST_SUCCESS << std::endl;
    
    std::cout << "\t\tTesting register+offset dereferencing: " << std::flush;
    for (int x = 0; x < 100; x++) {
        unsigned short args[8*8];
        for (int i = 0; i < 8*8; i += 8) {
            unsigned short r, r2, r3, r4;
            r = (70 + rand() % ((1 << 15)-80)) & 0xFFFF;
            r2 = (70 + rand() % ((1 << 15)-80)) & 0xFFFF;
            while ((r3+r4) < (0xFFFF+20)) {
                r3 = (r3 + rand() % ((1 << 16)-r3)) & 0xFFFF;
                r4 = (r4 + rand() % ((1 << 16)-r4)) & 0xFFFF;
            }
            unsigned short v = (rand() % (1 << 16)) & 0xFFFF;
            args[i]   = r;
            args[i+1] = r2;
            args[i+2] = r2;
            args[i+3] = r3;
            args[i+4] = r4;
            args[i+5] = r4;
            args[i+6] = v;
            args[i+7] = v;
        }
        
        runTest(stateList, dcpu16_SET_deref_offset_bin_size, dcpu16_SET_deref_offset_bin, 8*8, args, 6*8);
        
        std::ostringstream output; output << std::hex;
        bool failed = false;
        
        for (int i = 0; i < 8; i++) {
            unsigned short r, r2, r3, r4, v;
            r  = args[i*8];
            r2 = args[i*8+1];
            r3 = args[i*8+3];
            r4 = args[i*8+4];
            v  = args[i*8+6];
            
            int read_reg = DCPU16_REG_A;
            char* read_reg_name = "A";
            if (i == 0) {
                read_reg = DCPU16_REG_B;
                read_reg_name = "B";
            }
            
            //When offset doesn't wrap around
            if (stateList[i*6+1]->ram[r+r2] != v) {
                failed = true;
                output << "\t\t\tCould not set [" << registerNames[i] << "+" << r2 << "]\" ([" << r << "+" << r2 << "]) to " << v << ". Ram was " << stateList[i*6+1]->ram[r+r2] << " instead" << std::endl;
            }
            if (stateList[i*6+2]->ram[r+r2] != stateList[i*6+1]->ram[r+r2]) {
                failed = true;
                output << "\t\t\t\"SET " << read_reg_name << ", [" << registerNames[i] << "+" << r2 << "]\" ([" << r << "+" << r2 << "]) failed. Ram changed from " << stateList[i*6+1]->ram[r+r2] << " to " << stateList[i*6+2]->ram[r+r2] << std::endl;
            }
            if (stateList[i*6+2]->registers[read_reg] != stateList[i*6+1]->ram[r+r2]) {
                failed = true;
                output << "\t\t\tCould not read [" << registerNames[i] << "+" << r2 << "] ([" << r << "+" << r2 << "]). Ram was read as " << stateList[i*6+2]->registers[read_reg] << " instead of " << stateList[i*6+1]->ram[r+r2] << std::endl;
            }
            //When offset wraps around
            if (stateList[i*6+4]->ram[(r3+r4)&0xFFFF] != v) {
                failed = true;
                output << "\t\t\tCould not set [" << registerNames[i] << "+" << r4 << "]\" ([" << r3 << "+" << r4 << "]) to " << v << ". Ram was " << stateList[i*6+4]->ram[(r3+r4)&0xFFFF] << " instead" << std::endl;
            }
            if (stateList[i*6+5]->ram[(r3+r4)&0xFFFF] != stateList[i*6+4]->ram[(r3+r4)&0xFFFF]) {
                failed = true;
                output << "\t\t\t\"SET " << read_reg_name << ", [" << registerNames[i] << "+" << r4 << "]\" ([" << r3 << "+" << r4 << "]) failed. Ram changed from " << stateList[i*6+4]->ram[(r3+r4)&0xFFFF] << " to " << stateList[i*6+5]->ram[(r3+r4)&0xFFFF] << std::endl;
            }
            if (stateList[i*6+5]->registers[read_reg] != stateList[i*6+4]->ram[(r3+r4)&0xFFFF]) {
                failed = true;
                output << "\t\t\tCould not read [" << registerNames[i] << "+" << r4 << "] ([" << r3 << "+" << r4 << "]). Ram was read as " << stateList[i*6+5]->registers[read_reg] << " instead of " << stateList[i*6+4]->ram[(r3+r4)&0xFFFF] << std::endl;
            }
        }
            
        clearStateList(stateList);
        if (failed) {
            std::cout << TEST_FAIL << std::endl;
            std::cout << output.str();
            return false;
        }
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

