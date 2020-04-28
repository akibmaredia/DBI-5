#ifndef DBFILE_H
#define DBFILE_H

#include "TwoWayList.h"
#include "Record.h"
#include "Schema.h"
#include "File.h"
#include "Comparison.h"
#include "ComparisonEngine.h"
#include "BigQ.h"
#include "GenericDBFile.h"
#include "HeapDBFile.h"
#include "SortedDBFile.h"

class DBFile {
public:
    GenericDBFile *file;

    DBFile();
    ~DBFile();

    int Create(const char *f_path, fType file_type, void *startup);

    int Open(const char *f_path);

    int Close();

    void Load(Schema &schema, const char *tbl_file_path);

    void MoveFirst();

    void Add(Record &record_to_add);

    int GetNext(Record &record_to_fetch);

    int GetNext(Record &record_to_fetch, CNF &cnf, Record &record);
};
#endif