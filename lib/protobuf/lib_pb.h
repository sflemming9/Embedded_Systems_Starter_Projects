#pragma once

#include <stdbool.h>
#include <assert.h>

#include "pb_encode.h"

#ifdef STM32F4XX
	#include <stm32f4xx.h>
#elif STM32F30X
	#include <stm32f30x.h>
#elif STM32F2XX
    #include <stm32f2xx.h>
#endif

typedef struct PBInnerFieldT {
  const pb_field_t* field;
  void** messages;
  uint8_t length;
} PBInnerField;

// Encode a message and store it in the given buffer.
// Returns 0 if encoding failed. Otherwise, return number of bytes encoded.
size_t SerializeProtobuf(void* message, const pb_field_t* message_fields, char* buffer, size_t length);

// Decode a message stored in the buffer and place it in message.
bool DeserializeProtobuf(void* message, const pb_field_t* message_fields, char* buffer, size_t length);

// Callback function for writing to protobuf.
// Only need this for repeated fields
bool PB_encode_callback(pb_ostream_t *stream, const pb_field_t *field, void * const * arg);

// Callback function for reading from protobuf.
// Only need this for repeated fields
bool PB_decode_callback(pb_istream_t *stream, const pb_field_t *field, void ** arg);

