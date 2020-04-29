#include "ParseTree.h"
#include <string>


using namespace std;

int main() {
    int op;
    int state = 0;
    Sql db;

    while(true) {
        cout << endl << "Please select your operation:" << endl << endl;
        cout << "1. Create a new database (will delete current database)" << endl;
        cout << "2. Start current database" << endl;
        cout << "3. Execute SQL" << endl;
        cout << "4. Shut down database" << endl;
        cout << "5. Exit" << endl << endl;

        cout << "Enter option";
        cin >> op;

        switch (op)
        {
        case 1:
            if(state != 0) cerr << "Warning: Shut down the current database" << endl;
            else {
                db.Create();
            }
            break;

        case 2:
            if(state != 1) cerr << "Warning: Databse has not been started yet" << endl;
            else {
                db.Open();
                state = 1;
            }
            break;

        case 3:
            if (state != 1) cerr << "Warning: Database hasn't been started yet!" << endl;
            else {
                db.Execute();
            } 
            break;

        case 4:
            if (state != 1) cerr << "Warning: Database hasn't been started yet!" << endl;
            else {
                db.Close();
                state = 0;
            }
            break;

        case 5:
            if(state == 1) cerr << "Warning: Close the database first!" << endl;
            else {
                cout << "Ending program" << endl;
                return 0;
            }
            break;
        
        default:
            cerr << "Invalid option. Please enter options between [1-5]." << endl;
            break;
        }
        
    }
    
}