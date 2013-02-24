NAME    := emurom
TARGET  := ${NAME}.bin
SRCS    := ${NAME}.c startup_gcc.c
OBJS    := ${SRCS:.c=.o} 
DEPS    := ${SRCS:.c=.d} 
XDEPS   := $(wildcard ${DEPS}) 
INC     := -I/Users/andrew/src/arm/stellarisware

CC = arm-none-eabi-gcc
LD = arm-none-eabi-ld
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size
LM4FLASH = lm4flash

#Using the Thumb Instruction Set
CFLAGS += -mthumb

#The CPU Variant
CFLAGS += -mcpu=cortex-m4

#Which floating point ABI to use
CFLAGS += -mfloat-abi=softfp

#The type of FPU we are using
CFLAGS += -mfpu=fpv4-sp-d16

#Compile with Size Optimizations
#CFLAGS += -Os
#CFLAGS += -g
CFLAGS += -O2

#Create a separate function section
CFLAGS += -ffunction-sections

#Create a separate data section
CFLAGS += -fdata-sections

#Create dependency files (*.d)
CFLAGS += -MD

#Comply with C99
CFLAGS += -std=c99

#Enable All Warnings 
CFLAGS += -Wall

#More ANSI Checks
CFLAGS += -pedantic

#Flag used in driverlib for compiler specific flags
CFLAGS += -Dgcc

#Flag used in driverlib for specifying the silicon version.
CFLAGS += -DPART_LM4F120H5QR

#Used in driverlib to determine what is loaded in rom.
CFLAGS += -DTARGET_IS_BLIZZARD_RA1

CFLAGS += -S

#Path to Linker Script
LDFLAGS += -T ${NAME}.ld

#Name of the application entry point
LDFLAGS += --entry ResetISR

#Tell the linker to ignore functions that aren't used.
LDFLAGS += --gc-sections

LIBS  = ../stellarisware/driverlib/gcc-cm4f/libdriver-cm4f.a

.PHONY: all clean distclean upload
all:: ${TARGET} 
#	ifneq (${XDEPS},) 
#		include ${XDEPS} 
#	endif 

${TARGET}: ${OBJS} 
	${LD} ${LDFLAGS} -o a.out $^ ${LIBS} 
	${OBJCOPY} -O binary a.out $@
	${SIZE} a.out

${OBJS}: %.o: %.c %.d 
	${CC} ${CFLAGS} ${INC} -o $@ -c $< 

${DEPS}: %.d: %.c Makefile 
	${CC} ${CFLAGS} ${INC} -MM $< > $@ 

clean:: 
	-rm -f *~ *.o *.d *.out ${TARGET} 

distclean:: clean

upload:: ${TARGET}
	${LM4FLASH} -v ${TARGET}
