#ifndef STATISTICS_
#define STATISTICS_

#include "ParseTree.h"

#include <cstring>
#include <string>
#include <unordered_map>

using namespace std;

typedef struct RelationInfo {
    // Number of tuples this relation has.
    double tuple_count;

    // If the relation is joined.
    bool is_join;

    unordered_map<string, string> join_rel_map;

    // Map of attribute-name <-> distinct-tuple-count
    unordered_map<string, int> attribute_info_map;
} relation_info;

class Statistics {
private:
    // Map of relation-name <-> relation-info
    unordered_map<string, relation_info> relation_info_map;

    double AndOp(AndList *andList, char **relNames, int numToJoin);
    double OrOp(OrList *orList, char **relNames, int numToJoin);
    double CompOp(ComparisonOp *comparisonOp, char **relNames, int numToJoin);

    int GetRelationForOp(Operand *op, char **relNames, unordered_map<string, relation_info>::iterator &relation);

public:
    Statistics();

    // Performs deep copy
    Statistics(Statistics &copyMe);

    ~Statistics();

    void AddRel(char *relName, int numTuples);

    void AddAtt(char *relName, char *attName, int numDistincts);

    void CopyRel(char *oldName, char *newName);

    void Read(char *fromWhere);

    void Write(char *fromWhere);

    void  Apply(struct AndList *parseTree, char *relNames[], int numToJoin);

    double Estimate(struct AndList *parseTree, char **relNames, int numToJoin);

    int GetRelationInfoSize();

    void Print();
    bool Exists(string relation);
};

#endif