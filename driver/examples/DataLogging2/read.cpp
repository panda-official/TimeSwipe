#include<iostream>
#include<fstream>
#include<string.h>
#include<stdio.h>

#include <array> 
#include <vector>

using namespace std;

void usage(const char* name)
{
    std::cerr << "Usage: '" << name << " [--input <filename>]'" << std::endl;
}

struct Record
{
    array<float, 4> Sensors{0};
};

int main(int argc, char *argv[]) {
    std::string inputname;

    for (unsigned i = 1; i < argc; i++) {
        if (!strcmp(argv[i],"--input")) {
            if (i+1 > argc) {
                usage(argv[0]);
                return 1;
            }
            inputname = argv[i+1];
            ++i;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    ifstream rf(inputname, ios::out | ios::binary);
    if(!rf) {
        cout << "Cannot open file!" << endl;
        return 1;
    } else {
        cout << "Reading file: " << inputname << endl;
        
    }

    // get length of file:
    rf.seekg (0, rf.end);
    int length = rf.tellg();
    rf.seekg (0, rf.beg);

    int RecordLength = sizeof(Record);

    // Record rrec[length/RecordLength];   //on stack
    Record * rrec = new Record [length/RecordLength];    //on heap

    for(int i = 0; i < length/RecordLength; i++) {
        rf.read ((char *) &rrec[i],RecordLength);
    }

    if(!rf.good()) {
        cout << "Error occurred at reading time!" << endl;
        return 1;
    }

    size_t lastindex = inputname.find_last_of("."); 
    string outputname = inputname.substr(0, lastindex); 

    ofstream wf(outputname);
    if(!wf) {
        cout << "Cannot open file!" << endl;
        return 1;
    }

    for(int i = 0; i < length/RecordLength; i++) {
        auto rec = rrec[i];
        wf << rec.Sensors[0] << "\t" << rec.Sensors[1] << "\t" << rec.Sensors[2] << "\t" << rec.Sensors[3] << "\n";
    }
    wf.close();
    if(!wf.good()) {
        cout << "Error occurred at writing time!" << endl;
        return 1;
    } else {
        cout << "Successfully converted file: " << outputname << endl;
    }

    delete[] rrec;

    return 0;
}
