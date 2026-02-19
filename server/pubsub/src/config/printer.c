#include <stdio.h>
#include "configs.h"

void printFieldConfig(const FieldConfig* f, size_t indent) {
    if (!f) return;
    for (size_t i = 0; i < indent; i++) printf("  ");
    printf("Field: %s (%s)\n",
           f->name ? f->name : "(null)",
           f->type ? f->type : "(null)");
}

void printDataSetConfig(const DataSetConfig* ds, size_t indent) {
    if (!ds) return;
    for (size_t i = 0; i < indent; i++) printf("  ");
    printf("DataSet: %s (writerId=%u)\n",
           ds->name ? ds->name : "(null)",
           ds->writerId);

    for (size_t i = 0; i < ds->fieldCount; i++) {
        printFieldConfig(ds->fields[i], indent + 1);
    }
}

void printWriterGroupConfig(const WriterGroupConfig* wg, size_t indent) {
    if (!wg) return;
    for (size_t i = 0; i < indent; i++) printf("  ");
    printf("WriterGroup: id=%u, interval=%d, name=%s\n", wg->groupId, wg->interval, wg->name);

    for (size_t i = 0; i < wg->dataSetCount; i++) {
        printDataSetConfig(wg->dataSets[i], indent + 1);
    }
}

void printPublisherConfig(const PublisherConfig* pub, size_t indent) {
    if (!pub) return;
    for (size_t i = 0; i < indent; i++) printf("  ");
    printf("Publisher: %s (id=%u)\n",
           pub->name ? pub->name : "(null)", pub->publisherId);

    for (size_t i = 0; i < pub->groupCount; i++) {
        printWriterGroupConfig(pub->writerGroups[i], indent + 1);
    }
}

void printSystemConfig(const SystemConfig* sys) {
    if (!sys) {
        printf("(null SystemConfig)\n");
        return;
    }
    printf("SystemConfig: %zu publishers\n", sys->publisherCount);
    for (size_t i = 0; i < sys->publisherCount; i++) {
        printPublisherConfig(sys->publishers[i], 1);
    }
}
