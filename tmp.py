import assembler
import emulator

if __name__ == '__main__':
    #assembler.main(["C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.txt", "C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.bin"])
    #assembler.main(["C:\\Users\\Josh\\Desktop\\dcpu16\\DATtest.txt", "C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.bin"])
    #assembler.main(["C:\\Users\\Josh\\Desktop\\dcpu16\\PetriOS.txt", "C:\\Users\\Josh\\Desktop\\dcpu16\\PetriOS.bin"])
    assembler.main(["C:\\Users\\Josh\\Desktop\\dcpu16\\test.dasm", "C:\\Users\\Josh\\Desktop\\dcpu16\\PetriOS.bin"])
    #emulator.main(["C:\\Users\\Josh\\Desktop\\dcpu16\\notchTest.bin"])
    emulator.main()
    #import cProfile
    #cProfile.run("emulator.main()")
