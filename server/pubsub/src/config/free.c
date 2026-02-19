#include <stdlib.h>
#include "configs.h"

void freeFieldConfig(FieldConfig* f) {
    if (!f) return;
    free(f->name);
    free(f->type);
    free(f);
}

void freeDataSetConfig(DataSetConfig* ds) {
    if (!ds) return;
    free(ds->name);
    for (size_t i = 0; i < ds->fieldCount; i++) {
        freeFieldConfig(ds->fields[i]);
    }
    free(ds->fields);
    free(ds);
}

void freeWriterGroupConfig(WriterGroupConfig* wg) {
    if (!wg) return;
    for (size_t i = 0; i < wg->dataSetCount; i++) {
        freeDataSetConfig(wg->dataSets[i]);
    }
    free(wg->dataSets);
    free(wg);
}

void freePublisherConfig(PublisherConfig* pub) {
    if (!pub) return;
    free(pub->name);
    for (size_t i = 0; i < pub->groupCount; i++) {
        freeWriterGroupConfig(pub->writerGroups[i]);
    }
    free(pub->writerGroups);
    free(pub);
}

void freeSystemConfig(SystemConfig* sys) {
    if (!sys) return;
    for (size_t i = 0; i < sys->publisherCount; i++) {
        freePublisherConfig(sys->publishers[i]);
    }
    free(sys->publishers);
    free(sys);
}
