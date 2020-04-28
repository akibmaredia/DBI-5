#ifndef SORTED_FILE_H
#define SORTED_FILE_H

#include "TwoWayList.h"
#include "Record.h"
#include "Schema.h"
#include "File.h"
#include "Comparison.h"
#include "ComparisonEngine.h"

struct Args {
	OrderMaker *order_maker;
	int run_length;
};

enum Mode {READ, WRITE};

class SortedDBFile : public GenericDBFile {
private:
    BigQ *bigQ;

    Pipe *input_pipe, *output_pipe;
    Page *read_page, *write_page;

    Record *head;

    OrderMaker *query_order;

    ComparisonEngine *comparison_engine;

    Args *args;

    File* file;
    char *fileName;

    Mode mode;

    int page_index;
    off_t read_index;

    bool has_record_to_write, is_end_of_file, is_cnf_updated;

	struct thread_arguments {
		Pipe *input, *output;
		Args args;
	} thread_args;
	typedef struct thread_arguments thread_arguments;

private:
    void EmptyPipe();

    int BinarySearch(int low, int high, OrderMaker *order_maker, Record &record_to_find);

    int DoesRecordExists(Record *record);

    Record* GetMatchingRecord(Record &record);

public:
	SortedDBFile();
    ~SortedDBFile();

    int Create(const char *f_path, fType f_type, void *startup);

    int Open(const char *f_path);

    int Close();

    void Load(Schema &schema, const char *tbl_file_path);

	void MoveFirst();

	void Add(Record &record_to_add);

	int GetNext(Record &record_to_fetch);

	int GetNext(Record &record_to_fetch, CNF &cnf, Record &record);

    static void Start(thread_arguments  *);
};

#endif