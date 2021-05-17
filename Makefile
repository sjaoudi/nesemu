nesemu : ppu.o video.o cpu.o system.o cartridge.o controller.o memory.o main.o
	cc -g -o nesemu system.o cartridge.o ppu.o cpu.o video.o controller.o memory.o main.o -I/usr/local/include -L/usr/local/lib -lSDL2

memory.o : memory.c memory.h
	cc -g -c memory.c 

video.o : video.c video.h ppu.h 
	cc -g -c video.c $(sdl2-config --cflags)

ppu.o : ppu.c ppu.h cartridge.h cpu.h system.h video.h memory.h
	cc -g -c ppu.c 

cpu.o : cpu.c cpu.h cartridge.h controller.h memory.h
	cc -g -c cpu.c 

system.o : system.c system.h
	cc -g -c system.c 

cartridge.o : cartridge.c cartridge.h memory.h
	cc -g -c cartridge.c 

controller.o : controller.c controller.h
	cc -g -c controller.c

main.o : main.c cartridge.h system.h controller.h
	cc -g -c main.c

clean : 
	rm nesemu main.o cartridge.o system.o cpu.o ppu.o video.o controller.o
