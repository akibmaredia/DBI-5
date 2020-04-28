#include "Statistics.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iterator>
#include <vector>

Statistics::Statistics() {}

// Performs deep copy.
Statistics::Statistics(Statistics &copyMe) {
    for (auto& rel : copyMe.relation_info_map) {
        // Create new relation info instance and add it to the map.
        relation_info relationInfo;
        relationInfo.is_join = rel.second.is_join;
        relationInfo.tuple_count = rel.second.tuple_count;

        for (auto& attr : rel.second.attribute_info_map) {
            string attrName = attr.first;
            int distinct_tuple_count = attr.second;
            relationInfo.attribute_info_map[attrName] = distinct_tuple_count;
        }

        string relName = rel.first;
        relation_info_map[relName] = relationInfo;
    }
}

Statistics::~Statistics() {
    relation_info_map.clear();
}

int Statistics::GetRelationInfoSize() {
    return relation_info_map.size();
}

void Statistics::AddRel(char *relName, int numTuples) {
    relation_info relationInfo;
    relationInfo.is_join = false;
    relationInfo.tuple_count = numTuples;

    // Add new relation.
    relation_info_map[relName] = relationInfo;
}

void Statistics::AddAtt(char *relName, char *attName, int numDistincts) {
    relation_info relationInfo = relation_info_map[string(relName)];

    // Set distinct count to total number of tuples for that relation if given as "-1".
    if (numDistincts == -1) {
        numDistincts = relationInfo.tuple_count;
    }

    // Add new attribute to the relation.
    relationInfo.attribute_info_map[string(attName)] = numDistincts;

    // Update relation info map.
    relation_info_map[string(relName)] = relationInfo;
}

void Statistics::CopyRel(char *oldName, char *newName) {
    // Check if relation with oldName exists in the map.
    if (relation_info_map.find(string(oldName)) != relation_info_map.end()) {
        relation_info oldRel = relation_info_map[oldName];

        relation_info newRel;
        newRel.is_join = oldRel.is_join;
        newRel.tuple_count = oldRel.tuple_count;
        for (auto& attr : oldRel.attribute_info_map) {
            string attrName(newName);
            attrName.append(".").append(attr.first);
            int distinct_tuple_count = attr.second;
            newRel.attribute_info_map[attrName] = distinct_tuple_count;
        }

        // Add new relation to the map.
        relation_info_map[string(newName)] = newRel;
    }
}

void Statistics::Read(char *fromWhere) {
    relation_info_map.clear();

    ifstream file_obj(fromWhere);

    if (file_obj.is_open()) {
        string line, relName;
        while (getline(file_obj, line)) {
            if (line.empty()) continue;

            istringstream buf(line);
            istream_iterator<string> beg(buf), end;
            vector<string> tokens(beg, end);

            if (tokens[0] == "Relation") {
                // Parse relation info - example: "Relation supplier 10000"
                relation_info relationInfo;
                relName = string(tokens[1]);
                relationInfo.tuple_count = stod(tokens[2]);
                relation_info_map[relName] = relationInfo;
            }
            else if (tokens[0] == "Attribute") {
                // Parse attribute info - example: "Attribute s_suppkey 5000"
                relation_info relationInfo = relation_info_map[relName];
                relationInfo.attribute_info_map[tokens[1]] = stod(tokens[2]);
                relation_info_map[relName] = relationInfo;
            }
        }
    }

    file_obj.close();
}

void Statistics::Write(char *fromWhere) {
    ofstream file_obj(fromWhere);

    for (auto& rel : relation_info_map) {
        file_obj << "Relation " << rel.first << " " << rel.second.tuple_count << "\n";
        for (auto& attr : rel.second.attribute_info_map) {
            file_obj << "Attribute " << attr.first << " " << attr.second << "\n";
        }
        file_obj << "\n";
    }

    file_obj.close();
}

double Statistics::AndOp(AndList *andList, char **relNames, int numToJoin) {
    if (andList == nullptr)
        return 1.0;

    double left = 1.0;
    left = OrOp(andList->left, relNames, numToJoin);

    double right = 1.0;
    right = AndOp(andList->rightAnd, relNames, numToJoin);

    return left * right;
}

double Statistics::OrOp(OrList *orList, char **relNames, int numToJoin) {
    if (orList == nullptr)
        return 0.0;

    double left = 0.0;
    left = CompOp(orList->left, relNames, numToJoin);

    int count = 1;

    OrList *temp = orList->rightOr;
    char *attr = orList->left->left->value;

    while (temp) {
        if (strcmp(temp->left->left->value, attr) == 0)
            count++;
        temp = temp->rightOr;
    }

    if (count > 1)
        return (double) count * left;

    double right = 0.0;
    right = OrOp(orList->rightOr, relNames, numToJoin);

    return (double) (1.0 - (1.0 - left) * (1.0 - right));
}

