#ifndef SERVER_H
#define SERVER_H

#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include "configs.h"
#include "collection.h"
#include "nodeIdMap.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Top-level function to initialize and run the OPC UA Server
 * based on the provided PubSub configurations.
 */
int runServer(UA_String* subscribeTransportProfile, 
              UA_NetworkAddressUrlDataType* subscribeNetworkAddressUrl, 
              UA_String* publishTransportProfile, 
              UA_NetworkAddressUrlDataType* publishNetworkAddressUrl, 
              SystemConfig* sysConf, 
              bool debug, 
              ServerConfig serverConfig);

/**
 * Recursive builder functions that map the SystemConfig hierarchy
 * to the open62541 PubSub architecture.
 */

UA_StatusCode 
Server_addSysConfig(UA_Server *server, SystemConfig *sysConf, ServerContext *ctx);

UA_StatusCode 
Server_addPublisher(UA_Server *server, PublisherConfig *pubConf, 
                    ServerContext *ctx, NodeIdMapEntry ** writerGroupIdMap);

UA_StatusCode 
Server_addGroup(UA_Server *server, WriterGroupConfig *groupConf, 
                ServerContext *ctx, NodeIdMapEntry ** writerGroupIdMap);

UA_StatusCode 
Server_addDataset(UA_Server *server, DataSetConfig *dataSetConf, ServerContext *ctx);

#ifdef __cplusplus
}
#endif

#endif 