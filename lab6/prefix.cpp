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

Node *root = new Node;

bool setupTrie(){
	ifstream file("bgprib20131101.txt");

	string line;
	string FINALPREFIX = "";
	int MINLENGTH = -1;
	string FINALHOP = "";

	while(getline(file, line)){
		cout << line << endl;
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
			cout << "PREFIX: " << FINALPREFIX << endl;
			cout << "LENGTH: " << MINLENGTH << endl;
			cout << "NXTHOP: " << FINALHOP << endl;

			addNode(FINALPREFIX, FINALHOP);

			std::this_thread::sleep_for (std::chrono::seconds(5));

			FINALPREFIX = prefix;
			MINLENGTH = length;
			FINALHOP = next_hop;
		}
	}

	return true;
}

void addNode(string path, string address){
	int pos = 0;
	Node *nodePos = root;
	Node *test = root;

	int check = 0;
	while((check = path.length() - pos) > 0){
		string nextNode;

		if(check == 1){
			nextNode = path.substr(pos);
			nextNode += "0";
		}
		else{
			nextNode = path.substr(pos, 2);
		}

		int caseCompare = stoi(nextNode);
		switch(caseCompare){
			case 0:
				if(nodePos->zero0 == NULL)
					nodePos->zero0 = new Node;

				nodePos = nodePos->zero0;

				break;
			case 1:
				if(nodePos->zero1 == NULL)
					nodePos->zero1 = new Node;

				nodePos = nodePos->zero1;

				break;
			case 10:
				if(nodePos->one0 == NULL)
					nodePos->one0 = new Node;

				nodePos = nodePos->one0;

				break;
			case 11:
				if(nodePos->one1 == NULL)
					nodePos->one1 = new Node;

				nodePos = nodePos->one1;

				break;
		}

		pos += 2;
	}

	nodePos->data = address;
	int hi =0;
}

void findMatch(string address){
	
}

int main(int argc, char** argv){
	if (argc != 3) {
		printf("Please run program using 2 arguments");
		return 0;
	}
	
	if(!setupTrie())
		return 0;	

	root->data = "test";
	cout << root->data << endl;

	string address = "1.1.1.1";
	string path = "00000000111";

	addNode(path, address);

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