double Statistics::CompOp(ComparisonOp *comparisonOp, char **relNames, int numToJoin) {
    unordered_map<string, relation_info>::iterator relations[2];

    int leftResult = GetRelationForOp(comparisonOp->left, relNames, relations[0]);
    int rightResult = GetRelationForOp(comparisonOp->right, relNames, relations[1]);

    int code = comparisonOp->code;

    double left = 0.0;
    if (comparisonOp->left->code == NAME) {
        if (leftResult == -1) left = 1.0;
        else {
            string tempStr(comparisonOp->left->value);
            left = relations[0]->second.attribute_info_map[tempStr];
        }
    } else {
        left = -1.0;
    }

    double right = 0.0;
    if (comparisonOp->right->code == NAME) {
        if (rightResult == -1) right = 1.0;
        else {
            string tempStr(comparisonOp->right->value);
            right = relations[1]->second.attribute_info_map[tempStr];
        }
    } else {
        right = -1.0;
    }

    if (code == LESS_THAN || code == GREATER_THAN) {
        return 1.0 / 3.0;
    } else if (code == EQUALS) {
        if (left > right) {
            return 1.0 / left;
        } else {
            return 1.0 / right;
        }
    }

    return 0.0;
}

int Statistics::GetRelationForOp(Operand *op, char **relNames, unordered_map<string, relation_info>::iterator &relation) {
    if (op == nullptr || relNames == nullptr)
        return -1;

    string tempAttr(op->value);

    for (unordered_map<string, relation_info>::iterator relItr = relation_info_map.begin(); relItr != relation_info_map.end(); relItr++) {
        if (relItr->second.attribute_info_map.find(tempAttr) != relItr->second.attribute_info_map.end()) {
            relation = relItr;
            return 0;
        }
    }

    return -1;
}

void  Statistics::Apply(struct AndList *parseTree, char *relNames[], int numToJoin) {
    int i = 0, numJoined = 0;
    char *joinRelNames[100];

    relation_info rel;

    while (i < numToJoin) {
        string tempStr(relNames[i]);

        auto it = relation_info_map.find(tempStr);
        if (it != relation_info_map.end()) {
            rel = it->second;
            joinRelNames[numJoined++] = relNames[i];

            if (rel.is_join) {
                int size = rel.join_rel_map.size();
                if (size <= numToJoin) {
                    for (int j = 0; j < numToJoin; j++) {
                        string tempStr2(relNames[j]);
                        if (rel.join_rel_map.find(tempStr2) == rel.join_rel_map.end() &&
                            rel.join_rel_map[tempStr2] != rel.join_rel_map[tempStr]) {
                            cout << "Can not perform join!" << endl;
                            return;
                        }
                    }
                }
            }
            else {
                i++;
                continue;
            }
        }

        i++;
    }

    double estimate = Estimate(parseTree, joinRelNames, numJoined);

    string newRelName(joinRelNames[0]);

    relation_info newRel = relation_info_map[newRelName];
    newRel.is_join = true;
    newRel.tuple_count = estimate;

    relation_info_map.erase(newRelName);

    relation_info temp;

    i = 1;
    while (i < numJoined) {
        string tempStr(joinRelNames[i]);
        newRel.join_rel_map[tempStr] = tempStr;
        temp = relation_info_map[tempStr];
        relation_info_map.erase(tempStr);
        newRel.attribute_info_map.insert(temp.attribute_info_map.begin(), temp.attribute_info_map.end());
        i++;
    }

    relation_info_map.insert(pair<string, relation_info> (newRelName, newRel));

    /*double estimation = Estimate (parseTree, names, numJoin);

    index = 1;
    string firstRelName (names[0]);
    RelationInfo firstRel = relMap[firstRelName];
    RelationInfo temp;

    relMap.erase (firstRelName);
    firstRel.isJoint = true;
    firstRel.numTuples = estimation;

    while (index < numJoin) {
        string buffer (names[index]);

        firstRel.relJoint[buffer] = buffer;
        temp = relMap[buffer];
        relMap.erase (buffer);

        firstRel.attrMap.insert (temp.attrMap.begin (), temp.attrMap.end ());
        index++;
    }

    relMap.insert (pair<string, RelationInfo> (firstRelName, firstRel));*/
}

double Statistics::Estimate(struct AndList *parseTree, char **relNames, int numToJoin) {
    double product = 1.0;

    int i = 0;
    while (i < numToJoin) {
        string temp(relNames[i]);
        if (relation_info_map.find(temp) != relation_info_map.end()) {
            product *= relation_info_map[temp].tuple_count;
        }
        i++;
    }

    if (parseTree == nullptr)
        return product;

    double factor = 1.0;
    factor = AndOp(parseTree, relNames, numToJoin);

    return factor * product;
}

void Statistics::Print() {
    cout << "-------------------------------------------------------------------------------" << endl;
    cout << "Relation Map" << endl;
    for (auto& rel : relation_info_map) {
        cout << rel.first;

        cout << "\t";
        cout << rel.second.tuple_count;

        cout << "\t";
        cout << "[";
        for (auto& attr : rel.second.attribute_info_map) {
            cout << attr.first << " - " << attr.second << ", ";
        }
        cout << "]";

        cout << endl;
    }
    cout << endl;
}

bool Statistics::Exists(string relation) {
    return relation_info_map.find(relation) != relation_info_map.end();
}
