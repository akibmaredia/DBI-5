#include "Schema.h"
#include "Record.h"

enum fType {
    heap, 
    sorted, 
    tree
};

class GenericDBFile{
public:
	GenericDBFile();
	
    virtual ~GenericDBFile();

	virtual int Create(const char *f_path, fType file_type, void *startup) = 0;
	
    virtual int Open(const char *f_path) = 0;
	
    virtual int Close() = 0;

	virtual void Load(Schema &schema, const char *tbl_file_path) = 0;

	virtual void MoveFirst() = 0;
	
    virtual void Add(Record &record_to_add) = 0;
	
    virtual int GetNext(Record &record_to_fetch) = 0;

	virtual int GetNext(Record &record_to_fetch, CNF &cnf, Record &record) = 0;
};