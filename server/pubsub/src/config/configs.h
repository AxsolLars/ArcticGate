#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>   
#include <stddef.h>   


/* One field inside a dataset */
typedef struct {
    char *name;       
    char *type;       
} FieldConfig;

/* One dataset = one DataSetWriter */
typedef struct {
    uint16_t writerId;   
    char *name;          
    FieldConfig ** fields; 
    size_t fieldCount;
} DataSetConfig;

/* One writer group = contains multiple datasets */
typedef struct {
    uint16_t groupId;    
    int interval;
    char * name;        
    DataSetConfig ** dataSets;
    size_t dataSetCount;
} WriterGroupConfig;

/* One publisher = PLC */
typedef struct {
    uint16_t publisherId;    
    char *name;              
    WriterGroupConfig ** writerGroups;
    size_t groupCount;
} PublisherConfig;

/* Root = everything loaded from TOML */
typedef struct {
    PublisherConfig ** publishers;
    size_t publisherCount;
} SystemConfig;

#endif