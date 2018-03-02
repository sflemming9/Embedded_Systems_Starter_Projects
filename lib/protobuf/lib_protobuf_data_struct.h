// Stanford Solar Car Project (2017)
// Author: Gawan Fiore (gfiore@cs.stanford.edu)
#pragma once
#include "data.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

extern DataMessage lib_data_struct_external; //all values generated from vc will be referenced here (bb usage) (vc usage used to send telemetry)
extern DataMessage lib_data_struct_internal; //all values genereated from bb will be referenced here (bb usage)

#ifdef __cplusplus
}
#endif
