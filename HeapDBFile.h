#ifndef HEAPFILE_H
#define HEAPFILE_H

#include "TwoWayList.h"
#include "Record.h"
#include "Schema.h"
#include "File.h"
#include "Comparison.h"
#include "ComparisonEngine.h"
#include "DBFile.h"

class HeapDBFile : public GenericDBFile{
	File *file;
	char *filePath;

	Page *readPage, *writePage;
	off_t readIndex, writeIndex;

	Record *head;

	ComparisonEngine *comparisonEngine;

    bool toWrite, eof;

private:
    void WriteToFile();

public:
	HeapDBFile();
	~HeapDBFile();

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
