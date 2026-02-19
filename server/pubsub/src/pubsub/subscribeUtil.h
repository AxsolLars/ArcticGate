#ifndef SUBSCRIBEUTIL_H
#define SUBSCRIBEUTIL_H

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_pubsub.h>

#include <stdio.h>
#include <stdlib.h>

#include "configs.h"
#include "collection.h"

/* Fill metadata fields for a DataSet based on FieldConfig array */
void fillFields(UA_DataSetMetaDataType *pMetaData,
                FieldConfig **fieldConfs,
                size_t fieldCount);

/* Add subscribed variable nodes and bind them as target variables */
UA_StatusCode addValueAttributes(UA_Server *server,
                                UA_DataSetReaderConfig *readerConfig,
                                Sub_IdCollection *ids);

/* Add a DataSetReader to a ReaderGroup */
UA_StatusCode addDataSetReader(UA_Server *server,
                               Sub_IdCollection *ids,
                               char *dataSetName,
                               UA_DataSetReaderConfig *readerConfig);

/* Add a ReaderGroup to an existing PubSubConnection */
UA_StatusCode addReaderGroup(UA_Server *server,
                             Sub_IdCollection *ids);

/* Create an ObjectNode for one publisher under the SubscribedVariables folder */
UA_StatusCode addPublisherObject(UA_Server *server,
                                 Sub_IdCollection *ids,
                                 char *folderName);

/* Create the top-level "Subscribed Variables" folder */
UA_StatusCode addSubscribedVariables(UA_Server *server,
                                     Sub_IdCollection *ids);

/* Add a PubSubConnection for receiving PubSub messages */
UA_StatusCode addPubSubConnection(UA_Server *server,
                                  UA_String *transportProfile,
                                  UA_NetworkAddressUrlDataType *networkAddressUrl,
                                  Sub_IdCollection *ids);

#endif /* SUBSCRIBEUTIL_H */
