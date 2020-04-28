#include "Record.h"
#include "Schema.h"
#include "File.h"
#include "Comparison.h"
#include "DBFile.h"

#include <fstream>
#include <cstdlib>
#include <cstring>

void SortedDBFile::Start(thread_arguments *threadArguments) {
    // Invoke BigQ constructor which will sort the records from input pipe and
    // write the merged records into output pipe.
    new BigQ(*(threadArguments->input), *(threadArguments->output),
             *((threadArguments->args).order_maker), (threadArguments->args).run_length);
}

SortedDBFile::SortedDBFile() {
    bigQ  = nullptr;

    input_pipe = new Pipe(100);
    output_pipe = new Pipe(100);

    read_page = new Page();
    write_page = new Page();

    head = new Record();

    query_order = nullptr;
    args = nullptr;

    file = new File();

    mode = READ;

    has_record_to_write = false;
    is_end_of_file = false;
    is_cnf_updated = true;
}

SortedDBFile::~SortedDBFile() {
    delete bigQ;
    delete input_pipe;
    delete output_pipe;
    delete file;
    delete head;
    delete read_page;
    delete write_page;
}

int SortedDBFile::Create(const char *f_path, fType f_type, void *startup) {
    file->Open(0, f_path);
    fileName = (char *) malloc(sizeof(f_path) + 1);
    strcpy(fileName, f_path);
    has_record_to_write = false;
    args = (Args *) startup;
    read_index = 1;
    is_end_of_file = true;
    return 1;
}

int SortedDBFile::Open(const char *f_path) {
    has_record_to_write = false;
    char *fName = new char[20];
    sprintf(fName, "%s.meta", f_path);
    fileName = (char *) malloc(sizeof(f_path) + 1);
    strcpy(fileName, f_path);
    ifstream ifs(fName, ios::binary);

    ifs.seekg(sizeof(fileName) - 1);

    if (args == nullptr) {
        args = new Args;
        args->order_maker = new OrderMaker();
    }

    ifs.read((char *) args->order_maker, sizeof(*(args->order_maker)));
    ifs.read((char *) &(args->run_length), sizeof(args->run_length));
    ifs.close();
    mode = READ;
    file->Open(1, f_path);
    read_index = 1;
    is_end_of_file = false;
}

int SortedDBFile::Close() {
    if (mode == WRITE) EmptyPipe();

    file->Close();
    has_record_to_write = false;
    is_end_of_file = true;
    char fName[30];
    sprintf(fName, "%s.meta", fileName);
    ofstream out(fName);
    out << "sorted" << endl;
    out.close();

    ofstream ofs(fName, ios::binary | ios::app);
    ofs.write((char *) args->order_maker, sizeof(*(args->order_maker)));
    ofs.write((char *) &(args->run_length), sizeof(args->run_length));
    ofs.close();
}

void SortedDBFile::Load(Schema &schema, const char *tbl_file_path) {
    if (mode != WRITE) {
        mode = WRITE;
        has_record_to_write = true;
        if (bigQ == nullptr) {
            bigQ = new BigQ(*input_pipe, *output_pipe, *(args->order_maker), args->run_length);
        }
    }

    FILE *tableFile = fopen(tbl_file_path, "r");
    Record temp;
    while (temp.SuckNextRecord(&schema, tableFile) != 0) {
        input_pipe->Insert(&temp);
    }
    fclose(tableFile);
}

void SortedDBFile::MoveFirst() {
    has_record_to_write = 0;
    read_index = 1;

    if (mode == WRITE) {
        mode == READ;
        EmptyPipe();
        file->GetPage(read_page, read_index);
        read_page->GetFirst(head);
    } else if (file->GetLength() != 0) {
        file->GetPage(read_page, read_index);
        read_page->GetFirst(head);
    }

    is_cnf_updated = true;
}

void SortedDBFile::Add(Record &record_to_add) {
    input_pipe->Insert(&record_to_add);
    if (mode == READ) {
        has_record_to_write = 1;
        mode = WRITE;
        input_pipe = new Pipe(100);
        output_pipe = new Pipe(100);
        if (bigQ == nullptr) {
            thread_args.input = input_pipe;
            thread_args.output = output_pipe;
            thread_args.args.order_maker = args->order_maker;
            thread_args.args.run_length = args->run_length;

            pthread_t worker;
            pthread_create(&worker, nullptr, reinterpret_cast<void *(*)(void *)>(SortedDBFile::Start), (void *) &thread_args);
        }
    }
    is_cnf_updated = true;
}

int SortedDBFile::GetNext(Record &record_to_fetch) {
    if (mode == WRITE) {
        has_record_to_write = 0;
        mode = READ;
        read_page->EmptyItOut();
        EmptyPipe();
        MoveFirst();
    }

    if (!is_end_of_file) {
        record_to_fetch.Copy(head);
        if (read_page->GetFirst(head) != 0) return 1;
        if (read_index < file->GetLength() - 2) {
            read_index++;
            file->GetPage(read_page, read_index);
            read_page->GetFirst(head);
        } else {
            is_end_of_file = true;
            return 0;
        }
        return 1;
    } else {
        return 0;
    }
}

