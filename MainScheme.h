#include "Schema.h"
#include "Comparison.h"
#include "Record.h"
#include "Function.h"
#include "ParseTree.h"
#include "Statistics.h"
#include <iostream>
#include <map>

extern "C" {
int yyparse(void);
}
extern "C" struct YY_BUFFER_STATE *yy_scan_string(const char *);


extern struct FuncOperator *finalFunction;


extern struct TableList *tables;

extern struct AndList *boolean;

extern struct NameList *groupingAtts;

extern struct NameList *attsToSelect;

extern int distinctAttrs;

extern int distinctFunc;

typedef map <string, Schema> SchemeStore;
typedef map <string, string> AliasMap;

enum NodeType {
    SELECT_FILE,
    SELECT_PIPE,
    PROJECT,
    SUM,
    DISTINCT,
    GROUP_BY,
    JOIN
};

class MainScheme {
public:
    char *REGION = "region", *NATION = "nation", *PART = "part",
            *SUPPLIER = "supplier", *PARTSUPP = "partsupp", *CUSTOMER = "customer",
            *ORDERS = "orders", *LINEITEM = "lineitem";

    const int REGION_TUPLE_COUNT = 5, NATION_TUPLE_COUNT = 25,
            PART_TUPLE_COUNT = 200000, SUPPLIER_TUPLE_COUNT = 10000,
            PARTSUPP_TUPLE_COUNT = 800000, CUSTOMER_TUPLE_COUNT = 150000,
            ORDERS_TUPLE_COUNT = 1500000, LINEITEM_TUPLE_COUNT = 6001215;

    void initializeSchemeStore(SchemeStore &schemeStore);
    void initializeStatistics(Statistics &statistics);
};

class Node {
public:
    int pipeId{};

    NodeType nodeType;

    Schema out;

    Node();

    explicit Node(NodeType type) : nodeType(type) {}
    ~Node() = default;

    virtual void Print() {};
};


class ProjectTreeNode: public Node {
public:
    int inNum{}, outNum{};
    int *attrsToKeep{};

    Node *from{};

    ProjectTreeNode() : Node(PROJECT) {}

    ~ProjectTreeNode() ;

    void Print() override;
};


class SelectFileTreeNode : public Node {
public:
    CNF cnf;
    Record record;
    Node *from{};

    SelectFileTreeNode() : Node(SELECT_FILE) {}

    ~SelectFileTreeNode();

    void Print() override;
};


class SelectPipeTreeNode : public Node {
public:
    CNF cnf;
    Record record;
    Node *from{};
    string cnfStr;

    SelectPipeTreeNode() : Node(SELECT_PIPE) {}
    ~SelectPipeTreeNode();

    void Print() override;
};

class SumTreeNode : public Node {
public:
    Function func;
    Node *from;

    SumTreeNode() : Node(SUM) {}

    ~SumTreeNode();

    void Print() override;
};





class DistinctTreeNode : public Node {
public:
    Node *from{};

    DistinctTreeNode() : Node(DISTINCT) {}

    ~DistinctTreeNode();

    void Print() override;
};


class JoinTreeNode : public Node {
public:
    Node *left, *right;
    CNF cnf;
    Record record;
    Node *from;
    string cnfStr;

    JoinTreeNode() : Node(JOIN) {}

    ~JoinTreeNode();

    void Print() override;
};


class GroupByTreeNode : public Node {
public:
    Node *from;
    Function func;
    OrderMaker group;

    GroupByTreeNode() : Node(GROUP_BY) {}

    ~GroupByTreeNode();

    void Print() override;
};

