#include "Sql.h"
#include <stdio.h>

using namespace std;

extern "C" {
    int yyparse(void);
}

extern int commandType;
extern int dbfileType;
extern string tablename;
extern string loadFileName;
extern string outputFile;
extern struct AttrList *attrList;
extern struct NameList *sortList;

void Sql :: Create() {

}

void Sql :: Open() {
    
}

void Sql :: Execute() {

}

void Sql :: Close() {
    
}

