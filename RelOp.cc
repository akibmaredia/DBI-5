#include "RelOp.h"
#include "BigQ.h"

#include <vector>
#include <sstream>
#include <string>

void RelationalOp::WaitUntilDone() {
  	pthread_join (thread, NULL);
}

int RelationalOp::create_join_thread(pthread_t *thread, void *(*start_routine) (void *), void *arg) {
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	int temp = pthread_create(thread, &attr, start_routine, arg);
	pthread_attr_destroy(&attr);
	return temp;
}

SelectFile::~SelectFile(){
	this->dbFile = NULL;
	this->outPipe = NULL;
	this->selectOp = NULL;
	this->literal = NULL;
}

void SelectFile::Run (DBFile &inFile, Pipe &outPipe, CNF &selectOP, Record &literal) {
	
	this->dbFile = &inFile;
	this->outPipe = &outPipe;
	this->selectOp = &selectOP;
	this->literal = &literal;
	this->numberOfPg = DEFAULT_NUM_PAGES;

	create_join_thread(&thread, SelectFile::HelperRun, (void *) this);
}

void * SelectFile::HelperRun(void *param) {
	((SelectFile *) param)->Start();
}

void SelectFile::Start(){
	Record temporaryRecord;

	while(dbFile->GetNext(temporaryRecord, *selectOp, *literal)){
		outPipe->Insert(&temporaryRecord);
	}

	outPipe->ShutDown();
}

void SelectFile::WaitUntilDone (){
	RelationalOp::WaitUntilDone();
}

int SelectFile::Use_n_Pages (int n) {
	if(n<0)
		return -1;
	else
		this->numberOfPg = n;
}

SelectPipe::~SelectPipe(){
	this->inPipe = NULL;
	this->outPipe = NULL;
	this->selectOp = NULL;
	this->literal = NULL;
}

void SelectPipe::Run (Pipe &inPipe, Pipe &outPipe, CNF &selectOP, Record &literal){
	this->inPipe = &inPipe;
	this->outPipe = &outPipe;
	this->selectOp = &selectOP;
	this->literal = &literal;
	this->numberOfPg = DEFAULT_NUM_PAGES;

	create_join_thread(&thread, SelectPipe::HelperRun, (void *) this);
}

void * SelectPipe::HelperRun(void *param) {
	((SelectPipe *) param)->Start();
}

void SelectPipe::Start(){

	Record temporaryRecord;
	ComparisonEngine comparisonEngine;

	while(inPipe->Remove(&temporaryRecord)){
		if(comparisonEngine.Compare(&temporaryRecord, literal, selectOp)){
			outPipe->Insert(&temporaryRecord);
		}
	}

	outPipe->ShutDown();
}

void SelectPipe::WaitUntilDone (){
	RelationalOp::WaitUntilDone();
}
	
int SelectPipe::Use_n_Pages (int n){
	if(n<0)
		return -1;
	else
		this->numberOfPg = n;
}


Project::~Project(){

	this->inPipe = NULL;
	this->outPipe = NULL;
	this->keepMe = NULL;
}

void Project::Run (Pipe &inPipe, Pipe &outPipe, int *keepMe, int numAttsInput, int numAttsOutput){
	this->inPipe = &inPipe;
	this->outPipe = &outPipe;
	this->keepMe = keepMe;
	this->numAttsIn = numAttsInput;
	this->numAttsOut = numAttsOutput;
	this->numberOfPg = DEFAULT_NUM_PAGES;

	create_join_thread(&thread, Project::HelperRun, (void *) this);
}

void * Project::HelperRun(void *param){
	((Project *) param)->Start();
}

void Project::Start(){
	Record temporaryRecord;
	ComparisonEngine comparisonEnginep;

	while(inPipe->Remove(&temporaryRecord)){
		temporaryRecord.Project(keepMe, numAttsOut, numAttsIn);
		outPipe->Insert(&temporaryRecord);
	}

	outPipe->ShutDown();	
}

void Project::WaitUntilDone (){
	RelationalOp::WaitUntilDone();
}
	
int Project::Use_n_Pages (int n){
	if(n<0)
		return -1;
	else
		this->numberOfPg = n;
		return 1;
}


Join::~Join(){
	this->inPipeL = NULL;
	this->inPipeR = NULL;
	this->outPipe = NULL;
	this->selectOP = NULL;
	this->literal = NULL;
}

