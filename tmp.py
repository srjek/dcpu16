import assembler3
#import emulator
import emulator3

if __name__ == '__main__':
    #assembler3.main(["C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.txt", "C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.bin"])
    #assembler3.main(["C:\\Users\\Josh\\Desktop\\dcpu16\\DATtest.txt", "C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.bin"])
    #assembler3.main(["C:\\Users\\Josh\\Desktop\\dcpu16\\PetriOS.txt", "C:\\Users\\Josh\\Desktop\\dcpu16\\PetriOS.bin"])
    assembler3.main(["C:\\Users\\Josh\\Desktop\\dcpu16\\test.dasm", "C:\\Users\\Josh\\Desktop\\dcpu16\\PetriOS.bin"])
    #emulator.main(["C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.bin"])
    emulator3.main()
    #import cProfile
    #cProfile.run("emulator3.main()")
