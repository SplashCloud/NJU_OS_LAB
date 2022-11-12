OBJS = main.o my_print.o

main:	${OBJS}
	g++ ${OBJS} -o main
main.o:	main.cpp
	g++ -c main.cpp -o main.o
my_print.o: my_print.asm
	nasm -f elf32 -o my_print.o my_print.asm

clean:
	rm main ${OBJS}
run:
	./main
