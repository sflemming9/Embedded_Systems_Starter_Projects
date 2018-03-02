// Stanford Solar Car Project (2017)
// Author: Gawan Fiore (gfiore@stanford.edu)

#pragma once

#include "pb_common.h"
#include "data.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

// This function will copy the field values from one instance of a DataMessage
// to another. Only fields that have values in the source struct will be copied
// to the destination struct. All other fields in the destination struct will
// be unchanged.
//
// This function is particularly useful when updating an internal data struct
// with new values received over Ethernet from another board.
bool lib_protobuf_utils_DataMessageCopier(DataMessage *dest_message, DataMessage *src_message);

#ifdef __cplusplus
}
#endif


