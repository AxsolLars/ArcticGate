#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "toml.h"
#include "configs.h"

FieldConfig *  parseField(toml_table_t* field){
    FieldConfig *fieldConfig = malloc(sizeof(FieldConfig));
    toml_datum_t name = toml_string_in(field, "name");

    if (name.ok){
        fieldConfig->name = name.u.s;
    } else {
        fieldConfig->name = NULL;
    }
    toml_datum_t type = toml_string_in(field, "type");

    if (type.ok){
        fieldConfig->type = type.u.s;
    } else {
        fieldConfig->type = NULL;
    }

    return fieldConfig;
}

DataSetConfig *  parseDataSet(toml_table_t* dataSet){
    DataSetConfig *dataSetConf = malloc(sizeof(DataSetConfig));
    toml_datum_t name = toml_string_in(dataSet, "name");

    if (name.ok){
        dataSetConf->name = name.u.s;
    } else {
        dataSetConf->name = NULL;
    }
    toml_datum_t id = toml_int_in(dataSet, "writerId");

    if (id.ok){
        dataSetConf->writerId = id.u.i;
    } else {
        dataSetConf->writerId = -1;
    }

    toml_array_t * fields = toml_array_in(dataSet, "fields");


    int fieldCount = toml_array_nelem(fields);

    dataSetConf->fields = malloc(fieldCount * sizeof(FieldConfig*));
    dataSetConf->fieldCount = fieldCount;

    for (int i = 0; i < fieldCount; i++){
        toml_table_t* field = toml_table_at(fields, i);
        if(!field) continue;
        dataSetConf->fields[i] = parseField(field);
    }
    
    return dataSetConf;
}

WriterGroupConfig *  parseWritingGroup(toml_table_t* writerGroup){
    WriterGroupConfig *wgrConf = malloc(sizeof(WriterGroupConfig));
    toml_datum_t interval = toml_int_in(writerGroup, "interval");

    if (interval.ok){
        wgrConf->interval = interval.u.i;
    } else {
        wgrConf->interval = -1;
    }
    toml_datum_t id = toml_int_in(writerGroup, "id");

    if (id.ok){
        wgrConf->groupId = id.u.i;
    } else {
        wgrConf->groupId = -1;
    }

    toml_datum_t name = toml_string_in(writerGroup, "name");

    if (name.ok){
        wgrConf->name = name.u.s;
    } else {
        wgrConf->name = NULL;
    }

    toml_array_t * dataSets = toml_array_in(writerGroup, "dataSets");


    int setCount = toml_array_nelem(dataSets);

    wgrConf->dataSets = malloc(setCount * sizeof(DataSetConfig*));
    wgrConf->dataSetCount = setCount;

    for (int i = 0; i < setCount; i++){
        toml_table_t* dataSet = toml_table_at(dataSets, i);
        if(!dataSet) continue;
        wgrConf->dataSets[i] = parseDataSet(dataSet);
    }
    
    return wgrConf;
}

PublisherConfig *  parsePublisher(toml_table_t* publisher){
    PublisherConfig *pubConf = malloc(sizeof(PublisherConfig));
    toml_datum_t name = toml_string_in(publisher, "name");

    if (name.ok){
        pubConf->name = name.u.s;
    } else {
        pubConf->name = NULL;
    }
    toml_datum_t id = toml_int_in(publisher, "publisherId");

    if (id.ok){
        pubConf->publisherId = id.u.i;
    } else {
        pubConf->publisherId = -1;
    }

    toml_array_t * writerGroups = toml_array_in(publisher, "writerGroups");


    int groupCount = toml_array_nelem(writerGroups);

    pubConf->writerGroups = malloc(groupCount * sizeof(WriterGroupConfig*));
    pubConf->groupCount = groupCount;

    for (int i = 0; i < groupCount; i++){
        toml_table_t* group = toml_table_at(writerGroups, i);
        if(!group) continue;
        pubConf->writerGroups[i] = parseWritingGroup(group);
    }
    
    return pubConf;
}

SystemConfig* parseFile(char* path, int publisher){
    /*if publisher == -1 all publishers get parsed -> sub side*/
    FILE* fp = fopen(path, "r");
    
    if (!fp) {
        perror("fopen");
        return NULL;
    }

    char errbuf[200];
    toml_table_t* root = toml_parse_file(fp, errbuf, sizeof(errbuf));

    fclose(fp);

    if (!root) {
        fprintf(stderr, "TOML parse error: %s\n", errbuf);
        return NULL;
    }
    
    toml_array_t* publishers   = toml_array_in(root, "publishers");

    SystemConfig * sysConf = malloc(sizeof(SystemConfig));
    int publisherCount = toml_array_nelem(publishers);
    if (publisher == -1){
        sysConf->publishers = malloc(publisherCount * sizeof(PublisherConfig*));
        sysConf->publisherCount = publisherCount;

        for (int i = 0; i < publisherCount; i++){
            toml_table_t* pub = toml_table_at(publishers, i);
            if(!pub) continue;
            sysConf->publishers[i] = parsePublisher(pub);
        }

    } else {
        sysConf->publishers = malloc(sizeof(PublisherConfig*));
        sysConf->publisherCount = 1;

        
        
        toml_table_t* pub = toml_table_at(publishers, publisher);
        if (!pub){
            printf("Publisher not in config\n");
            return NULL;
        } 
        sysConf->publishers[publisher] = parsePublisher(pub);
    }

    return sysConf;
}