#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <thread>
#include <chrono>
#include <stdlib.h>
#include <bitset>
#include <cstring>
#include <string>
#include <algorithm>

using namespace std;

string getPrefix(string input);
void addNode(string path, string address);

#include <string>
#include <stdlib.h>

using namespace std;

class Node{
	public:
		string data;
		Node* zero0;
		Node* zero1;
		Node* one0;
		Node* one1;	
};

Node *root = new Node();

void setupTrie(string fileName){
	ifstream file("bgprib20131101.txt");
	
	int nodeCount = 0;

	string line;
	string FINALPREFIX = "";
	int MINLENGTH = -1;
	string FINALHOP = "";

	while(getline(file, line)){
		//cout << line << endl;
		string delimiter = "|";

		size_t pos = 0;
		string token;
		int count = 0;

		string info[3];
		while ((pos = line.find(delimiter)) != std::string::npos) {
		    token = line.substr(0, pos);
			info[count] = token;
			count++;
		    line.erase(0, pos + delimiter.length());
		}

		info[2] = line;

		string prefix = getPrefix(info[0]);
		int length = std::count(info[1].begin(), info[1].end(), ' ');
		string next_hop = info[2];

		if (FINALPREFIX.compare("") == 0) { // First line of the file
			FINALPREFIX = prefix;
			MINLENGTH = length;
			FINALHOP = next_hop;
		}

		if (FINALPREFIX.compare(prefix) == 0) { // Within same block of prefixes
			if (length < MINLENGTH) {
				MINLENGTH = length;
				FINALHOP = next_hop;
			}
		} else { // Entered new block of prefixes
			//cout << "PREFIX: " << FINALPREFIX << endl;
			//cout << "LENGTH: " << MINLENGTH << endl;
			//cout << "NXTHOP: " << FINALHOP << endl;

			addNode(FINALPREFIX, FINALHOP);
			
			nodeCount++;
			cout << "Node added: " << nodeCount << " Hop: " << FINALHOP << endl;

			FINALPREFIX = prefix;
			MINLENGTH = length;
			FINALHOP = next_hop;
		}
	}
	
	cout << "End Node Setup" << endl;
}

void addNode(string path, string address){
	//cout << "PATH: " << path << endl;
	//cout << "DATA: " << address << endl;

	//cout << "HERE 1" << endl;
	
	int pos = 0;
	Node *nodePos = root;

	//cout << "HERE 2" << endl;

	int check = 0;
	while((check = path.length() - pos) > 0){
		//cout << "HERE 3" << endl;
		string nextNode;

		if(check == 1){
			nextNode = path.substr(pos);
			nextNode += "0";
		}
		else{
			nextNode = path.substr(pos, 2);
			//cout << "NEXTNODE: " << nextNode << endl;
		}

		//cout << "HERE A" << endl;

		int caseCompare = stoi(nextNode);

		//cout << "HERE B" << endl;

		switch(caseCompare){
			case 0:
				//cout << "HERE 4.1" << endl;				
				if(nodePos->zero0 == NULL) {
					//cout << "HERE 4.1A" << endl;					
					nodePos->zero0 = new Node();
					//cout << "HERE 4.1B" << endl;
				}
				//cout << "HERE 4.2" << endl;
				nodePos = nodePos->zero0;
				//cout << "HERE 4.3" << endl;
				break;
			case 1:
				//cout << "HERE 5" << endl;
				if(nodePos->zero1 == NULL)
					nodePos->zero1 = new Node();

				nodePos = nodePos->zero1;

				break;
			case 10:
				//cout << "HERE 6" << endl;
				if(nodePos->one0 == NULL)
					nodePos->one0 = new Node();

				nodePos = nodePos->one0;

				break;
			case 11:
				//cout << "HERE 7" << endl;
				if(nodePos->one1 == NULL)
					nodePos->one1 = new Node();

				nodePos = nodePos->one1;

				break;
		}
		//cout << "BEFORE POS INC: " << pos << endl;
		pos += 2;
		//cout << "AFTER POS INC: " << pos << endl;
	}
	//cout << "HERE FINAL" << endl;

	nodePos->data = address;
	int hi =0;
}

string findMatch(string address){	
	int pos = 0;
	Node *nodePos = root;
	string match = "No match";

	if(!root->data.empty())
		match = root->data;

	int check = 0;
	while((check = address.length() - pos) > 0){
		string nextNode;

		if(check == 1){
			nextNode = address.substr(pos);
			nextNode += "0";
		}
		else{
			nextNode = address.substr(pos, 2);
		}

		int caseCompare = stoi(nextNode);
		switch(caseCompare){
			case 0:
				if(nodePos->zero0 != NULL)
					nodePos = nodePos->zero0;

				break;
			case 1:
				if(nodePos->zero1 != NULL)
					nodePos = nodePos->zero1;

				break;
			case 10:
				if(nodePos->one0 == NULL)
					nodePos = nodePos->one0;

				break;
			case 11:
				if(nodePos->one1 == NULL)
					nodePos = nodePos->one1;

				break;
			default:
				if(!nodePos->data.empty())
					match = nodePos->data;
		}		
	}

	return match;
}

void readFile(string fileName) {
	ifstream file("sampleips.txt");
	string line;

	ofstream outputFile;
	outputFile.open("output.txt");

	while(getline(file, line)){
		// Convert address to binary
		int ipInt[4];
		int count = 0;
		string delimiter = ".";
		string token;
		size_t pos = 0;
		while ((pos = line.find(delimiter)) != std::string::npos) {
			token = line.substr(0, pos);
			ipInt[count] = atoi(token.c_str());
			count++;
			line.erase(0, pos + delimiter.length());
		}

		ipInt[3] = atoi(line.c_str());
		string binaryIP;

		for (int i = 0; i < 4; i++) {
			binaryIP += bitset<8>(ipInt[i]).to_string();
		}

		cout << "End Write to file" << endl;
		outputFile << line << "\t" << findMatch(binaryIP) << endl;
	}
	
	outputFile.close();
}

int main(int argc, char** argv){
	if (argc != 3) {
		printf("Please run program using 2 arguments");
		return 0;
	}

	string pathsFile = argv[1];
	string ipsFile = argv[2];
	
	setupTrie(pathsFile);
	readFile(ipsFile);

	return 0;
}

string getPrefix(string input) {
	string line = input;	
	string delimiter = "/";

	size_t pos = 0;
	string token, ipaddress;
	int limit;
	while ((pos = line.find(delimiter)) != std::string::npos) {
		token = line.substr(0, pos);
		ipaddress = token;
		line.erase(0, pos + delimiter.length());
	}

	int ipInt[4];
	int count = 0;
	delimiter = ".";
	pos = 0;
	while ((pos = ipaddress.find(delimiter)) != std::string::npos) {
		token = ipaddress.substr(0, pos);
		ipInt[count] = atoi(token.c_str());
		count++;
		ipaddress.erase(0, pos + delimiter.length());
	}

	ipInt[3] = atoi(ipaddress.c_str());
	string binaryIP;

	for (int i = 0; i < 4; i++) {
		binaryIP += bitset<8>(ipInt[i]).to_string();
	}

	limit = atoi(line.c_str());
	binaryIP = binaryIP.substr(0, limit);

	return binaryIP;
}
