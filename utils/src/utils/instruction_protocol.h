#ifndef UTILS_INSTRUCTION_H_
#define UTILS_IINSTRUCTION_H_

typedef enum {
	SET,
    SUM,
    SUB,
    JNZ,
    MOV_IN, 
    MOV_OUT, 
    RESIZE, 
    COPY_STRING, 
    WAIT,
    SIGNAL,
    IO_STDIN_READ, 
    IO_STDOUT_WRITE,
    IO_GEN_SLEEP,
	EXIT,
	UNKNOWN,
} set_instruction; // cpu - IO

#endif