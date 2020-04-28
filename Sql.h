#ifndef SQL_h
#define SQL_h

#include "ParseTree.h"
#include "Statistics.h"
#include "Schema.h"
#include "MainScheme.h"
#include "string.h"
#include <iostream>
#include <float.h>

class Sql {
    
private:
    const string statistics_path = "db/Statistics";
    const string DBInfo = "db/DBInfo";
    const string catalog_path = "db/catalog";
    const string db_dir = "db/";
    const string tpch_dir = "/cise/tmp/dbi_sp11/DATA/10M/";
    
    int numofTables = 0;
    unordered_map<string, int> tablesInDB;
    unordered_map<string, Schema*> schemas;
    int outputSet = 1; // 0 for NONE, 1 for STDOUT, 2 for 'myfile'
    Statistics s;
    
    void shuffleOrderHelper(vector<string> &seenTable, int index, vector<vector<string>> &res, vector<string> &tmpres);
    vector<vector<string>> shuffleOrder(vector<string> &seenTable);
    void traverse(MainScheme *currNode, int mode);
    void copySchema(unordered_map<string, Schema*> &aliasSchemas, char* oldName, char* newName);
    void selectSQL();
    
public:
    Sql() {};
    ~Sql() {};
    void Create();
    void Open();
    void Execute();
    void Close();
};

#endif