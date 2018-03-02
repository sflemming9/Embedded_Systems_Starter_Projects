#include "lib_pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include <stdbool.h>

size_t SerializeProtobuf(void* message, const pb_field_t* message_fields, char* buffer, size_t length) {
  pb_ostream_t stream = pb_ostream_from_buffer((pb_byte_t*) buffer, length);
  bool status = pb_encode(&stream, message_fields, message);
  if (status) {
    return stream.bytes_written;
  } else {
    return 0;
  }
}

bool DeserializeProtobuf(void* message, const pb_field_t* message_fields, char* buffer, size_t length) {
  pb_istream_t stream = pb_istream_from_buffer((pb_byte_t*) buffer, length);
  bool status = pb_decode(&stream, message_fields, message);
  return status;
}

// Callback function for writing to protobuf.
// Only need this for repeated fields
bool PB_encode_callback(pb_ostream_t *stream, const pb_field_t *field, void * const * arg)
{
  PBInnerField* inner_field = (PBInnerField *) *arg;
  for (uint8_t i = 0; i < inner_field->length; i++) {
    if (!pb_encode_tag_for_field(stream, field))
      return false;
    
    if (!pb_encode_submessage(stream, inner_field->field, inner_field->messages[i]))
      return false;
  }
  return true;
}

// Callback function for reading from protobuf.
// Only need this for repeated fields
bool PB_decode_callback(pb_istream_t *stream, const pb_field_t *field, void ** arg)
{
  PBInnerField* inner_field = (PBInnerField *) *arg;
  if (!pb_decode(stream, inner_field->field, inner_field->messages[inner_field->length])) {
    return false;
  }
  inner_field->length += 1;
  return true; 
}