void Join::Run (Pipe &inPipeL, Pipe &inPipeR, Pipe &outPipe, CNF &selectOP, Record &literal){

	this->inPipeL = &inPipeL;
	this->inPipeR = &inPipeR;
	this->outPipe = &outPipe;
	this->selectOP = &selectOP;
	this->literal = &literal;
	this->numberOfPg = DEFAULT_NUM_PAGES;

	create_join_thread(&thread, Join::HelperRun, (void *) this);
}

void * Join::HelperRun(void *param){
	((Join *) param)->Start();
}

void Join::Start(){

	OrderMaker sortOrderL, sortOrderR;
	int isSortOrderExist = selectOP->GetSortOrders(sortOrderL, sortOrderR);

	if(isSortOrderExist){
		SortMergeJoin(sortOrderL, sortOrderR);
	}else{
		BlockNestedJoin();
	}
	
	outPipe->ShutDown();
}

void Join::SortMergeJoin(OrderMaker &sortOrderL, OrderMaker sortOrderR){

	Record temporaryRecordL, temporaryRecordR, tempMergeRec;
	ComparisonEngine comparisonEngine; 

	Pipe outPipeL(numberOfPg);
	Pipe outPipeR(numberOfPg);
	BigQ bigQL(*inPipeL, outPipeL, sortOrderL, numberOfPg);
	BigQ bigQR(*inPipeR, outPipeR, sortOrderR, numberOfPg);

	int isEmptyOutPipeL = 0, isEmptyOutPipeR = 0;

	if(!outPipeL.Remove(&temporaryRecordL)){
		isEmptyOutPipeL = 1;
	}
	if(!outPipeR.Remove(&temporaryRecordR)){
		isEmptyOutPipeR = 1;
	}

	const int numAttsL = temporaryRecordL.GetNumAttrs();
	const int numAttsR = temporaryRecordR.GetNumAttrs();
	const int totalAtts = numAttsL + numAttsR;
	int attsToKeep[totalAtts];

	int count = 0;
	for(int counter = 0; counter < numAttsL; counter++){
			attsToKeep[count++] = counter; 
	}
	for(int counter = 0; counter < numAttsR; counter++){
		attsToKeep[count++] = counter; 
	}

	vector<Record> lBuffer;
	vector<Record> rBuffer;
	lBuffer.reserve(1000);
	rBuffer.reserve(1000);

	while(!isEmptyOutPipeL && !isEmptyOutPipeR){
		if(comparisonEngine.Compare(&temporaryRecordL, &sortOrderL, &temporaryRecordR, &sortOrderR)<0){
			if(!outPipeL.Remove(&temporaryRecordL)) {
				isEmptyOutPipeL = 1;
			}
		}else if(comparisonEngine.Compare(&temporaryRecordL, &sortOrderL, &temporaryRecordR, &sortOrderR)>0) {
			if(!outPipeR.Remove(&temporaryRecordR)) {
				isEmptyOutPipeR = 1;
			}
		}else{ //records match as per sort order
			lBuffer.push_back(temporaryRecordL);
			rBuffer.push_back(temporaryRecordR);

			//fetch all records from left pipe which match current sort order
			Record recL;
			recL.Copy(&temporaryRecordL);
			if(outPipeL.Remove(&temporaryRecordL)){
				while(!comparisonEngine.Compare(&temporaryRecordL, &recL, &sortOrderL)){
					lBuffer.push_back(temporaryRecordL);
					if(!outPipeL.Remove(&temporaryRecordL)){
						isEmptyOutPipeL = 1;
						break;
					}
				}
			}else{
				isEmptyOutPipeL = 1;
			}

			Record recR;
			recR.Copy(&temporaryRecordR);
			if(outPipeR.Remove(&temporaryRecordR)){
				while(!comparisonEngine.Compare(&temporaryRecordR, &recR, &sortOrderR)){
					rBuffer.push_back(temporaryRecordR);
					if(!outPipeR.Remove(&temporaryRecordR)){
						isEmptyOutPipeR = 1;
						break;
					}
				}
			}else{
				isEmptyOutPipeR = 1;
			}

			int sizeL = lBuffer.size();
			int sizeR = rBuffer.size();
			for(int i = 0; i < sizeL; i++){
				for(int j = 0; j < sizeR; j++) {
					if(comparisonEngine.Compare(&lBuffer[i], &rBuffer[j], literal, selectOP)){
						tempMergeRec.MergeRecords(&lBuffer[i], &rBuffer[j], numAttsL, numAttsR, attsToKeep, totalAtts, numAttsL);
						outPipe->Insert(&tempMergeRec);
					}
				}
			}

			lBuffer.clear();
			rBuffer.clear();
		}
	}
}

