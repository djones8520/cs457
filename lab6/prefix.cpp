#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

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
		
		while ((pos = line.find(delimiter)) != std::string::npos) {
		    token = s.substr(0, pos);
		    std::cout << token << std::endl;
		    s.erase(0, pos + delimiter.length());
		}
	}

	return true;
}

int main(){
	if(setupTrie())
		return 0;	

	return 0;
}
