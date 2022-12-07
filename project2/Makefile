output: encrypt-driver.o encrypt-module.o
	gcc encrypt-driver.o encrypt-module.o -o encrypt
encrypt-driver.o: encrypt-driver.c encrypt-module.h
	gcc -c encrypt-driver.c 
encrypt-module.o: encrypt-module.c encrypt-module.h
	gcc -c encrypt-module.c
clean:
	rm *.o *.txt encrypt
