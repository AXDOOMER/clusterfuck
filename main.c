// Copyright (c) 2020, Alexandre-Xavier Labonté-Lamoureux
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>

#define TAPE_SIZE 30000
#define MAX_CODE_SIZE 65536*2
#define STACK_SIZE 256

unsigned int fetch_similar(unsigned char command, unsigned int index, char* program, unsigned int program_size)
{
	int count = 0;
	for (unsigned int i = index; i < program_size; i++)
	{
		if (program[i] == command)
			count++;
		else
			return count;
	}

	return count;
}

void make_collapse(unsigned char command, unsigned int* index, char* program, unsigned int program_size, char* addr, char* opcodes)
{
	unsigned int count = fetch_similar(command, *index, program, program_size);

	// Cap this variable because the maximum value that can be encoded is 0xFF.
	if (count > 0xFF)
		count = 0xFF;

	memcpy(addr, opcodes, 3);
	memcpy(addr + 3, &count, 1);

	*index += count - 1;
}

int main(int argc, char* argv[])
{
	unsigned char* tape = malloc(TAPE_SIZE);
	memset(tape, 0, TAPE_SIZE);

	char* program = NULL;
	unsigned int program_size = 0;

	unsigned int loop_start_index = 0;
	char* loop_start[STACK_SIZE];

	char* code = mmap(0, MAX_CODE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);

	if (code == MAP_FAILED)
	{
		puts("mmap failed.");
		return 3;
	}

	if (argc > 1)
	{
		FILE *fp = fopen(argv[1], "r");
		if (fp != NULL)
		{
			fseek(fp, 0, SEEK_END);
			program_size = ftell(fp);
			rewind(fp);

			program = malloc(program_size);

			fread(program, 1, program_size, fp);
			fclose(fp);
		}
		else
		{
			puts("Couldn't open program.");
			return 2;
		}
	}
	else
	{
		puts("Clusterfuck, a x64 JIT interpreter for brainfuck.");
		puts("Copyright (c) 2020, Alexandre-Xavier Labonté-Lamoureux");
		puts("Distributed under the BSD 2-Clause License.");

		printf("\nUsage: %s program.bf\n", argv[0]);
		return 1;
	}

	char* addr = code;
	unsigned int offset;

	for (unsigned i = 0; i < program_size; i++)
	{
		switch (program[i])
		{
		case '>':
			make_collapse('>', &i, program, program_size, addr, "\x49\x83\xc0");
			addr += 4;
			break;

		case '<':
			make_collapse('<', &i, program, program_size, addr, "\x49\x83\xe8");
			addr += 4;
			break;

		case '+':
			make_collapse('+', &i, program, program_size, addr, "\x41\x80\x00");
			addr += 4;
			break;

		case '-':
			make_collapse('-', &i, program, program_size, addr, "\x41\x80\x28");
			addr += 4;
			break;

		case '.':
			memcpy(addr, "\x48\xc7\xc0\x01\x00\x00\x00\x48\xc7\xc0\x01\x00\x00\x00\x4c\x89\xc6\x48\xc7\xc2\x01\x00\x00\x00\x0f\x05", 26);
			addr += 26;
			break;

		case ',':
			memcpy(addr, "\x48\xc7\xc0\x00\x00\x00\x00\x48\xc7\xc7\x00\x00\x00\x00\x4c\x89\xc6\x48\xc7\xc2\x01\x00\x00\x00\x0f\x05", 26);
			addr += 26;
			break;

		// looooooooooops
		case '[':
			memcpy(addr, "\x41\x80\x38\x00\x0f\x84\x00\x00\x00\x00", 10);

			// Must come back later and set the last four bytes to the correct address
			loop_start[loop_start_index] = addr;
			loop_start_index++;

			addr += 10;
			break;

		case ']':
			memcpy(addr, "\xe9\x00\x00\x00\x00", 5);

			// Get the address from the stack. Compute the jump offset for the `[` part.
			loop_start_index--;
			offset = addr - loop_start[loop_start_index] - 5;
			memcpy(loop_start[loop_start_index] + 6, &offset, 4);

			// Use the same address from the stack to compute the jump from `]` to `[`.
			offset = loop_start[loop_start_index] - addr - 5;
			memcpy(addr + 1, &offset, 4);

			addr += 5;
			break;

		}
	}

	// End this with a "ret"
	memcpy(addr, "\xc3", 1);
	addr += 1;

	// BUG: Remove this printf or make it empty and the program won't display anything
	printf("compiled program size is %ld bytes, running...\n", addr - code);

	asm("mov %0, %%r8;"
	    "call *%1;"
	     :
	     : "r" ((uint64_t)tape), "r" ((uint64_t)code)
	     : "r8");
}

