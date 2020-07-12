/*
	Copyright (c) 2020, Alexandre-Xavier Labonté-Lamoureux
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <string.h>

void dumpbytes(unsigned char *data, int count)
{
	int i;

	printf("\n");

	for (i = 0; i < count; i++)
	{
		if (i % 32 == 0)
		{
			printf("\n");
		}

		printf("%02X ", data[i]);
	}

	printf("\n\n");
}

unsigned int fetch_similar(unsigned char command, unsigned int index, unsigned char* program)
{
	int count = 0;
	for (unsigned int i = index; i < strlen(program); i++)
	{
		if (program[i] == command)
			count++;
		else
			return count;
	}

	return count;
}

/*void cpyasm(void* destination, const void* source, size_t num)
{
	memcpy(destination, source, num);
}*/

#define TAPE_SIZE 30000

int main(int argc, char* argv[])
{
	//char tape[TAPE_SIZE] = {0};
	unsigned char* tape = malloc(TAPE_SIZE);
	memset(tape, 0, TAPE_SIZE);

	unsigned char* program = "++++++++++[>+++++++>++++++++++>+++>+<<<<-]>++.>+.+++++++..+++.>++.<<+++++++++++++++.>.+++.------.--------.>+.>.";
	const unsigned int PROGRAM_SIZE = 65536;

	unsigned int STACK_SIZE = 256;

	unsigned int loop_start_index = 0;
	unsigned char* loop_start[STACK_SIZE];

	//unsigned int loop_end_index = 0;
	//unsigned int loop_end[STACK_SIZE];

	void* ptr = mmap(0, PROGRAM_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	printf("mmap area starts at: 0x%x\n", ptr);

	if (ptr == MAP_FAILED)
	{
		puts("mmap failed.");
		return 1;
	}

	if (argc > 1)
	{
		FILE *fp = fopen(argv[1], "r");
		if (fp != NULL)
		{
			fseek(fp, 0, SEEK_END);
			unsigned int filesize = ftell(fp);
			rewind(fp);

			program = malloc(filesize);

			fread(program, 1, filesize, fp);
			fclose(fp);
		}
	}

	unsigned char* addr = ptr;
	unsigned char count;
	unsigned int pointer;

	memcpy(addr, "\x90", 1);
	addr += 1;

	for (unsigned i = 0; i < strlen(program); i++)
	{
		switch (program[i])
		{
		case '>':
			count = fetch_similar('>', i, program);
			if (count > 1)
			{
//				printf("found %d similar instructions.\n", count);
				memcpy(addr, "\x49\x83\xc0", 3);
				memcpy(addr + 3, &count, 1);
				addr += 4;

				i += count - 1;
			}
			else
			{
				memcpy(addr, "\x49\x83\xc0\x01", 4);
				addr += 4;
			}
			break;

		case '<':
			count = fetch_similar('<', i, program);
			if (count > 1)
			{
//				printf("found %d similar instructions.\n", count);
				memcpy(addr, "\x49\x83\xe8", 3);
				memcpy(addr + 3, &count, 1);
				addr += 4;

				i += count - 1;
			}
			else
			{
				memcpy(addr, "\x49\x83\xe8\x01", 4);
				addr += 4;
			}
			break;

		case '+':
			count = fetch_similar('+', i, program);
			if (count > 1)
			{
//				printf("found %d similar instructions.\n", count);
				memcpy(addr, "\x41\x80\x00", 3);
				memcpy(addr + 3, &count, 1);
				addr += 4;

				i += count - 1;
			}
			else
			{
				memcpy(addr, "\x41\x80\x00\x01", 4);
				addr += 4;
			}
			break;

		case '-':
			memcpy(addr, "\x41\x80\x28\x01", 4);
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

		// control flow stuff
		case '[':
			// last 4 bytes are the jump addr
			memcpy(addr, "\x41\x80\x38\x00\x0f\x84\x00\x00\x00\x00", 10);

			printf("loop start at 0x%x.\n", addr);
			loop_start[loop_start_index] = addr;	// addr is the start of the test to see if the tape is 0
			loop_start_index++;			// do +6 to get addr where the jmp addr must be replaced

			addr += 10;
			break;

		case ']':
			// last 4 bytes are the jump addr
			memcpy(addr, "\xe9\x00\x00\x00\x00", 5);	// addr+1 must be later set to matching [ addr

			loop_start_index--;
								pointer = addr - loop_start[loop_start_index] - 5;
			memcpy(loop_start[loop_start_index] + 6, &pointer, 4);	// write address to jump inside [

			pointer = loop_start[loop_start_index] - addr - 5;
			//unsigned int relative = addr - loop_start[loop_start_index];
			printf("writing addr at 0x%x for loop, relative jmp 0x%x (%d bytes).\n", addr + 1, pointer, pointer);
			
			memcpy(addr + 1, &pointer, 4);	// write address to jump inside this ]

			if (loop_start_index < 0)
				puts("FUCKER!!!!");

			addr += 5;
			break;

		}
	}

	// end this with a "ret"
	memcpy(addr, "\xc3", 1);
	addr += 1;

//	printf("mmap area starts at: 0x%x\n", ptr);
	printf("end of code: %x\n", addr);

	unsigned int shellcodesize = addr - (unsigned char*)ptr;

	printf("size of shellcode: 0x%x (%d bytes)\n", shellcodesize, shellcodesize);

	dumpbytes(ptr, shellcodesize);

	//asm(".byte 0xcc");

	printf("jumping into shellcode.\n");

	asm(/*".intel_syntax;"*/
		"mov %0, %%r8;"
		"call *%1;"
		/*".att_syntax;"*/
		 :
		 : "r" ((uint64_t)tape), "r" ((uint64_t)ptr)
		 : );
}

/*
Pour le pointeur sur le tape, choisir un registre pas utilisé lors des syscalls.

> 
add r8, 1               ; 49 83 c0 01

<
sub r8, 1               ; 49 83 e8 01

+
add byte ptr [r8], 1    ; 41 80 00 01
                        ; 01 c'est l'adresse. on peut pas faire > 0xff

-
sub byte ptr [r8], 1    ; 41 80 28 01
                        ; 01 c'est l'adresse. on peut pas faire > 0xff

.
mov rax, 1  ; syscall write            48 c7 c0 01 00 00 00
mov rdi, 1  ; stdout                   48 c7 c7 01 00 00 00
mov rsi, r8 ; address of char          4c 89 c6
mov rdx, 1  ; number of bytes          48 c7 c2 01 00 00 00
syscall                                0f 05

,
mov rax, 0  ; syscall read             48 c7 c0 00 00 00 00
mov rdi, 0  ; stdin                    48 c7 c7 00 00 00 00
mov rsi, r8 ; address to write to      4c 89 c6
mov rdx, 1  ; number of bytes          48 c7 c2 01 00 00 00
syscall                                0f 05


[
cmp byte ptr [r8], 0                   41 80 38 00
je 11223344                            0f 84 00 00 00 00
; faut sauter après l'adresse du ] fermant  (qui fait jmp)


]
jmp 11223344                           e9 00 00 00 00
; sauter au début des instructions du [ (qui fait cmp...)





faire un `ret` quand on a fini


*/

