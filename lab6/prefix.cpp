#include <iostream>
#include <fstream>
#include <sstream>
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

	while(getline(file, line)){
		cout << line << endl;

		string delimiter = "|";

		size_t pos = 0;
		int count = 0;
		string ipaddress;
		string path;
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

int main(){	
	root->data = "test";
	cout << root->data << endl;

	string address = "1.1.1.1";
	string path = "00000000111";

	addNode(path, address);

	return 0;
}