void Join::BlockNestedJoin(){

	Record temporaryRecordL, temporaryRecordR, tempMergeRec;
	ComparisonEngine comparisonEngine; 

	if(!inPipeL->Remove(&temporaryRecordL)){
		return;
	}
	if(!inPipeR->Remove(&temporaryRecordR)){
		return;
	}

	const int numAttsL = temporaryRecordL.GetNumAttrs();
	const int numAttsR = temporaryRecordR.GetNumAttrs();
	const int totalAtts = numAttsL + numAttsR;
	int attsToKeep[totalAtts];

	int count = 0;
	for(int counter = 0; counter < numAttsL; counter++){
			attsToKeep[count++] = counter; 
	}
	for(int counter = 0; counter < numAttsR; counter++){
		attsToKeep[count++] = counter; 
	}

	vector<Record> lBuffer;
	do{
		lBuffer.push_back(temporaryRecordL);
	}while(inPipeL->Remove(&temporaryRecordL));

	int sizeL = lBuffer.size();

	do{
		for(int counter = 0; counter < sizeL; counter++){
			if(comparisonEngine.Compare(&lBuffer[counter], &temporaryRecordR, literal, selectOP)){
				tempMergeRec.MergeRecords(&lBuffer[counter], &temporaryRecordR, numAttsL, numAttsR, attsToKeep, totalAtts, numAttsL);
				outPipe->Insert(&tempMergeRec);
			}
		}
	}while(inPipeR->Remove(&temporaryRecordR));

	lBuffer.clear();
}

void Join::WaitUntilDone (){
	RelationalOp::WaitUntilDone();
}
	
int Join::Use_n_Pages (int n){
	if(n<0)
		return -1;
	else
		this->numberOfPg = n;
		return 1;
}

DuplicateRemoval::~DuplicateRemoval(){
	this->inPipe = NULL;
	this->outPipe = NULL;
	this->mySchema = NULL;
}

void DuplicateRemoval::Run(Pipe &inPipe, Pipe &outPipe, Schema &mySchema) {

	this->inPipe = &inPipe;
	this->outPipe = &outPipe;
	this->mySchema = &mySchema;
	this->numberOfPg = DEFAULT_NUM_PAGES;

	create_join_thread(&thread, DuplicateRemoval::HelperRun, (void *) this);
}

void * DuplicateRemoval::HelperRun(void *param){
	((DuplicateRemoval *) param)->Start();
}

void DuplicateRemoval::Start(){

	OrderMaker sortOrder(mySchema);
	Pipe tempOutPipe(numberOfPg);
	BigQ bigQ(*inPipe, tempOutPipe, sortOrder, numberOfPg);

	ComparisonEngine comparisonEngine;
	Record prevRec, currRec;
	if(tempOutPipe.Remove(&currRec)){
		prevRec.Copy(&currRec);
		outPipe->Insert(&currRec);

		while(tempOutPipe.Remove(&currRec)){
			if(comparisonEngine.Compare(&prevRec, &currRec, &sortOrder)){
				prevRec.Copy(&currRec);
				outPipe->Insert(&currRec);
			}
		}	
	}

	outPipe->ShutDown();	
}

void DuplicateRemoval::WaitUntilDone (){
	RelationalOp::WaitUntilDone();
}
	
int DuplicateRemoval::Use_n_Pages (int n){
	if(n<0)
		return -1;
	else
		this->numberOfPg = n;
		return 1;
}


Sum::~Sum(){

	this->inPipe = NULL;
	this->outPipe = NULL;
	this->computeMe = NULL;
}

void Sum::Run(Pipe &inPipe, Pipe &outPipe, Function &computeMe) {

	this->inPipe = &inPipe;
	this->outPipe = &outPipe;
	this->computeMe = &computeMe;
	this->numberOfPg = DEFAULT_NUM_PAGES;

	create_join_thread(&thread, Sum::HelperRun, (void *) this);
}

