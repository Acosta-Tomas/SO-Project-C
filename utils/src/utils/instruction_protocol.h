#ifndef UTILS_INSTRUCTION_H_
#define UTILS_IINSTRUCTION_H_

typedef enum {
	SET,
    SUM,
    SUB,
    JNZ,
    IO_GEN_SLEEP,
	EXIT,
	UNKNOWN,
} set_instruction; // cpu - IO

#endif