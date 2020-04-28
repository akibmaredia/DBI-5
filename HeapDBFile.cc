#include "TwoWayList.h"
#include "Record.h"
#include "Schema.h"
#include "File.h"
#include "Comparison.h"
#include "ComparisonEngine.h"
#include "Defs.h"
#include "HeapDBFile.h"

HeapDBFile::HeapDBFile() {
	file = new File();
    readPage = new Page();
    writePage = new Page();
    head = new Record();
    comparisonEngine = new ComparisonEngine();
}

HeapDBFile::~HeapDBFile() {
	delete file;
    delete readPage;
    delete writePage;
    delete head;
    delete comparisonEngine;
}

int HeapDBFile::Create(const char *f_path, fType f_type, void *startup) {
    if (f_path == NULL || f_path[0] == '\0' || f_type < heap || f_type > tree) {
        return 0;
    }

    filePath = (char *) f_path;

    file->Open(0, f_path);

    readIndex = 1;
    writeIndex = 1;
    toWrite = false;
    eof = true;

    return 1;
}

void HeapDBFile::Load(Schema &schema, const char *tbl_file_path) {
    FILE *file = NULL;
    file = fopen(tbl_file_path, "r");
    if(file != NULL) {
        Record *tempRecord = new Record();
        while(tempRecord->SuckNextRecord(&schema, file)) {
            Add(*(tempRecord));
        }
        delete tempRecord;
        fclose(file);
    } else {
        std::cerr << "HeapDBFile::Load - Error in opening the file: " << tbl_file_path << std::endl;
    }
}

int HeapDBFile::Open(const char *f_path) {
    if (f_path == NULL || f_path[0] == '\0') {
        return 0;
    }

    filePath = (char *) f_path;

    file->Open(1, f_path);

    readIndex = 1;
    eof = false;

    return 1;
}

void HeapDBFile::MoveFirst() {
    file->GetPage(readPage, 1);
    readPage->GetFirst(head);
}

int HeapDBFile::Close() {
    if(toWrite) {
        WriteToFile();
    }
    char fileName[20];
    sprintf(fileName, "%s.meta", filePath);
    FILE *meta = fopen(fileName,"w");
    fprintf(meta, "%s", "heap");

    eof = true;
    return file->Close();
}

void HeapDBFile::WriteToFile() {
    file->AddPage(writePage, writeIndex++);
    writePage->EmptyItOut();
}





// //TODO: use File::AppendPage in this function after below code works fine
// void HeapDBFile::Add (Record &rec) {
//   if (!writingMode && curr_page.NumRecords() != 0){
//     curr_page.EmptyItOut();

//     //if last page written to file was not full then we will get that page and write in it otherwise in a new page
//     if(db_file.GetLength() > 0){
//       //cout<<"curr page records: "<<curr_page.NumRecords()<<endl;
//       //cout<<"file length: "<<db_file.GetLength();

//       off_t page_no = db_file.GetLength() - 2;
//       db_file.GetPage(&curr_page, page_no);

//       if(page_no == 1) db_file.SetLength(0);
//       else db_file.SetLength(page_no);

//       //cout<<"curr page records: "<<curr_page.NumRecords()<<endl;
//       //cout<<"file length: "<<db_file.GetLength();
//     }
//   }
//   writingMode = true;

//   //write record to page
//   Record temp;
//   temp.Consume(&rec);
//   int isAppended = curr_page.Append(&temp);
//   num_rec++;

//   if (!isAppended){
//     //if empty file then page number starts with 0 else current file length - 1
//     off_t page_no = db_file.GetLength();
//     if (page_no != 0) page_no--;

//     //page is full, write page to file on disk
//     db_file.AddPage(&curr_page, page_no);
//     curr_page.EmptyItOut();

//     //write record to page
//     curr_page.Append(&temp);
//   }
// }

// int HeapDBFile::GetNext (Record &fetchme) {
//   //if there is anything that is not written to file yet, write it
//   if (writingMode && curr_page.NumRecords() != 0){
//     //if empty file then page number starts with 0 else current file length - 1
//     off_t page_no = db_file.GetLength();
//     if (page_no != 0) page_no--;

//     db_file.AddPage(&curr_page, page_no);
//     curr_page.EmptyItOut();

//     //fetch current page from disk for reading
//     //db_file.GetPage(&curr_page, curr_page_index);
//   }
//   writingMode = false;

//   if (!writingMode && curr_page.NumRecords() == 0){
//     //if reached at the end of current page then fetch next page if current page is not last page
//     if (curr_page_index < db_file.GetLength()-1){
//       db_file.GetPage(&curr_page, curr_page_index);
//       curr_page_index++;
//     }
//     else{
//       return 0;
//     }
//   }
  
//   return curr_page.GetFirst(&fetchme);
// }




void HeapDBFile::Add(Record &record_to_add) {
    toWrite = true;

    if(writePage->GetCurrentSize() + record_to_add.GetSize() > PAGE_SIZE) {
        WriteToFile();
    }

    if(!writePage->Append(&record_to_add)) {
        std::cerr << "Heap::Add - Error adding record to page." << std::endl;
    }
}

int HeapDBFile::GetNext(Record &record_to_fetch) {
    if(!eof) {
        record_to_fetch.Copy(head);
        if (!readPage->GetFirst(head)) {
            if (++readIndex < file->GetLength () - 1) {
                file->GetPage(readPage, readIndex);
                readPage->GetFirst(head);
            } else {
                eof = true;
            }
        }
        return 1;
    }
    return 0;
}

int HeapDBFile::GetNext(Record &record_to_fetch, CNF &cnf, Record &record) {
    while (true) {
        int result = GetNext(record_to_fetch);
        if(!result) return 0;

        result = comparisonEngine->Compare(&record_to_fetch, &record, &cnf);
        if(result) break;
    }
    return 1;
}