void * Sum::HelperRun(void *param){
	((Sum *) param)->Start();
}

void Sum::Start(){

	Record temporaryRecord;
	int tempIntResult, intResult = 0;
	double tempDoubleResult, doubleResult = 0.0;
	Type resType;

	while(inPipe->Remove(&temporaryRecord)){
		
		resType = computeMe->Apply(temporaryRecord, tempIntResult, tempDoubleResult);

		if(Int == resType){
			intResult += tempIntResult;
		}else{
			doubleResult += tempDoubleResult;
		}
	}

	//pack result in a record
    stringstream rec;
    Attribute attr;
    attr.name = "SUM";
    if (Int == resType){
        attr.myType = Int;
        rec << intResult;
    }else{
        attr.myType = Double;
       	rec << doubleResult;
    }
    rec << "|";
    
    Schema tempSchema("temp_schema", 1, &attr);
    temporaryRecord.ComposeRecord(&tempSchema, rec.str().c_str());
    outPipe->Insert(&temporaryRecord);

	outPipe->ShutDown();	
}

void Sum::WaitUntilDone (){
	RelationalOp::WaitUntilDone();
}
	
int Sum::Use_n_Pages (int n){
	if(n<0)
		return -1;
	else
		this->numberOfPg = n;
		return 1;
}


GroupBy::~GroupBy(){

	this->inPipe = NULL;
	this->outPipe = NULL;
	this->groupAtts = NULL;
	this->computeMe = NULL;
}

void GroupBy::Run(Pipe &inPipe, Pipe &outPipe, OrderMaker &groupAtts, Function &computeMe) {

	this->inPipe = &inPipe;
	this->outPipe = &outPipe;
	this->groupAtts = &groupAtts;
	this->computeMe = &computeMe;
	this->numberOfPg = DEFAULT_NUM_PAGES;

	create_join_thread(&thread, GroupBy::HelperRun, (void *) this);
}

void * GroupBy::HelperRun(void *param){
	//cout<<"In GroupBy::HelperRun"<<endl;
	((GroupBy *) param)->Start();
}

void GroupBy::Start(){


	//sort records from input pipe
	Pipe tempOutPipe(numberOfPg);
	BigQ bigQ(*inPipe, tempOutPipe, *groupAtts, numberOfPg);

	ComparisonEngine comparisonEngine;
	Record temporaryRecord1, temporaryRecord2, resRec;

	if(tempOutPipe.Remove(&temporaryRecord1)){

		int intResult = 0;
		double doubleResult = 0.0;
		Type resType;
		ApplyFunction(temporaryRecord1, intResult, doubleResult, resType);

		while(tempOutPipe.Remove(&temporaryRecord2)){
		
			if(!comparisonEngine.Compare(&temporaryRecord1, &temporaryRecord2, groupAtts)){
				ApplyFunction(temporaryRecord2, intResult, doubleResult, resType);
			}else{
				PackResultInRecord(resType, intResult, doubleResult, temporaryRecord1, resRec);
		
				outPipe->Insert(&resRec); //TODO-pipe getting full after inserting just 2 records, check why


				intResult = 0;
				doubleResult = 0.0;
				ApplyFunction(temporaryRecord2, intResult, doubleResult, resType);

				temporaryRecord1.Consume(&temporaryRecord2);
			}
		}

		PackResultInRecord(resType, intResult, doubleResult, temporaryRecord1, resRec);
		outPipe->Insert(&resRec);

	}

	outPipe->ShutDown();	
}

void GroupBy::ApplyFunction(Record &rec, int &intResult, double &doubleResult, Type &resType){

	int tempIntResult;
	double tempDoubleResult;

	resType = computeMe->Apply(rec, tempIntResult, tempDoubleResult);
	if(Int == resType){
		intResult += tempIntResult;
	}else{
		doubleResult += tempDoubleResult;
	}
}