int SortedDBFile::GetNext(Record &record_to_fetch, CNF &cnf, Record &record) {
    if (mode == WRITE) {
        has_record_to_write = 0;
        mode = READ;
        read_page->EmptyItOut();
        EmptyPipe();
        MoveFirst();
    }

    if (is_cnf_updated) {
        query_order = cnf.CreateQueryMaker(*(args->order_maker));
    }

    if (query_order != nullptr) {
        Record *rec;
        rec = GetMatchingRecord(record);
        if (rec != nullptr) {
            record_to_fetch.Consume(rec);
            if (!comparison_engine->Compare(&record_to_fetch, &record, &cnf)) {
                while (GetNext(record_to_fetch)) {
                    if (comparison_engine->Compare(&record_to_fetch, &record, query_order) == 0) {
                        if (!comparison_engine->Compare(&record_to_fetch, &record, &cnf)) continue;
                        return 1;
                    } else {
                        return 0;
                    }
                }
            } else {
                return 1;
            }
        } else {
            return 0;
        }
    } else {
        while (GetNext(record_to_fetch)) {
            if (!comparison_engine->Compare(&record_to_fetch, &record, &cnf)) continue;
            return 1;
        }
        return 0;
    }
    return 0;
}

void SortedDBFile::EmptyPipe() {
    input_pipe->ShutDown();

    if (!write_page) {
        write_page = new Page();
    }

    page_index = 0;
    Record *rec, *removeMe = new Record();
    DoesRecordExists(rec);

    Page *pPage = new Page();
    File *pFile = new File();
    pFile->Open(0, "merge.tmp");

    bool is_file_empty = false;
    int result = DoesRecordExists(rec);
    int page_count = 1;

    if (result == 0) is_file_empty = true;

    if (has_record_to_write != 0) {
        while (!is_file_empty) {
            if (output_pipe->Remove(removeMe) == 1) {
                while (comparison_engine->Compare(rec, removeMe, args->order_maker) < 0) {
                    if (pPage->Append(rec) == 0) {
                        pFile->AddPage(pPage, page_count++);
                        pPage->EmptyItOut();
                        pPage->Append(rec);
                    }
                    if (!DoesRecordExists(rec)) {
                        is_file_empty = true;
                        break;
                    }
                }
                if (pPage->Append(removeMe) == 1) continue;
                pFile->AddPage(pPage, page_count++);
                pPage->EmptyItOut();
                pPage->Append(removeMe);
            } else {
                do {
                    if (pPage->Append(rec) != 1) {
                        pFile->AddPage(pPage, page_count++);
                        pPage->EmptyItOut();
                        pPage->Append(rec);
                    }
                } while (DoesRecordExists(rec) != 0);
                break;
            }
        }
    }

    output_pipe->Remove(removeMe);
    if (is_file_empty) {
        do {
            if (pPage->Append(removeMe) != 1) {
                pFile->AddPage(pPage, page_count++);
                pPage->EmptyItOut();
                pPage->Append(removeMe);
            }
        } while (output_pipe->Remove(removeMe) != 0);
    }

    pFile->AddPage(pPage, page_count);
    pFile->Close();
    file->Close();

    if (rename(fileName, "combine.tmp") == 0) {
        remove("combine.tmp");
        if (rename("merge.tmp", fileName) == 0) {
            read_page->EmptyItOut();
            file->Open(1, fileName);
        }
    }
}

int SortedDBFile::BinarySearch(int low, int high, OrderMaker *order_maker, Record &record_to_find) {
    if(high < low) return -1;

    if(high == low) return low;

    auto *page = new Page;
    auto *record = new Record;

    int mid = (int) (high + low) / 2;
    file->GetPage(page, mid);
    page->GetFirst(record);

    int res = comparison_engine->Compare(record, args->order_maker, &record_to_find, order_maker);

    delete page;
    delete record;

    return res == -1 ? low == mid ? mid : BinarySearch(low, mid - 1, order_maker, record_to_find) : res == 0 ?
                        mid : BinarySearch(mid + 1, high, order_maker, record_to_find);
}

int SortedDBFile::DoesRecordExists(Record *record) {
    while (!write_page->GetFirst(record)) {
        if (page_index >= file->GetLength() - 1) {
            return 0;
        } else {
            file->GetPage(write_page, page_index);
            page_index++;
        }
    }
    return 1;
}

Record* SortedDBFile::GetMatchingRecord(Record &record) {
    if(is_cnf_updated) {
        int low = read_index, high = file->GetLength() - 2;

        int index = BinarySearch(low, high, query_order, record);
        if(index != -1) {
            if(index != read_index) {
                read_page->EmptyItOut();
                file->GetPage(read_page, index);
                read_index = index + 1;
            }
            is_cnf_updated = false;
        }
        else {
            return nullptr;
        }
    }

    auto *res = new Record;
    while (read_page->GetFirst(res)) {
        if(comparison_engine->Compare(res, &record, query_order) != 0) continue;
        return res;
    }

    if (read_index >= file->GetLength() - 2) {
        return nullptr;
    }
    else {
        read_index++;
        file->GetPage(read_page, read_index);

        while (read_page->GetFirst(res)) {
            if(comparison_engine->Compare(res, &record, query_order) != 0) continue;
            return res;
        }
    }

    return nullptr;
}