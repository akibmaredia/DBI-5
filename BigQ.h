#ifndef BIGQ_H
#define BIGQ_H

#include <pthread.h>
#include <iostream>
#include <utility>

#include "vector"
#include "Pipe.h"
#include "File.h"
#include "Record.h"
#include "ComparisonEngine.h"

using namespace std;

class RecordWrapper {
public:
    Record *current;
    int run_index, page_index;

    RecordWrapper();
    ~RecordWrapper();
};

class Run {
private:
    int run_index;

    off_t startPage;
    off_t endPage;
    off_t currentPage;

    File *temp_file;

    Page page;

public:
    Run(int runIndex, off_t startPage, off_t endPage, File *tempFile);
    Run(const Run &run);

    Run & operator= (const Run &run);

    ~Run();

    int GetNextRecord(Record *record);
};

class RunRecord {
public:
    int run_index;
    Record record;

    RunRecord(int runIndex, Record rec);
    ~RunRecord();
};

class CompareRecord {
private:
    OrderMaker sortOrder;
    ComparisonEngine comparisonEngine;

public:
    CompareRecord(OrderMaker orderMaker);
    ~CompareRecord();

    bool operator()(const Record &left, const Record &right);
};

class CompareRunRecord {
private:
    OrderMaker sortOrder;
    ComparisonEngine comparisonEngine;

public:
    CompareRunRecord(OrderMaker orderMaker);
    ~CompareRunRecord();

    bool operator()(const RunRecord &left, const RunRecord &right);
};

class BigQ {
private:
    Pipe &input_pipe, &output_pipe;
    OrderMaker sortOrder;

    int runLength, total_runs;
    long total_records;

    off_t currentPage_num;

    vector<pair<off_t, off_t> > run_page_offsets;

    File temp_file;

    void SortingPhase();
    void Sort(vector<Record> &run);
    pair<off_t, off_t> WriteInTemp(vector<Record> &run);

    void MergingPhase();


public:
    BigQ(Pipe &input, Pipe &output);
    BigQ(Pipe &input, Pipe &output, OrderMaker &orderMaker, int runLength);

    ~BigQ();

    void * Execute(void);

    bool HeapComparator(const RecordWrapper *left, const RecordWrapper *right);
    bool RecordComparator(const RecordWrapper *left, const RecordWrapper *right);
};

#endif