void GroupBy::PackResultInRecord(Type resType, int &intResult, double &doubleResult, Record &groupRec, Record &resultRec){
	//create single attribute record with SUM as attribute
	stringstream recSS;
    Attribute attr;
    attr.name = "SUM";
    if (Int == resType){
        attr.myType = Int;
        recSS << intResult;
    }else{
        attr.myType = Double;
       	recSS << doubleResult;
    }
    recSS << "|";
    
    Record sumRec;
    Schema tempSchema("temp_schema", 1, &attr);
    sumRec.ComposeRecord(&tempSchema, recSS.str().c_str());


    const int numAtts = groupAtts->GetNumAttrs()+1; //+1 for the new SUM attribute
	int attsToKeep[numAtts];

	attsToKeep[0] = 0;
	for(int i=1; i<numAtts; i++){
		attsToKeep[i] = (groupAtts->GetWhichAttrs())[i-1]; 
	}

	//merge sum record and attributes from the current group by record into result record
	resultRec.MergeRecords(&sumRec, &groupRec, 1, groupRec.GetNumAttrs(), attsToKeep, numAtts, 1);
}

void GroupBy::WaitUntilDone (){
	RelationalOp::WaitUntilDone();
}
	
int GroupBy::Use_n_Pages (int n){
	if(n<0)
		return -1;
	else
		this->numberOfPg = n;
		return 1;
}


WriteOut::~WriteOut(){

	this->inPipe = NULL;
	this->outFile = NULL;
	this->mySchema = NULL;
}

void WriteOut::Run (Pipe &inPipe, FILE *outFile, Schema &mySchema) {

	this->inPipe = &inPipe;
	this->outFile = outFile;
	this->mySchema = &mySchema;
	this->numberOfPg = DEFAULT_NUM_PAGES;

	create_join_thread(&thread, WriteOut::HelperRun, (void *) this);
}

void * WriteOut::HelperRun(void *param){
	((WriteOut *) param)->Start();
}

void WriteOut::Start(){
	Record temporaryRecord;
	ostringstream os;

	while(inPipe->Remove(&temporaryRecord)){
      temporaryRecord.Print(mySchema, os);
	}

	fputs(os.str().c_str(), outFile);
}

void WriteOut::WaitUntilDone (){
	RelationalOp::WaitUntilDone();
}

int WriteOut::Use_n_Pages (int n) {
	if(n<0)
		return -1;
	else
		this->numberOfPg = n;
		return 1;
}

// #include "RelOp.h"
// #include <stdexcept>

// void *InitOperation(void *args) {
//     ((RelationalOp *) args)->Init();
// }

// int RelationalOp::CreateJoinableThread(pthread_t *thread, void *(*StartRoutine) (void *), void *args) {
//     pthread_attr_t attr;
//     pthread_attr_init(&attr);
//     pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

//     int res = pthread_create(thread, &attr, StartRoutine, args);
//     pthread_attr_destroy(&attr);
//     return res;
// }

// void RelationalOp::WaitUntilDone() {
//     pthread_join(thread, nullptr);
// }




// void SelectFile::Run(DBFile &inFile, Pipe &outPipe, CNF &selOp, Record &literal) {
//     dbFile = &inFile;
//     outputPipe = &outPipe;
//     cnf = &selOp;
//     record = &literal;

//     CreateJoinableThread(&thread, InitOperation, (void *) this);
// }

// void SelectFile::Init() {
//     Record next;
//     dbFile->MoveFirst();
//     while (dbFile->GetNext(next, *cnf, *record)) {
//         outputPipe->Insert(&next);
//     }
//     outputPipe->ShutDown();
// }

// void SelectPipe::Run(Pipe &inPipe, Pipe &outPipe, CNF &selOp, Record &literal) {
//     inputPipe = &inPipe;
//     outputPipe = &outPipe;
//     cnf = &selOp;
//     record = &literal;

//     CreateJoinableThread(&thread, InitOperation, (void *) this);
// }

// void SelectPipe::Init() {
//     Record tempRecord;
//     ComparisonEngine comparisonEngine;
//     while (inputPipe->Remove(&tempRecord)) {
//         if (comparisonEngine.Compare(&tempRecord, record, cnf)) {
//             outputPipe->Insert(&tempRecord);
//         }
//     }
//     outputPipe->ShutDown();
// }

// void Project::Run(Pipe &inPipe, Pipe &outPipe, int *keepMe, int numAttsInput, int numAttsOutput) {
//     inputPipe = &inPipe;
//     outputPipe = &outPipe;
//     outputAttributeCount = numAttsOutput;
//     keepAttributeCount = keepMe;
//     inputAttributeCount = numAttsInput;

//     CreateJoinableThread(&thread, InitOperation, (void *) this);
// }

