// Stanford Solar Car Project (2017)
// Author: Gawan Fiore (gfiore@cs.stanford.edu)

#include "lib_protobuf_utils.h"

#include "stdint.h"

#include "pb.h"
#include "pb_common.h"
#include "data.pb.h"

// TODO(gawan): add option to pass in list of tags that should not be copied

// Iterates through all of the fields in the src_message, copying to the
// dest_message only those fields in src_message that have a value.
bool lib_protobuf_utils_ProtobufCopier(void *dest_message, void *src_message,
                                       const pb_field_t *message_fields)
{
  // Get iterator for each message.
  pb_field_iter_t dest_iterator;
  bool dest_has_fields = pb_field_iter_begin(&dest_iterator,
                                             message_fields,
					     dest_message);
  pb_field_iter_t src_iterator;
  bool src_has_fields = pb_field_iter_begin(&src_iterator,
                                            message_fields,
					    src_message);
  // Case: either src_message or dest_message is empty.
  if(!dest_has_fields || !src_has_fields) {
    return false;
  }
  // While iterators have not reached end of messages.
  do
  { 
    bool src_has_field = *(bool*)(src_iterator.pSize);
    if(src_has_field) {
      // Copy field from src to dest.
      *(uint32_t*)(dest_iterator.pData) = *(uint32_t*)(src_iterator.pData);
      // Set field's "has" bit to 1.
      *(bool*)(dest_iterator.pSize) = true;
    }
  } while(pb_field_iter_next(&src_iterator) && pb_field_iter_next(&dest_iterator));
  return true;
}


bool lib_protobuf_utils_DataMessageCopier(DataMessage *dest_message,
                                          DataMessage *src_message)
{
  return lib_protobuf_utils_ProtobufCopier((void*) dest_message, 
                                           (void*) src_message,
				          DataMessage_fields);}

