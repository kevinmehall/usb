// Unit test for the pipe.h for AVR or host
// gcc pipe_test.c -O3 -g -o pipe_test -Wall -std=c99

#include "pipe.h"
#include <assert.h>

PIPE(pipe, 16, 1, 8, 8)
PIPE(pipe2, 100, 0, 1, 1)

int main(void){
	assert(pipe_can_read(&pipe) == -1);
	assert(pipe_can_write(&pipe) == 15);
	
	uint8_t* w1 = pipe_write_ptr(&pipe);
	pipe_done_write(&pipe);
	
	assert(pipe_can_read(&pipe) == 7);
	assert(pipe_can_write(&pipe) == 7);
	
	uint8_t* w2 = pipe_write_ptr(&pipe);
	assert(w2 == w1+8);
	pipe_done_write(&pipe);
	
	assert(pipe_can_read(&pipe) == 15);
	assert(pipe_can_write(&pipe) == -1);
	
	uint8_t* r1 = pipe_read_ptr(&pipe);
	assert(r1 == w1);
	pipe_done_read(&pipe);
	
	assert(pipe_can_read(&pipe) == 7);
	assert(pipe_can_write(&pipe) == 7);
	
	uint8_t* w3 = pipe_write_ptr(&pipe);
	assert(w3 == w1);
	pipe_done_write(&pipe);
	
	assert(pipe_can_read(&pipe) == 15);
	assert(pipe_can_write(&pipe) == -1);
	
	uint8_t* r2 = pipe_read_ptr(&pipe);
	assert(r2 == w2);
	pipe_done_read(&pipe);
	
	assert(pipe_can_read(&pipe) == 7);
	assert(pipe_can_write(&pipe) == 7);
	
	uint8_t* r3 = pipe_read_ptr(&pipe);
	assert(r3 == w3);
	pipe_done_read(&pipe);
	
	assert(pipe_can_read(&pipe) == -1);
	assert(pipe_can_write(&pipe) == 15);
	
	for (int i=0; i<80; i++){
		*pipe_write_ptr(&pipe2) = i;
		pipe_done_write(&pipe2);
	}
	
	assert(pipe_can_read(&pipe2) == 80);
	assert(pipe_can_write(&pipe2) == 20);
	
	for (int i=0; i<50; i++){
		assert(*pipe_read_ptr(&pipe2) == (i&0xff));
		pipe_done_read(&pipe2);
	}
	
	assert(pipe_can_read(&pipe2) == 30);
	assert(pipe_can_write(&pipe2) == 70);
	
	for (int i=80; pipe_can_write(&pipe2); i++){
		*pipe_write_ptr(&pipe2) = i;
		pipe_done_write(&pipe2);
	}
	
	assert(pipe_can_read(&pipe2) == 100);
	assert(pipe_can_write(&pipe2) == 0);
	
	for (int i=50; pipe_can_read(&pipe2); i++){
		assert(*pipe_read_ptr(&pipe2) == (i&0xff));
		pipe_done_read(&pipe2);
	}
	
	assert(pipe_can_read(&pipe2) == 0);
	assert(pipe_can_write(&pipe2) == 100);
	
	return 0;	
}
