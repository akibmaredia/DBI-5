#include "MainScheme.h"



void SelectFileTreeNode::Print() {
    if (from) from->Print();
    cout << "\n ******************** " << endl;
    cout << "SELECT FILE operation" << endl;
    cout << "INPUT PIPE ID : 0" << endl;
    cout << "OUTPUT PIPE ID : " << pipeId << endl;
    cout << "Output Schema:" << endl;
    out.Print();
}


void SelectPipeTreeNode::Print() {
    from->Print();
    cout << "\n ******************** " << endl;
    cout << "SELECT PIPE operation" << endl;
    cout << "INPUT PIPE ID : " << from->pipeId << endl;
    cout << "OUTPUT PIPE ID : " << pipeId << endl;
    cout << "Output Schema:" << endl;
    out.Print();
    cout << "CNF:" << endl;
    cout << "\t(" << cnfStr << ")" << endl;
}

void ProjectTreeNode::Print() {
    from->Print();
    cout << "\n ******************** " << endl;
    cout << "PROJECT operation" << endl;
    cout << "INPUT PIPE ID : " << from->pipeId << endl;
    cout << "OUTPUT PIPE ID : " << pipeId << endl;
    cout << "Output Schema:" << endl;
    out.Print();
}



void DistinctTreeNode::Print() {
    from->Print();
    cout << "\n ******************** " << endl;
    cout << "DISTINCT operation" << endl;
    cout << "INPUT PIPE ID : " << from->pipeId << endl;
    cout << "OUTPUT PIPE ID : " << pipeId << endl;
}







void SumTreeNode::Print() {
    from->Print();
    cout << "\n ******************************************* " << endl;
    cout << "SUM operation" << endl;
    cout << "INPUT PIPE ID : " << from->pipeId << endl;
    cout << "OUTPUT PIPE ID : " << pipeId << endl;
    cout << "Function :" << endl;
    func.Print();
    cout << "Output Schema : " << endl;
    out.Print();
}




void JoinTreeNode::Print() {
    from->Print();
    cout << "\n ******************** " << endl;
    cout << "JOIN operation" << endl;
    cout << "Left INPUT PIPE ID : " << left->pipeId << endl;
    cout << "Right INPUT PIPE ID : " << right->pipeId << endl;
    cout << "OUTPUT PIPE ID : " << pipeId << endl;
    cout << "Output Schema : " << endl;
    out.Print();
    cout << "CNF:" << endl;
    cout << "\t(" << cnfStr << ")" << endl;
}


void GroupByTreeNode::Print() {
    from->Print();
    cout << "\n ******************** " << endl;
    cout << "GROUP BY operation" << endl;
    cout << "INPUT PIPE ID : " << from->pipeId << endl;
    cout << "OUTPUT PIPE ID : " << pipeId << endl;
    cout << "Output Schema : " << endl;
    out.Print();
    cout << "Function : " << endl;
    func.Print();
    cout << "OrderMaker : " << endl;
    group.Print();
}





void MainScheme::initializeSchemeStore(SchemeStore &schemeStore) {
    schemeStore[string(REGION)] = Schema("catalog", REGION);
    schemeStore[string(NATION)] = Schema("catalog", NATION);
    schemeStore[string(PART)] = Schema("catalog", PART);
    schemeStore[string(SUPPLIER)] = Schema("catalog", SUPPLIER);
    schemeStore[string(PARTSUPP)] = Schema("catalog", PARTSUPP);
    schemeStore[string(CUSTOMER)] = Schema("catalog", CUSTOMER);
    schemeStore[string(ORDERS)] = Schema("catalog", ORDERS);
    schemeStore[string(LINEITEM)] = Schema("catalog", LINEITEM);
}




