#include <algorithm>
#include <climits>
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include "MainScheme.h"
#include "Statistics.h"

using namespace std;

static int bufferPipeid = 0;

int getbufferPipeid() { return ++bufferPipeid; }

vector<OrList*> join_pre;
vector<OrList*> lit_pre;


bool whether_lit(OrList &orl) {
    return orl.left->right->code != NAME;
}







vector<string> getRel(struct ComparisonOp *comp) {
    vector<string> str_rel;

    //relations

    if (comp->left && comp->left->code == NAME) {
        string attName(comp->left->value);
        string ret_val = attName.substr(0, attName.find("."));
        str_rel.push_back(ret_val);
    }
    if (comp->right && comp->right->code == NAME) {
        string attName(comp->right->value);
        string ret_val = attName.substr(0, attName.find("."));
        str_rel.push_back(ret_val);
    }
    return str_rel;
}


void initialize_predicate() {
    AndList *temp_stg_andL = boolean;
    while (temp_stg_andL) {
        OrList *temp_stg_orL = temp_stg_andL->left;
        if (whether_lit(*temp_stg_orL)) {
            lit_pre.push_back(temp_stg_orL);
        } else {
            join_pre.push_back(temp_stg_orL);
        }

        //assign back
        temp_stg_andL = temp_stg_andL->rightAnd;
    }
}


void initialize_aliasMap(TableList *tableList, Statistics &stat,
                               vector<char *> &tableNames, AliasMap &aliasNameMap) {

    //we initialize table names and map alises
    while (tableList) {
        stat.CopyRel(tableList->tableName, tableList->aliasAs);
        aliasNameMap[tableList->aliasAs] = tableList->tableName;
        tableNames.push_back(tableList->aliasAs);
        tableList = tableList->next;
    }
}

void MapNameList(NameList *nameList, vector <string> &names) {
    //error to be checked.
    while (nameList) {
        names.push_back(string(nameList->name));
        nameList = nameList->next;
    }
}

string getCNF(struct ComparisonOp *comp_op) {
    string stg_cnf;

    if (comp_op->left) {
        stg_cnf.append(comp_op->left->value);
    }

    if (comp_op->code == LESS_THAN) stg_cnf.append(" < ");
    else if(comp_op->code == GREATER_THAN) stg_cnf.append(" > ");
    else stg_cnf.append(" = ");

    if (comp_op->right) {
        stg_cnf.append(comp_op->right->value);
    }

    return stg_cnf;
}

void OutputNameList(NameList *nameList) {
    while (nameList) {
        cout << nameList->name << endl;
        nameList = nameList->next;
    }
}