// void Project::Init() {
//     Record tempRecord;
//     while (inputPipe->Remove(&tempRecord)) {
//         tempRecord.Project(keepAttributeCount, outputAttributeCount, inputAttributeCount);
//         outputPipe->Insert(&tempRecord);
//     }
//     outputPipe->ShutDown();
// }

// void Sum::Run(Pipe &inPipe, Pipe &outPipe, Function &computeMe) {
//     inputPipe = &inPipe;
//     outputPipe = &outPipe;
//     function = &computeMe;

//     CreateJoinableThread(&thread, InitOperation, (void *) this);
// }

// void Sum::Init() {
//     int intTotal = 0;
//     int intSumCounter = 0;
//     double doubleTotal = 0.0;
//     double doubleSumCounter = 0.0;

//     Type type;
//     Record *temp = new Record();
//     while (inputPipe->Remove(temp)) {
//         type = function->Apply(*temp, intSumCounter, doubleSumCounter);
//         if(type == Int) {
//             intTotal += intSumCounter;
//         } else {
//             doubleTotal += doubleSumCounter;
//         }
//     }

//     delete temp;

//     Record *result = new Record();
//     std::stringstream strStream;
//     if (type == Int) {
//         strStream << intTotal;
//     } else {
//         strStream << doubleTotal;
//     }
//     strStream << '|';

//     Attribute attr = {"sum", Double};
//     Schema out_schema("out_sch", 1, &attr);
//     result->ComposeRecord(&out_schema, strStream.str().c_str());

//     outputPipe->Insert(result);
//     outputPipe->ShutDown();

//     delete result;
// }

// void Join::Run(Pipe &inPipeL, Pipe &inPipeR, Pipe &outPipe, CNF &selOp, Record &literal) {
//     inputPipeLeft = &inPipeL;
//     inputPipeRight = &inPipeR;
//     outputPipe = &outPipe;
//     cnf = &selOp;
//     record = &literal;

//     CreateJoinableThread(&thread, InitOperation, (void *) this);
// }

// void Join::Init() {
//     OrderMaker orderLeft, orderRight;
//     int sortOrderExists = cnf->GetSortOrders(orderLeft, orderRight);
//     if (sortOrderExists) {
//         SortMergeJoin(&orderLeft, &orderRight);
//     } else {
//         NestedLoopJoin();
//     }
//     outputPipe->ShutDown();
// }

// void Join::SortMergeJoin(OrderMaker *orderLeft, OrderMaker *orderRight) {
//     Record tempRecLeft, tempRecRight, tempMergedRec;
//     ComparisonEngine comparisonEngine;

//     Pipe pipeLeft(DEFAULT_NUM_PAGES);
//     Pipe pipeRight(DEFAULT_NUM_PAGES);

//     BigQ bigQLeft(*inputPipeLeft, pipeLeft, *orderLeft, DEFAULT_NUM_PAGES);
//     BigQ bigQRight(*inputPipeRight, pipeRight, *orderRight, DEFAULT_NUM_PAGES);

//     bool isLeftPipeEmpty = false;
//     if(!pipeLeft.Remove(&tempRecLeft)) {
//         isLeftPipeEmpty = true;
//     }

//     bool isRightPipeEmpty = false;
//     if(!pipeRight.Remove(&tempRecRight)) {
//         isRightPipeEmpty = true;
//     }

//     const int leftAttributeCount = tempRecLeft.GetNumAttrs();
//     const int rightAttributeCount = tempRecRight.GetNumAttrs();
//     const int totalAttributeCount = leftAttributeCount + rightAttributeCount;

//     int attrsToKeep[totalAttributeCount];
//     for (int i = 0; i < leftAttributeCount; i++) {
//         attrsToKeep[i] = i;
//     }
//     for (int i = 0; i < rightAttributeCount; i++) {
//         attrsToKeep[leftAttributeCount + i] = i;
//     }

//     vector<Record> leftRecords;
//     leftRecords.reserve(1000);

//     vector<Record> rightRecords;
//     rightRecords.reserve(1000);