//initialize statistics obj
void MainScheme::initializeStatistics(Statistics &stat) {
    map<char *, int> relations({{REGION,   REGION_TUPLE_COUNT},
                                {NATION,   NATION_TUPLE_COUNT},
                                {PART,     PART_TUPLE_COUNT},
                                {SUPPLIER, SUPPLIER_TUPLE_COUNT},
                                {PARTSUPP, PARTSUPP_TUPLE_COUNT},
                                {CUSTOMER, CUSTOMER_TUPLE_COUNT},
                                {ORDERS,   ORDERS_TUPLE_COUNT},
                                {LINEITEM, LINEITEM_TUPLE_COUNT}});

    for (auto &rel : relations) {
        stat.AddRel(rel.first, rel.second);
    }

    map<char *, map<char *, int>> attributes(
            {
                    {"region",
                            {{"r_regionkey", REGION_TUPLE_COUNT},
                                    {"r_name",     REGION_TUPLE_COUNT},
                                    {"r_comment",     REGION_TUPLE_COUNT}}},
                    
                    {NATION,
                            {{"n_nationkey", NATION_TUPLE_COUNT},
                                    {"n_name",     NATION_TUPLE_COUNT},
                                    {"n_regionkey",   REGION_TUPLE_COUNT},
                                    {"n_comment",     NATION_TUPLE_COUNT}}},
                   
                    {PART,
                            {{"p_partkey",   PART_TUPLE_COUNT},
                                    {"p_name",     PART_TUPLE_COUNT},
                                    {"p_mfgr",        PART_TUPLE_COUNT},
                                    {"p_brand",       PART_TUPLE_COUNT},
                                    {"p_type",      PART_TUPLE_COUNT},
                                    {"p_size",          PART_TUPLE_COUNT},
                                    {"p_container",  PART_TUPLE_COUNT},
                                    {"p_retailprice",  PART_TUPLE_COUNT},
                                    {"p_comment",    PART_TUPLE_COUNT}}},
                    
                    {SUPPLIER,
                            {{"s_suppkey",   SUPPLIER_TUPLE_COUNT},
                                    {"s_name",     SUPPLIER_TUPLE_COUNT},
                                    {"s_address",     SUPPLIER_TUPLE_COUNT},
                                    {"s_nationkey",   NATION_TUPLE_COUNT},
                                    {"s_phone",     SUPPLIER_TUPLE_COUNT},
                                    {"s_acctbal",       SUPPLIER_TUPLE_COUNT},
                                    {"s_comment",    SUPPLIER_TUPLE_COUNT}}},
                    
                    {PARTSUPP,
                            {{"ps_partkey",  PART_TUPLE_COUNT},
                                    {"ps_suppkey", SUPPLIER_TUPLE_COUNT},
                                    {"ps_availqty",   PARTSUPP_TUPLE_COUNT},
                                    {"ps_supplycost", PARTSUPP_TUPLE_COUNT},
                                    {"ps_comment",  PARTSUPP_TUPLE_COUNT}}},
                    {CUSTOMER,
                            {{"c_custkey",   CUSTOMER_TUPLE_COUNT},
                                    {"c_name",     CUSTOMER_TUPLE_COUNT},
                                    {"c_address",     CUSTOMER_TUPLE_COUNT},
                                    {"c_nationkey",   NATION_TUPLE_COUNT},
                                    {"c_phone",     CUSTOMER_TUPLE_COUNT},
                                    {"c_acctbal",       CUSTOMER_TUPLE_COUNT},
                                    {"c_mktsegment", 5},
                                    {"c_comment",      CUSTOMER_TUPLE_COUNT}}},
                    {ORDERS,
                            {{"o_orderkey",  ORDERS_TUPLE_COUNT},
                                    {"o_custkey",  CUSTOMER_TUPLE_COUNT},
                                    {"o_orderstatus", 3},
                                    {"o_totalprice",  ORDERS_TUPLE_COUNT},
                                    {"o_orderdate", ORDERS_TUPLE_COUNT},
                                    {"o_orderpriority", 5},
                                    {"o_clerk",      ORDERS_TUPLE_COUNT},
                                    {"o_shippriority", 1},
                                    {"o_comment",    ORDERS_TUPLE_COUNT}}},
                    {LINEITEM,
                            {{"l_orderkey",  ORDERS_TUPLE_COUNT},
                                    {"l_partkey",  PART_TUPLE_COUNT},
                                    {"l_suppkey",     SUPPLIER_TUPLE_COUNT},
                                    {"l_linenumber",  LINEITEM_TUPLE_COUNT},
                                    {"l_quantity",  LINEITEM_TUPLE_COUNT},
                                    {"l_extendedprice", LINEITEM_TUPLE_COUNT},
                                    {"l_discount",   LINEITEM_TUPLE_COUNT},
                                    {"l_tax",          LINEITEM_TUPLE_COUNT},
                                    {"l_returnflag", 3},
                                    {"l_linestatus", 2},
                                    {"l_shipdate", LINEITEM_TUPLE_COUNT},
                                    {"l_commitdate", LINEITEM_TUPLE_COUNT},
                                    {"l_receiptdate", LINEITEM_TUPLE_COUNT},
                                    {"l_shipinstruct", LINEITEM_TUPLE_COUNT},
                                    {"l_shipmode", 7},
                                    {"l_comment", LINEITEM_TUPLE_COUNT}}}});

    for (auto &rel : attributes) {
        for (auto &attr : rel.second) {
            stat.AddAtt(rel.first, attr.first, attr.second);
        }
    }
}

SumTreeNode::~SumTreeNode() {
    delete from;
}

SelectPipeTreeNode::~SelectPipeTreeNode() {
    delete from;
}

JoinTreeNode::~JoinTreeNode() {
    delete left;
    delete right;
}

SelectFileTreeNode::~SelectFileTreeNode() {
    if (from) delete from;
}

GroupByTreeNode::~GroupByTreeNode() {
    delete from;
}


ProjectTreeNode::~ProjectTreeNode() {
    delete[] attrsToKeep;
    delete from;
}


DistinctTreeNode::~DistinctTreeNode() {
    delete from;
}