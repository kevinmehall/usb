// Ring buffer that takes heavy advantage of constant folding
#pragma once

#include "Common.h"

// Mutable part, becomes an actual struct
typedef struct Pipe_data{
	uint8_t* read_ptr;
	uint8_t* write_ptr;
	int8_t count; // available slots
} Pipe_data;

// Immutable part, constant-folded at compile time
typedef struct Pipe{
	Pipe_data* data;
	uint8_t* buffer;
	uint8_t  slots;         // Number of slots in the buffer
	uint8_t reserve_read;  // Slots to prevent reading so they are available to writer
	uint8_t reserve_write; // Slots to prevent writing so they are available to reader
	uint16_t size;          // Size of a buffer slot in bytes
} Pipe;

#define PIPE_H(NAME, SLOTS, SIZE, RESERVE_READ, RESERVE_WRITE)       \
	extern Pipe_data NAME##_data;         \
	extern uint8_t NAME##_buffer[(SLOTS)*(SIZE)]; \
	const static Pipe NAME = {            \
		.data = &(NAME##_data),           \
		.buffer = &((NAME##_buffer)[0]),  \
		.slots = (SLOTS),                 \
		.size = (SIZE),                   \
		.reserve_read = (RESERVE_READ),   \
		.reserve_write = (RESERVE_WRITE), \
	};


#define PIPE_C(NAME, SLOTS, SIZE)                   \
	uint8_t NAME##_buffer[(SLOTS)*(SIZE)];           \
	Pipe_data NAME##_data = {                \
		.count = 0,                          \
		.read_ptr = &((NAME##_buffer)[0]),                       \
		.write_ptr = &((NAME##_buffer)[0]),                      \
	};

#define PIPE(NAME, SLOTS, SIZE, RESERVE_READ, RESERVE_WRITE) \
	PIPE_H(NAME, SLOTS, SIZE, RESERVE_READ, RESERVE_WRITE)   \
	PIPE_C(NAME, SLOTS, SIZE)

// Number of slots available to read
static inline int8_t pipe_can_read(const Pipe* pipe) ATTR_ALWAYS_INLINE;
static inline int8_t pipe_can_read(const Pipe* pipe){
	return pipe->data->count - pipe->reserve_read;
}

// Number of slots available to write
static inline int8_t pipe_can_write(const Pipe* pipe) ATTR_ALWAYS_INLINE;
static inline int8_t pipe_can_write(const Pipe* pipe){
	return (int8_t)pipe->slots - pipe->data->count - pipe->reserve_write;
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
	pipe->data->count -= 1;
	pipe->data->read_ptr += pipe->size;
	// Assumes buffer_size % slot_size == 0 and all accesses aligned
	if (pipe->data->read_ptr == &pipe->buffer[pipe->slots*pipe->size])
		pipe->data->read_ptr = &pipe->buffer[0];
}

static inline void pipe_done_write(const Pipe* pipe) ATTR_ALWAYS_INLINE;
static inline void pipe_done_write(const Pipe* pipe){
	pipe->data->count += 1;
	pipe->data->write_ptr += pipe->size;
	// Assumes buffer_size % slot_size == 0 and all accesses aligned
	if (pipe->data->write_ptr == &pipe->buffer[pipe->slots*pipe->size])
		pipe->data->write_ptr = &pipe->buffer[0];
}

static inline void pipe_reset(const Pipe* pipe) ATTR_ALWAYS_INLINE;
static inline void pipe_reset(const Pipe* pipe){
	pipe->data->count = 0;
	pipe->data->read_ptr = pipe->data->write_ptr = pipe->buffer;
}
