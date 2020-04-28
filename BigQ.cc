#include <algorithm>
#include <cstdio>
#include <queue>
#include <utility>

#include "BigQ.h"

using namespace std;

void *Start(void *obj) {
    BigQ *bigQ = (BigQ*) obj;
    bigQ->Execute();
}

BigQ::BigQ(Pipe &input, Pipe &output) : input_pipe(input), output_pipe(output) {}

BigQ::BigQ(Pipe &input, Pipe &output, OrderMaker &orderMaker, int runLength) :
    input_pipe(input), output_pipe(output), sortOrder(orderMaker), runLength(runLength), total_runs(0), total_records(0), currentPage_num(0) {
    pthread_t worker;
    pthread_create(&worker, NULL, &Start, this);
}

BigQ::~BigQ () {}

void * BigQ::Execute(void) {
    // Create temporary file to store sorted runs.
    char name[] = "/tmp/tempfileXXXXXX";
    temp_file.OpenTemp(name);

    SortingPhase();

    MergingPhase();

    output_pipe.ShutDown();

    temp_file.Close();

    remove(name);

    pthread_exit(NULL);
}

void BigQ::SortingPhase() {
    const size_t max_run_size = runLength * PAGE_SIZE;
    size_t curr_run_size = 0;

    vector<Record> run;
    run.reserve(runLength);

    Record temp;
    while (input_pipe.Remove(&temp)){
        total_records++;
        size_t rec_size = temp.GetSize();

        if (curr_run_size + rec_size > max_run_size) {
            Sort(run);
            pair<off_t, off_t> offset = WriteInTemp(run);
            run_page_offsets.push_back(offset);

            total_runs++;
            run.clear();
            curr_run_size = 0;
        }

        run.push_back(temp);
        curr_run_size += rec_size;
    }

    if(curr_run_size > 0) {
        Sort(run);
        pair<off_t, off_t> offset = WriteInTemp(run);
        run_page_offsets.push_back(offset);

        total_runs++;
        run.clear();
        curr_run_size = 0;
    }
}

void BigQ::Sort(vector<Record> &run) {
    CompareRecord compareRec(sortOrder);
    sort(run.begin(), run.end(), compareRec);
}

pair<off_t, off_t> BigQ::WriteInTemp(vector <Record> &run) {
    off_t start = currentPage_num;

    Page page;
    for (Record record: run) {
        if (!page.Append(&record)) {
            temp_file.AddPage(&page, currentPage_num);
            page.EmptyItOut();
            currentPage_num++;
            page.Append(&record);
        }
    }

    temp_file.AddPage(&page, currentPage_num);
    page.EmptyItOut();
    currentPage_num++;

    return make_pair(start, currentPage_num);
}

void BigQ::MergingPhase() {
    if (total_runs <= RUN_THRESHOLD) {
        vector<Run> runs;
    runs.reserve(total_runs);
    for(int count = 0; count < total_runs; count++) {
        runs.emplace_back(Run(count, run_page_offsets[count].first, run_page_offsets[count].second, &temp_file));
    }

    vector<Record> records;
    records.reserve(total_runs);
    for(int count = 0; count < total_runs; count++) {
        Record temp;
        runs[count].GetNextRecord(&temp);
        records.push_back(temp);
    }

    CompareRecord compareRec(sortOrder);
    for(int count = 0; count < total_records; count++) {
        int index = distance(records.begin(), min_element(records.begin(), records.end(), compareRec));

        output_pipe.Insert(&records[index]);

        if(!runs[index].GetNextRecord(&records[index])){
            records.erase(records.begin() + index);
            runs.erase(runs.begin() + index);
        }
    }
    } else {
        vector<Run> runs;
    runs.reserve(total_runs);
    for (int count = 0; count< total_runs; count++) {
        runs.emplace_back(Run(count, run_page_offsets[count].first, run_page_offsets[count].second, &temp_file));
    }

    priority_queue<RunRecord, vector<RunRecord>, CompareRunRecord> pq(sortOrder);
    for (int count = 0; count < total_runs; count++) {
        Record temp;
        runs[count].GetNextRecord(&temp);
        pq.push(RunRecord(count, temp));
    }

    for (int count = 0; count < total_records; count++) {
        RunRecord rr(pq.top());
        Record r(rr.record);
        int index = rr.run_index;

        output_pipe.Insert(&r);
        pq.pop();

        if(runs[index].GetNextRecord(&r)) {
            pq.push(RunRecord(index, r));
        }
    }
    }
}

Run::Run(int runIndex, off_t startPage, off_t endPage, File *tempFile) :
    run_index(runIndex), startPage(startPage), endPage(endPage), currentPage(startPage), temp_file(tempFile) {
    temp_file->GetPage(&page, startPage);
}