//     while (!isLeftPipeEmpty && !isRightPipeEmpty) {
//         int result = comparisonEngine.Compare(&tempRecLeft, orderLeft, &tempRecRight, orderRight);
//         if (result < 0) {
//             if (!pipeLeft.Remove(&tempRecLeft)) isLeftPipeEmpty = true;
//         }
//         else if (result > 0) {
//             if (!pipeRight.Remove(&tempRecRight)) isRightPipeEmpty = true;
//         }
            
//         else {
//             leftRecords.push_back(tempRecLeft);
//             rightRecords.push_back(tempRecRight);

//             /*
//              * Fetch all the records matching the sort order form left and right pipe and add it into their respective vectors.
//              */
//             Record recLeft;
//             recLeft.Copy(&tempRecLeft);
//             if (pipeLeft.Remove(&tempRecLeft)) {
//                 while (!comparisonEngine.Compare(&tempRecLeft, &recLeft, orderLeft)) {
//                     leftRecords.push_back(tempRecLeft);
//                     if (!pipeLeft.Remove(&tempRecLeft)) {
//                         isLeftPipeEmpty = true;
//                         break;
//                     }
//                 }
//             }
//             else {
//                 isLeftPipeEmpty = true;
//             }

//             Record recRight;
//             recRight.Copy(&tempRecRight);
//             if (pipeRight.Remove(&tempRecRight)) {
//                 while (!comparisonEngine.Compare(&tempRecRight, &recRight, orderRight)) {
//                     rightRecords.push_back(tempRecLeft);
//                     if (!pipeRight.Remove(&tempRecRight)) {
//                         isRightPipeEmpty = true;
//                         break;
//                     }
//                 }
//             }
//             else {
//                 isRightPipeEmpty = true;
//             }

//             for (int i = 0; i < leftRecords.size(); i++) {
//                 for (int j = 0; j < rightRecords.size(); j++) {
//                     if (comparisonEngine.Compare(&leftRecords[i], &rightRecords[j], record, cnf)) {
//                         tempMergedRec.MergeRecords(&leftRecords[i], &rightRecords[j], leftAttributeCount, rightAttributeCount,
//                                                    attrsToKeep, totalAttributeCount, leftAttributeCount);
//                         outputPipe->Insert(&tempMergedRec);
//                     }
//                 }
//             }

//             leftRecords.clear();
//             rightRecords.clear();
//         }
//     }
// }

// void Join::NestedLoopJoin() {
//     Record tempRecLeft, tempRecRight, tempMergedRec;
//     ComparisonEngine comparisonEngine;

//     if(!inputPipeLeft->Remove(&tempRecLeft)) return;
//     if(!inputPipeRight->Remove(&tempRecRight)) return;

//     const int leftAttributeCount = tempRecLeft.GetNumAttrs();
//     const int rightAttributeCount = tempRecRight.GetNumAttrs();
//     const int totalAttributeCount = leftAttributeCount + rightAttributeCount;

//     int attrsToKeep[totalAttributeCount];
//     for (int i = 0; i < leftAttributeCount; i++) {
//         attrsToKeep[i] = i;
//     }
//     for (int i = 0; i < rightAttributeCount; i++) {
//         attrsToKeep[leftAttributeCount + i] = i;
//     }

//     vector<Record> leftRecords;
//     do {
//         leftRecords.push_back(tempRecLeft);
//     } while (inputPipeLeft->Remove(&tempRecLeft));

//     do {
//         for (int i = 0; i < leftRecords.size(); i++) {
//             if (comparisonEngine.Compare(&leftRecords[i], &tempRecRight, record, cnf)) {
//                 tempMergedRec.MergeRecords(&leftRecords[i], &tempRecRight, leftAttributeCount, rightAttributeCount,
//                                            attrsToKeep, totalAttributeCount, leftAttributeCount);
//                 outputPipe->Insert(&tempMergedRec);
//             }
//         }
//     } while (inputPipeRight->Remove(&tempRecRight));

//     leftRecords.clear();
// }

// void DuplicateRemoval::Run(Pipe &inPipe, Pipe &outPipe, Schema &mySchema) {
//     inputPipe = &inPipe;
//     outputPipe = &outPipe;
//     schema = &mySchema;

//     CreateJoinableThread(&thread, InitOperation, (void *) this);
// }

// void DuplicateRemoval::Init() {
//     OrderMaker sortOrder(schema);

//     Pipe pipe(DEFAULT_NUM_PAGES);

//     BigQ bigQ(*inputPipe, pipe, sortOrder, DEFAULT_NUM_PAGES);

