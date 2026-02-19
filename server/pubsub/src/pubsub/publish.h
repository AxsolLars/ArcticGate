#ifndef PUBLISH_H
#define PUBLISH_H

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include <stdio.h>
#include <stdlib.h>
#include "configs.h"
int runPublish(UA_String *transportProfile,
               UA_NetworkAddressUrlDataType *networkAddressUrl,
               SystemConfig * sysConf,
                int publisherIndex,
                bool debug);
#endif