Run::Run(const Run &run) : run_index(run.run_index), startPage(run.startPage), endPage(run.endPage),
    currentPage(run.currentPage), temp_file(run.temp_file) {
    temp_file->AddPage(const_cast<Page *>(&run.page), currentPage);
    temp_file->GetPage(&page, currentPage);
}

Run & Run::operator= (const Run &run) {
    run_index = run.run_index;
    startPage= run.startPage;
    endPage = run.endPage;
    currentPage = run.currentPage;

    run.temp_file->AddPage(const_cast<Page *>(&run.page), currentPage);
    temp_file = run.temp_file;
    temp_file->GetPage(&page, currentPage);
    return * this;
}

Run::~Run() {}

int Run::GetNextRecord(Record *record) {
    if (!page.GetFirst(record)) {
        if (currentPage < endPage - 1) {
            page.EmptyItOut();
            currentPage++;
            temp_file->GetPage(&page, currentPage);
            return page.GetFirst(record);
        } else {
            return 0;
        }
    }
    return 1;
}





// void BigQ::SortRun(vector<Record> &run){
// 	CompareRecord comp(sortorder);
// 	sort(run.begin(), run.end(), comp);
// }

// pair<off_t, off_t> BigQ::WriteRunToTempFile(vector<Record> &run){
// 	off_t start = curr_pageno;

// 	Page page;
// 	for(Record r: run){
// 		if(!page.Append(&r)){
// 			tempfile.AddPage(&page, curr_pageno);
// 			page.EmptyItOut();
// 			curr_pageno++;
			
// 			page.Append(&r);
// 		}
// 	}

// 	tempfile.AddPage(&page, curr_pageno);
// 	page.EmptyItOut();
// 	curr_pageno++;

// 	return make_pair(start, curr_pageno);
// }









// Run::Run(int _runid, off_t _startpage, off_t _endpage, File *_tempfile): 
// runid(_runid), startpage(_startpage), endpage(_endpage), currpage(_startpage), tempfile(_tempfile) {
// 	tempfile->GetPage(&page, startpage);
// }

// Run::Run(const Run &run) :
//   runid(run.runid), startpage(run.startpage), endpage(run.endpage), currpage(run.currpage), tempfile(run.tempfile){
//       tempfile->AddPage(const_cast<Page *>(&run.page), currpage);
//       tempfile->GetPage(&page,currpage);
//   }

// Run & Run::operator= (const Run &run){
//       runid = run.runid;
//       startpage = run.startpage;
//       endpage = run.endpage;
//       currpage = run.currpage; 

//       run.tempfile->AddPage(const_cast<Page *>(&run.page),currpage);
//       tempfile = run.tempfile;
//       tempfile->GetPage(&page,currpage);
//       return * this;
// }

// Run::~Run(){}

// int Run::GetNextRecord(Record *record){
// 	if(!page.GetFirst(record)){
// 		if(currpage < endpage-1){
// 			page.EmptyItOut();
// 			currpage++;
// 			tempfile->GetPage(&page, currpage);

// 			return page.GetFirst(record);
// 		}else{
// 			return 0;
// 		}
// 	}

// 	return 1;
// }

RunRecord::RunRecord(int runIndex, Record rec) : run_index(runIndex), record(rec) {}

RunRecord::~RunRecord() {}

CompareRecord::CompareRecord(OrderMaker orderMaker) : sortOrder(orderMaker) {}

CompareRecord::~CompareRecord() {}

bool CompareRecord::operator()(const Record &left, const Record &right){
    return (comparisonEngine.Compare(const_cast<Record*>(&left), const_cast<Record*>(&right), &sortOrder) < 0);
}

CompareRunRecord::CompareRunRecord(OrderMaker orderMaker): sortOrder(orderMaker) {}

CompareRunRecord::~CompareRunRecord() {}

bool CompareRunRecord::operator()(const RunRecord &left, const RunRecord &right){
    return (comparisonEngine.Compare(const_cast<Record*>(&left.record), const_cast<Record*>(&right.record), &sortOrder) < 0);
}


OrderMaker *orderMaker;
bool BigQ::HeapComparator(const RecordWrapper *left, const RecordWrapper *right) {
    if (right == nullptr || left == nullptr) {
        return false;
    }

    ComparisonEngine comparisonEngine;
    return comparisonEngine.Compare(left->current, right->current, orderMaker) >= 0;
}

bool BigQ::RecordComparator(const RecordWrapper *left, const RecordWrapper *right) {
    if (right == nullptr || left == nullptr) {
        return false;
    }

    ComparisonEngine comparisonEngine;
    return comparisonEngine.Compare(left->current, right->current, orderMaker) < 0;
}