//     Record previous, current;
//     ComparisonEngine comparisonEngine;

//     if (pipe.Remove(&current)) {
//         previous.Copy(&current);
//         outputPipe->Insert(&current);
//         while (pipe.Remove(&current)) {
//             if (comparisonEngine.Compare(&previous, &current, &sortOrder)) {
//                 previous.Copy(&current);
//                 outputPipe->Insert(&current);
//             }
//         }
//     }

//     outputPipe->ShutDown();
// }

// void GroupBy::Run(Pipe &inPipe, Pipe &outPipe, OrderMaker &groupAtts, Function &computeMe) {
//     inputPipe = &inPipe;
//     outputPipe = &outPipe;
//     orderMaker = &groupAtts;
//     function = &computeMe;

//     CreateJoinableThread(&thread, InitOperation, (void *) this);
// }

// void GroupBy::Init() {
//     Pipe pipe(DEFAULT_NUM_PAGES);
//     BigQ bigQ(*inputPipe, pipe, *orderMaker, DEFAULT_NUM_PAGES);

//     Record previous, current, result;
//     ComparisonEngine comparisonEngine;

//     if (pipe.Remove(&previous)) {
//         int intAttributeCount = 0;
//         double doubleAttributeCount = 0.0;
//         Type type;

//         ApplyFunction(previous, intAttributeCount, doubleAttributeCount, type);

//         while (pipe.Remove(&current)) {
//             if (!comparisonEngine.Compare(&previous, &current, orderMaker)) {
//                 ApplyFunction(current, intAttributeCount, doubleAttributeCount, type);
//             }
//             else {
//                 WrapResultRecord(type, intAttributeCount, doubleAttributeCount, previous, result);
//                 outputPipe->Insert(&result);

//                 intAttributeCount = 0;
//                 doubleAttributeCount = 0.0;
//                 ApplyFunction(current, intAttributeCount, doubleAttributeCount, type);

//                 previous.Consume(&current);
//             }
//         }

//         WrapResultRecord(type, intAttributeCount, doubleAttributeCount, previous, result);
//         outputPipe->Insert(&result);
//     }

//     outputPipe->ShutDown();
// }

// void GroupBy::ApplyFunction(Record &record, int &intAttrCount, double &doubleAttrCount, Type &type) {
//     int tempIntAttributeCount;
//     double tempDoubleAttributeCount;
//     type = function->Apply(record, tempIntAttributeCount, tempDoubleAttributeCount);
//     if(type == Int) {
//         intAttributeCount += tempIntAttributeCount;
//     }else{
//         doubleAttributeCount += tempDoubleAttributeCount;
//     }
// }

// void GroupBy::WrapResultRecord(Type type, int &intAttrCount, double &doubleAttrCount, Record &record, Record &result) {
//     Attribute attr;
//     attr.name = "sum";
//     attr.myType = Int;

//     std::stringstream ss;
//     if (type == Int) {
//         ss << intAttributeCount;
//     } else {
//         ss << doubleAttributeCount;
//     }
//     ss << "|";

//     Record sum;
//     Schema out("temp_schema", 1, &attr);
//     sum.ComposeRecord(&out, ss.str().c_str());

//     const int numAttrs = orderMaker->GetNumAttrs() + 1;
//     int attrsToKeep[numAttrs];
//     attrsToKeep[0] = 0;
//     for (int i = 1; i < numAttrs; i++) {
//         attrsToKeep[i] = (orderMaker->GetWhichAttrs())[i-1];
//     }

//     result.MergeRecords(&sum, &record, 1, record.GetNumAttrs(), attrsToKeep, numAttrs, 1);
// }

// void WriteOut::Run(Pipe &inPipe, FILE *outFile, Schema &mySchema) {
//     inputPipe = &inPipe;
//     file = outFile;
//     schema = &mySchema;

//     CreateJoinableThread(&thread, InitOperation, (void *) this);
// }

// void WriteOut::Init() {
//     Record rec;
//     std::ostringstream os;
//     while(inputPipe->Remove(&rec)){
//         rec.Print(schema, os);
//     }
//     fputs(os.str().c_str(), file);
// }
// bool RelationalOp::Use_n_Pages(int n) {
//     if (n < 0) {
//         return false;
//     }
//     runLength = n;
//     return true;
// }