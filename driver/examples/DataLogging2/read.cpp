#include <iostream>
#include <fstream>
#include <string.h>
#include <stdio.h>
#include <array> 
#include <vector>

void usage ( const char* name )
{
	std::cerr << "Usage: '" << name << " <filename>'" << std::endl;
}

struct Record
{
	array < float, 4 > Sensors { 0 };
};

int main ( int argc, char *argv[] )
{

	if ( argc != 2 )
	{
		usage ( argv[0] );
		return 1;
	}

	std::string inputname = argv[1];

	ifstream rf ( inputname );
	if ( !rf )
	{
		std::cout << "Cannot open file: " << inputname << " !" << std::endl;
		return 1;
	}
	else
	{
		std::cout << "Reading file: " << inputname << " ..." << std::endl;
	}

	// get length of file:
	rf.seekg ( 0, rf.end );
	int length = rf.tellg ( );
	rf.seekg ( 0, rf.beg );

	int RecordLength = sizeof ( Record );

	Record * rrec = new Record [length/RecordLength];

	for ( int i = 0; i < length / RecordLength; i++ )
	{
		rf.read ( ( char * ) &rrec[i], RecordLength );
	}

	if ( !rf.good ( ) )
	{
		std::cout << "Error occurred whilst reading!" << std::endl;
		return 1;
	}

	size_t lastindex = inputname.find_last_of ( "." ); 
	string outputname = inputname.substr ( 0, lastindex ); 

	ofstream wf ( outputname );
	if ( !wf )
	{
		std::cout << "Cannot open output file!" << std::endl;
		return 1;
	}

	for ( int i = 0; i < length / RecordLength; i++ )
	{
		auto rec = rrec[i];
		wf << rec.Sensors[0] << "\t" << rec.Sensors[1] << "\t" << rec.Sensors[2] << "\t" << rec.Sensors[3] << "\n";
	}
	wf.close ( );
	if ( !wf.good ( ) )
	{
		std::cout << "Error occurred whilst writing!" << std::endl;
		return 1;
	}
	else
	{
		std::cout << "Successfully converted file: " << outputname << std::endl;
	}

	delete[] rrec;

	return 0;
}
