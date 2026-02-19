#ifndef SERVERUTILS_H
#define SERVERUTILS_H

#include <open62541/server.h>
#include <open62541/server_pubsub.h>
#include "collection.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --- PubSub Connection Setup --- */

UA_StatusCode
Util_addSubConnection(UA_Server *server, UA_String *transportProfile,
                      UA_NetworkAddressUrlDataType *networkAddressUrl, ServerContext * ctx);

UA_StatusCode
Util_addUdpPubConnection(UA_Server *server, UA_String *transportProfile,
                      UA_NetworkAddressUrlDataType *networkAddressUrl, ServerContext * ctx);

/* --- Publisher & Address Space Hierarchy --- */

UA_StatusCode
Util_addSubscribedVariables(UA_Server *server, ServerContext * ctx);

UA_StatusCode 
Util_addPublisherObject(UA_Server *server, ServerContext * ctx, char * folderName);

/* --- Group Management (Reader & Writer) --- */

UA_StatusCode
Util_addReaderGroup(UA_Server *server, ServerContext * ctx);

UA_StatusCode
Util_addWriterGroup(UA_Server *server, ServerContext * ctx, int interval, char * groupName);

void
initializeWriterGroupConfig(UA_WriterGroupConfig* writerGroupConfig, ServerContext * ctx, 
                            int interval, char * groupName);

void
initializeWriterGroupMessage(UA_WriterGroupConfig * writerGroupConfig, 
                             UA_UadpWriterGroupMessageDataType *writerGroupMessage);

/* --- DataSet & Variable Management --- */

UA_StatusCode
Util_addPublishedDataSet(UA_Server *server, ServerContext * ctx, char * datasetName);

UA_StatusCode
Util_addVariableNode(UA_Server * server, ServerContext * ctx, char * fieldname, char * type);

void
Util_initializeDataSetFieldConfig(UA_DataSetFieldConfig * dsfConf, char * fieldname, 
                                  char * type, ServerContext *ctx);

UA_StatusCode
Util_addDataSetField(UA_Server * server, ServerContext * ctx, UA_DataSetFieldConfig * dsfConf);

UA_StatusCode
Util_addDataSetWriter(UA_Server * server, ServerContext * ctx, char* name);

/* --- Reader Configuration & MetaData --- */

void 
Util_initializeDataSetMetaData(UA_DataSetReaderConfig * readerConfig, size_t fieldCount);

UA_StatusCode
Util_addDataSetMetaData(UA_DataSetReaderConfig * readerConfig, char * fieldname, 
                        char * type, int currentIndex);

void
Util_initializeReaderConfig(UA_DataSetReaderConfig * readerConfig, ServerContext * ctx, char * name);

UA_StatusCode
Util_addDataSetReader(UA_Server *server, ServerContext * ctx, UA_DataSetReaderConfig * readerConfig);

/* --- Target Variable Mapping --- */

void
Util_addTargetVariable(UA_FieldTargetVariable  * targetVars, ServerContext * ctx, size_t index);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_H */