int main(int argc, char *argv[]) {
    cout << "enter: " << endl;

    if (yyparse() != 0) {
        cout << "parsing CNF. error" << endl;
        exit(1);
    }



    initialize_predicate();

    MainScheme mainScheme;

    SchemeStore schemeStore;
    mainScheme.initializeSchemeStore(schemeStore);

    Statistics statistics;
    mainScheme.initializeStatistics(statistics);

    vector<char *> tableNames;
    AliasMap aliasNameMap;
    initialize_aliasMap(tables, statistics, tableNames, aliasNameMap);
    sort(tableNames.begin(), tableNames.end());

    vector<char *> joinOrder;
    vector<char *> buffer(2);

 


    int cost = 0, minCost = INT_MAX;

    do {
        Statistics tempStat(statistics);

        auto iter_count = tableNames.begin();
        buffer[0] = *iter_count;
        iter_count++;

        while (iter_count != tableNames.end()) {
            buffer[1] = *iter_count;

            cost += tempStat.Estimate(boolean, &buffer[0], 2);
            tempStat.Apply(boolean, &buffer[0], 2);

            if (cost <= 0 || cost > minCost) {
                break;
            }

            iter_count++;
        }

        if (cost >= 0 && cost < minCost) {
            minCost = cost;
            joinOrder = tableNames;
        }

        cost = 0;
    } while (next_permutation(tableNames.begin(), tableNames.end()));

    map<string, Node> relNodeMap;

    Node *root, *endNode;

    SelectPipeTreeNode *sel_pipe = new SelectPipeTreeNode();

    for (auto& pred : lit_pre) {
        vector<string> str_rel = getRel(pred->left);

        SelectFileTreeNode *sel_file = new SelectFileTreeNode();
        sel_file->pipeId = getbufferPipeid();
        sel_file->out = Schema(schemeStore[aliasNameMap[str_rel[0]]]);
        sel_file->out.Reset(str_rel[0]);
        sel_file->cnf.GrowFromParseTree(NULL, &(sel_file->out), sel_file->record);

        sel_pipe->pipeId = getbufferPipeid();
        sel_pipe->out = Schema(schemeStore[aliasNameMap[str_rel[0]]]);
        sel_pipe->out.Reset(str_rel[0]);
        sel_pipe->cnf.GrowFromParseTree(boolean, &(sel_pipe->out), sel_pipe->record);
        sel_pipe->from = sel_file;
        sel_pipe->cnfStr = getCNF(pred->left);

        relNodeMap.insert(pair<string,Node> (str_rel[0], *sel_pipe));

        endNode = sel_pipe;
    }

    int joinCount = join_pre.size();

    if (joinCount > 0) {
        // Perform select on all the remaining tables.
        for (auto& pred : join_pre) {
            vector<string> str_rel = getRel(pred->left);

            for (string rel : str_rel) {
                if (relNodeMap.find(rel) == relNodeMap.end()) {
                    SelectFileTreeNode *sel_file = new SelectFileTreeNode();
                    sel_file->pipeId = getbufferPipeid();
                    sel_file->out = Schema(schemeStore[aliasNameMap[rel]]);
                    sel_file->out.Reset(rel);
                    sel_file->cnf.GrowFromParseTree(NULL, &(sel_file->out), sel_file->record);
                    sel_file->from = endNode;

                    relNodeMap.insert(pair<string,Node> (rel, *sel_file));

                    endNode = sel_file;
                }
            }
        }

        // Join op
        OrList *joinOp = join_pre[0];

        vector<string> rel_join_fir = getRel(joinOp->left);

        map<string, Node>::iterator it = relNodeMap.find(rel_join_fir[0]);
        Node *leftNode = &(it->second);

        it = relNodeMap.find(rel_join_fir[1]);
        Node *rightNode = &(it->second);



        JoinTreeNode *joinFunction = new JoinTreeNode();
        joinFunction->pipeId = getbufferPipeid();
        joinFunction->left = leftNode;
        joinFunction->right = rightNode;
        joinFunction->out.JoinSchema(joinFunction->left->out, joinFunction->right->out);
        joinFunction->cnf.GrowFromParseTree(boolean, &(joinFunction->left->out),
                                        &(joinFunction->right->out), joinFunction->record);
        joinFunction->cnfStr = getCNF(joinOp->left);
        joinFunction->from = endNode;

        endNode = joinFunction;

        if (joinCount > 1) {
            joinOp = join_pre[1];
            vector<string> secondJoinRel = getRel(joinOp->left);
     

            if (rel_join_fir[0] == secondJoinRel[0] || rel_join_fir[1] == secondJoinRel[0]) {
                leftNode = endNode;

                it = relNodeMap.find(secondJoinRel[1]);
                rightNode = &(it->second);
            }
            else {
                it = relNodeMap.find(secondJoinRel[0]);
                leftNode = &(it->second);

                rightNode = endNode;
            }

            joinFunction = new JoinTreeNode();
            joinFunction->pipeId = getbufferPipeid();
            joinFunction->left = leftNode;
            joinFunction->right = rightNode;
            joinFunction->out.JoinSchema(joinFunction->left->out, joinFunction->right->out);
            joinFunction->cnf.GrowFromParseTree(boolean, &(joinFunction->left->out),
                                            &(joinFunction->right->out), joinFunction->record);
            joinFunction->cnfStr = getCNF(joinOp->left);
            joinFunction->from = endNode;

            endNode = joinFunction;
        }
    }

    if (groupingAtts) {
        if (distinctFunc) {
            DistinctTreeNode *dNode = new DistinctTreeNode();
            dNode->pipeId = getbufferPipeid();
            dNode->out = endNode->out;
            dNode->from = endNode;

            endNode = dNode;
        }

     

        GroupByTreeNode *gbNode = new GroupByTreeNode();
        gbNode->pipeId = getbufferPipeid();
        gbNode->func.GrowFromParseTree(finalFunction, endNode->out);
        gbNode->out.GroupBySchema(endNode->out, gbNode->func.ReturnInt());
        gbNode->group.GrowFromParseTree(groupingAtts, &(gbNode->out));
        gbNode->from = endNode;

        endNode = gbNode;
    }


    if (finalFunction) {
        SumTreeNode *sNode = new SumTreeNode();
        sNode->pipeId = getbufferPipeid();
        sNode->func.GrowFromParseTree(finalFunction, endNode->out);

        Attribute si_attributes[2][1] = {{{"sum", Int}},
                                 {{"sum", Double}}};
        sNode->out = Schema(NULL, 1, sNode->func.ReturnInt() ? si_attributes[0] : si_attributes[1]);
        sNode->from = endNode;

        endNode = sNode;
    }

    if (attsToSelect) {
        vector<int> attrsToKeep;
        vector<string> si_attributes;
        MapNameList(attsToSelect, si_attributes);

        ProjectTreeNode *projectNode = new ProjectTreeNode();
        projectNode->pipeId = getbufferPipeid();
        projectNode->attrsToKeep = &attrsToKeep[0];
        projectNode->inNum = endNode->out.GetNumAtts();
        projectNode->outNum = si_attributes.size();
        projectNode->from = endNode;

        endNode = projectNode;
    }



    int selectCount = 0;
    while (attsToSelect) {
        selectCount++;
        attsToSelect = attsToSelect->next;
    }
    cout << endl << "Number of selects: " << selectCount << endl;

    cout << "Number of joins: " << joinCount << endl << endl;

    if (groupingAtts) {
        cout << "GROUPING ON: ";
        OutputNameList(groupingAtts);
        cout << endl;
    }




    cout << "PRINTING TREE IN ORDER:" << endl;
    root = endNode;
    root->Print();

    return 0;
}




