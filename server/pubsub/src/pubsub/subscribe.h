#ifndef SUBSCRIBE_H
#define SUBSCRIBE_H

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include <stdio.h>
#include <stdlib.h>
#include "configs.h"
int runSubscribe(UA_String *transportProfile, UA_NetworkAddressUrlDataType *networkAddressUrl, SystemConfig* sysConf, bool debug);
#endif