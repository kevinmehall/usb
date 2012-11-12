// Ring buffer that takes heavy advantage of constant folding
#pragma once

#include "Common.h"

// Mutable part, becomes an actual struct
typedef struct Pipe_data{
	uint8_t* read_ptr;
	uint8_t* write_ptr;
	int16_t count;
} Pipe_data;

// Immutable part, constant-folded at compile time
typedef struct Pipe{
	Pipe_data* data;
	uint8_t* buffer;
	uint16_t size; // Number of bytes in the buffer
	uint16_t reserve_read; // Bytes to prevent reading so they are available to writer
	uint16_t reserve_write; // Bytes to prevent writing so they are available to reader
	uint16_t read_size;
	uint16_t write_size;
} Pipe;

#define PIPE_H(NAME, SIZE, READ_SIZE, WRITE_SIZE, RESERVE_READ, RESERVE_WRITE)       \
	extern Pipe_data NAME##_data;         \
	extern uint8_t NAME##_buffer[(SIZE)]; \
	const static Pipe NAME = {            \
		.data = &(NAME##_data),           \
		.buffer = &((NAME##_buffer)[0]),  \
		.size = (SIZE),                   \
		.read_size = (READ_SIZE),         \
		.write_size = (WRITE_SIZE),       \
		.reserve_read = (RESERVE_READ),   \
		.reserve_write = (RESERVE_WRITE), \
	};


#define PIPE_C(NAME, SIZE)                   \
	uint8_t NAME##_buffer[(SIZE)];           \
	Pipe_data NAME##_data = {                \
		.count = 0,                          \
		.read_ptr = &((NAME##_buffer)[0]),                       \
		.write_ptr = &((NAME##_buffer)[0]),                      \
	};

#define PIPE(NAME, SIZE, READ_SIZE, WRITE_SIZE, RESERVE_READ, RESERVE_WRITE) \
	PIPE_H(NAME, SIZE, READ_SIZE, WRITE_SIZE, RESERVE_READ, RESERVE_WRITE)   \
	PIPE_C(NAME, SIZE)

static inline int16_t pipe_can_read(const Pipe* pipe) ATTR_ALWAYS_INLINE;
static inline int16_t pipe_can_read(const Pipe* pipe){
	return pipe->data->count - pipe->reserve_read;
}

static inline int16_t pipe_can_write(const Pipe* pipe) ATTR_ALWAYS_INLINE;
static inline int16_t pipe_can_write(const Pipe* pipe){
	return (int16_t)pipe->size - pipe->data->count - pipe->reserve_write;
}

static inline uint8_t* pipe_read_ptr(const Pipe* pipe) ATTR_ALWAYS_INLINE;
static inline uint8_t* pipe_read_ptr(const Pipe* pipe){
	return pipe->data->read_ptr;
}

static inline uint8_t* pipe_write_ptr(const Pipe* pipe) ATTR_ALWAYS_INLINE;
static inline uint8_t* pipe_write_ptr(const Pipe* pipe){
	return pipe->data->write_ptr;
}

static inline void pipe_done_read(const Pipe* pipe) ATTR_ALWAYS_INLINE;
static inline void pipe_done_read(const Pipe* pipe){
	pipe->data->count -= pipe->read_size;
	pipe->data->read_ptr += pipe->read_size;
	// Assumes pipe->size % size == 0 and all accesses aligned
	if (pipe->data->read_ptr == &pipe->buffer[pipe->size])
		pipe->data->read_ptr = &pipe->buffer[0];
}

static inline void pipe_done_write(const Pipe* pipe) ATTR_ALWAYS_INLINE;
static inline void pipe_done_write(const Pipe* pipe){
	pipe->data->count += pipe->write_size;
	pipe->data->write_ptr += pipe->write_size;
	// Assumes pipe->size % size == 0 and all accesses aligned
	if (pipe->data->write_ptr == &pipe->buffer[pipe->size])
		pipe->data->write_ptr = &pipe->buffer[0];
}

static inline uint8_t pipe_read_byte(const Pipe* pipe) ATTR_ALWAYS_INLINE;
static inline uint8_t pipe_read_byte(const Pipe* pipe){
	GCC_ASSERT(pipe->read_size == 1);
	uint8_t r = *pipe->data->read_ptr;
	pipe_done_read(pipe);
	return r;
}

static inline void pipe_write_byte(const Pipe* pipe, uint8_t v) ATTR_ALWAYS_INLINE;
static inline void pipe_write_byte(const Pipe* pipe, uint8_t v){
	GCC_ASSERT(pipe->write_size == 1);
	*pipe->data->write_ptr = v;
	pipe_done_write(pipe);
}

static inline void pipe_reset(const Pipe* pipe) ATTR_ALWAYS_INLINE;
static inline void pipe_reset(const Pipe* pipe){
	pipe->data->count = 0;
	pipe->data->read_ptr = pipe->data->write_ptr = pipe->buffer;
}


