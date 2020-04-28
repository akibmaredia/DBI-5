#include "TwoWayList.h"
#include "Record.h"
#include "Schema.h"
#include "File.h"
#include "Comparison.h"
#include "ComparisonEngine.h"
#include "Defs.h"
#include "TwoWayList.h"
#include "Record.h"
#include "DBFile.h"
#include <string.h>
#include "SortedDBFile.h"
#include "HeapDBFile.h"

DBFile::DBFile() {
    file = nullptr;
}

DBFile::~DBFile() {
    delete file;
}

int DBFile::Create(const char *f_path, fType f_type, void *startup) {
    if (f_type == heap) {
        file = new HeapDBFile();
    } else if (f_type == sorted) {
        file = new SortedDBFile();
    } else {
       return 0;
    }
    file->Create(f_path, f_type, startup);
}

void DBFile::Load(Schema &schema, const char *tbl_file_path) {
    file->Load(schema, tbl_file_path);
}

int DBFile::Open(const char *f_path) {
    if (f_path == nullptr || f_path[0] == '\0') {
        return 0;
    }
    char metaDataFileName[20], f_type[20];
    sprintf(metaDataFileName, "%s.meta", f_path);
    FILE *meta = fopen(metaDataFileName, "r");
    fscanf(meta, "%s", f_type);

    if (strcmp(f_type, "heap") == 0) {
        file = new HeapDBFile();
    } else if (strcmp(f_type, "sorted") == 0) {
        file = new SortedDBFile();
    }

    file->Open(f_path);
}

void DBFile::MoveFirst() {
    file->MoveFirst();
}

int DBFile::Close() {
    return file->Close();
}

void DBFile::Add(Record &record_to_add) {
    return file->Add(record_to_add);
}

int DBFile::GetNext(Record &record_to_fetch) {
    return file->GetNext(record_to_fetch);
}

int DBFile::GetNext(Record &record_to_fetch, CNF &cnf, Record &record) {
    return file->GetNext(record_to_fetch, cnf, record);
}