// Author: Mansour Saffar Merhjardi (saffarme@ualberta.ca)
// Date: January 2017
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <vector>
#include <unordered_map>
#include <set>
#include <math.h>
#include <algorithm>
#include <tuple>
#include <iomanip>
#include <sstream>


using namespace std;


// implementation of Trie (Prefix) tree (Nodes are hashed by unordered map(hash table))
class Trie{

public:
	Trie(int d){
		depth = d;
		isLeaf = true;
	}

	void addChild(Trie* c, int key){
		children[key] = c;
		isLeaf = false;
	}


	int getDepth(){return depth;}

	bool hasChild(int key){
		if(children.find(key) != children.end()){
			return true;
		}
		else{
			return false;
		}
	}

	Trie* getChild(int key){
		return children[key];
	}

	bool isLeafNode(){
		return isLeaf;
	}


private:
	int depth;
	bool isLeaf;
	unordered_map<int, Trie*> children;

};


// Implementation of Apriori class

class Apriori{

public:
	// constructor
	Apriori(char* f,float s, float c,string o){
		fileName = f;
		minSup = s;
		minConf = c;
		option = o;
	}

	void getInfoAboutData();
	int getNumberOfTransactions(){return numOfTransactions;}
	void printItemSet();
	vector<string> getItemSet(){return itemSet;}
	int getItemSetSize(){return itemSet.size();}
	void findAllFrequentItemSets();
	void printCs();
	void outputResults();
	void Delete();


private:
	// private fields
	vector<string> transactions;
	vector<string> strongRules;
	float minSup;
	float minConf;
	char* fileName;
	string option;
	int numOfTransactions = 0;
	vector<string> itemSet;
	vector<vector<string>*> candidateItemSets;
	vector<unordered_map<string,int>*> largeItemSets;
	bool epmtySetIsObserved = false;
	// private methods
	void parseTransaction(string&, vector<string>&);
	void parseTransactionInteger(string&, vector<int>&);
	void generateNewCandidates(vector<string>*, int);
	void countItemSets(vector<string>*, unordered_map<string,int>*,int);
	void extractLargeItemSets(unordered_map<string, int>*);
	void countItems(Trie*, vector<int>, int, int subset[],int, int, int,unordered_map<string,int>*);
	void createTrie(vector<string>*, Trie*);
	bool prune(string&,int , Trie*);
	void prunebyTrie(Trie*, vector<int>,int, int subset[],int , int , int, bool&);
	void printLargeSets();
	void createStrongRules();
	void makeRule(vector<tuple<string,string>>&, string& , int);
	void partitionFrequentItemSet(vector<int> ,int, int subset[],int, int, int , vector<tuple<string, string>>&);
	void printStrongRules();

};


// Get number of transactions, Unique Items in database and count of them.
void Apriori::getInfoAboutData(){

	unordered_map<string, int>* temp = new unordered_map<string,int>;
	ifstream dataFile(fileName);
	string line;
	vector<string> T;
	set<string> itemSET;
	while(getline(dataFile,line)){
		// add line to transactions
		transactions.push_back(line);
		// parse transaction
		parseTransaction(line, T);
		// add items to itemSet
		vector<string>::iterator it;
		for(it=T.begin() ; it!=T.end() ; ++it){
			itemSET.insert(*it);
			// count it in temp
			if(temp->find(*it + " ") != temp->end()){
				temp->at(*it+" ") += 1;
			}
			else{
				temp->insert({*it+" ",1});
			}

		}
		// clear T
		T.clear();
		// Increment number of transactions by 1 
		numOfTransactions++;
	}
	// add set to vector
	set<string>::iterator iter;
	for(iter = itemSET.begin(); iter!=itemSET.end();iter++){
		itemSet.push_back(*iter);
	}
	// sort vector
	sort (itemSet.begin(), itemSet.end()); 
	extractLargeItemSets(temp);
	delete temp;
}



void Apriori::parseTransaction(string& line, vector<string>& T){

	int pos;
	while((pos = line.find(' ')) != line.npos){
		T.push_back(line.substr(0, pos));
		line.erase(0, pos+1);
	}
}



void Apriori::parseTransactionInteger(string& line, vector<int>& T){

	int pos;
	while((pos = line.find(' ')) != line.npos){
		T.push_back(atoi(line.substr(0, pos).c_str()));
		line.erase(0, pos+1);
	}
}



void Apriori::printItemSet(){

	vector<string>::iterator it;
	for(it=itemSet.begin() ; it!=itemSet.end(); ++it){
		cout << *it << endl;
	}
}



// Generate new candidates
void Apriori::generateNewCandidates(vector<string>* kthItemSet, int k){


	// If k=2 then C will be combination of all 2 items in L1
	if(k==2){

		vector<int> temp;

		unordered_map<string,int>::iterator it;
		string s;
		for(it=largeItemSets[k-2]->begin() ; it!=largeItemSets[k-2]->end(); ++it){
			s = it->first;
			// remove last space and add it to temp
			temp.push_back(atoi(s.erase(s.size()-1).c_str()));
		}

		sort (temp.begin(),temp.end());
		for(int i = 0; i < temp.size(); i++){
			int j = i+1;
			while(j < temp.size()){
				kthItemSet->push_back(to_string(temp[i]) + " " + to_string(temp[j])+ " ");
				j++;
			}
		}

		if(kthItemSet->size() == 0 ){
			epmtySetIsObserved = true;
			return;
		}
		candidateItemSets.push_back(kthItemSet);
	}


	// If k > 2 then joining and pruning should be done [As in the paper]
	if(k > 2){

		vector<string>* temp = new vector<string> ;
		vector<string>* tempCopy = new vector<string>;
		unordered_map<string,int>::iterator it;
		string s;
		for(it=largeItemSets[k-2]->begin() ; it!=largeItemSets[k-2]->end(); ++it){
			s = it->first;
			tempCopy->push_back(s);
			temp->push_back(s.erase(s.size()-1));
		}

		//  build tree based on Large Itemsets L_(k-1)		
		Trie* root = new Trie(0);
		createTrie(tempCopy,root);
		delete tempCopy;

		// Join frquent itemsets and decide whether to prune them or not
		string pass;
		for(int i =0; i < temp->size() ; i++){
			for(int j = i+1 ; j < temp->size() ; j++){
				// if first k-1 items are the same, then join them
				if(temp->at(i).substr(0,temp->at(i).rfind(" ")) == temp->at(j).substr(0,temp->at(j).rfind(" "))){
					int mi = min(stoi(temp->at(i).substr(temp->at(i).rfind(" ")+1,temp->at(i).size()-1)) , 
						stoi(temp->at(j).substr(temp->at(j).rfind(" ")+1,temp->at(j).size()-1)));
					int ma = max(stoi(temp->at(i).substr(temp->at(i).rfind(" ")+1,temp->at(i).size()-1)) , 
						stoi(temp->at(j).substr(temp->at(j).rfind(" ")+1,temp->at(j).size()-1)));
					pass = temp->at(i).substr(0,temp->at(i).rfind(" ")) + " " + to_string(mi) + " " + to_string(ma) + " ";
					string passCopy = pass;
					// determine whether to prune the new candidate or not
					if(!prune(pass, k,root)){
						kthItemSet->push_back(passCopy);
					}

				}
			}
		}
		//  delete root and temp
		delete temp;
		delete root;
		// if an empty candidate set is seen ,terminate further creation of subsequent large itemset
		if(kthItemSet->size() == 0){
			epmtySetIsObserved = true;
		}


	}

}


// Prune the new candidate by Trie built by large itmesets of size k-1.
bool Apriori::prune(string& line,int k, Trie* tree){
	vector<int> T;
	parseTransactionInteger(line,T);
	// int subset[k];
	int subset[k-1];
	bool prune = false;
	// recursively find each k-1 subset of T and check if they are in Trie (built by L_k-1)
	prunebyTrie(tree, T, k-1, subset, 0, T.size()-1, 0, prune);
	return prune;
}


// recursive function to find all subsets of size k from candidate
void Apriori::prunebyTrie(Trie* head, vector<int> can,int k, int subset[],int start, int end, int index, bool& prune){

	// if prune flag is already set, return
	if(prune){return;}
	// if subset of size k-1 is reached, see if it's in tree or not
	if(index == k){
		Trie* t = head;
		for(int j=0 ; j < k ; j++){
			// if not in tree, set prune to true and return
			if(!t->hasChild(subset[j])){
				prune = true;
				return;
			}
			// if seen, then go to child and process more
			else{t = t->getChild(subset[j]);}
			// if leaf node is reached, return
			if(t->isLeafNode()){return;}	
		}
	}

	for(int i = start ;  i <= end && end-i+1>=k-index ; i++){
		subset[index] = can[i];
		prunebyTrie(head, can,k, subset, i+1, end, index+1,prune);
	}

}


// Find all frequent item sets
void Apriori::findAllFrequentItemSets(){
	// frequent itemsets of size 1 were determined when reading the database.
	// starting form k=2, generate new candidates based of previous large 
	// itemsets and count them to create large itemsets of next size.
	int k = 2;
	while(true){
		unordered_map<string, int>* tempMap = new unordered_map<string,int>;
		vector<string>* tempSet = new vector<string>;
		// Generate candidates
		generateNewCandidates(tempSet, k);
		// if an empty candidate set is seen, terminate further generation of L
		if(epmtySetIsObserved){
			break;}
		// count new candidates
		countItemSets(tempSet, tempMap,k);
		// extract large candidates to create next large itemset
		extractLargeItemSets(tempMap);
		// delete tempMap,  it's no longer useful
		delete tempMap;
		// if an empty large itemset is seen. terminate further creation of C 
		if(epmtySetIsObserved){
			break;}
		k++;
		// if(k > 2){break;}
	}

}


// This function counts number of occurrences of candidates in tempSet in all transactions
// and adds them to tempMap. 
void Apriori::countItemSets(vector<string>* tempSet, unordered_map<string,int>* tempMap,int k){

	// create Trie based on candidates
	Trie* root = new Trie(0);
	createTrie(tempSet,root);
	// iterator through transactions
	string line;
	vector<int> T;
	for(int m = 0; m < transactions.size(); m++){
		line = transactions[m];
		parseTransactionInteger(line, T);
		// if size of transaction is at least k, then process it to 
		// count candidates.
		if(T.size() >= k){
			int subset[k];
			countItems(root, T, k, subset,0, T.size()-1, 0,tempMap);
		}	

		T.clear();
	}
	delete root;

}



// Function to extract large itemsets from the candidates
void Apriori::extractLargeItemSets(unordered_map<string, int>* tempMap){

	// UNCOMMENT TO MAKE IT PERCENTAGE
	// int minSupInt = ceil(numOfTransactions * minSup);
	// UNCOMMENT TO MAKE MINSUP = NUMBER OF TRANSACTIONS
	int minSupInt = ceil(minSup);

	// cout << "minsupint= " << minSupInt << endl;
	
	unordered_map<string, int>* largeSets = new unordered_map<string, int>;
	unordered_map<string, int>::iterator it;
	for(it = tempMap->begin() ; it!=tempMap->end() ; ++it){
		if(it->second >= minSupInt){
			largeSets->insert({it->first, it->second});
		}
	}
	// add largeSets to largeItemSets if it is not empty
	if(!largeSets->empty()){
		largeItemSets.push_back(largeSets);
	}
	else{
		epmtySetIsObserved = true;
	}

}


// function to print results based on fourth argument given to program
void Apriori::outputResults(){

	// decide what to output based on option
	if(option=="n"){
		// create rules
		createStrongRules();
		unordered_map<string,int>::iterator it;
		for(int i = 0 ; i < largeItemSets.size(); i++){
			cout << "Number of frequent_" << i+1 << " itemsets: "<< largeItemSets[i]->size() << endl;
		}
		cout << "Number of association rules: " << strongRules.size() << endl;
	}
	// print all rules and frequent itemsets
	else if(option=="a"){
		createStrongRules();
		printLargeSets();
		printStrongRules();
	}
	// print all frequent itemsets
	else if(option=="f"){
		printLargeSets();
	}
	//  print all rules
	else if(option=="r"){
		createStrongRules();
		printStrongRules();
	}
}


// extract possible strong rules from large itemsets and determine if they are strong or not
void Apriori::createStrongRules(){

	unordered_map<string,int>::iterator it;
	//  iterate through largeItemSet with size > 1
	vector<int> T;
	vector<tuple<string, string>> partitions;
	string s;
	// for every large itemset of size i, partition it into 2 parts of a rule and 
	// determine if the rule is strong or not
	for(int i = 1 ; i < largeItemSets.size() ; i++){
		for(it = largeItemSets[i]->begin() ; it!=largeItemSets[i]->end() ; ++it){
			s = it->first;
			parseTransactionInteger(s,T);
			// recursively create subsets of size 1 to k-1. 
			// both parts of rule are stored in partitions.
			for(int m = 1 ; m <= i ; m++){
				int subset[m];
 				partitionFrequentItemSet(T,m, subset,0, T.size()-1, 0,partitions);
			}
			s = it->first;
			// determine all strong rules created from partitioning this itemSet
			makeRule(partitions, s,i);
			T.clear();
			partitions.clear();

		}
	}

}


// This function determines which candidate strong rules are strong.

void Apriori::makeRule(vector<tuple<string,string>>& partitions, string& all, int k){
	// determine all the strong rules
	string pred;
	string rule;
	string predCopy;
	vector<int> P;
	for(int i = 0 ; i < partitions.size() ; i++){
		rule = "";
		P.clear();
		pred = get<0>(partitions[i]);
		predCopy = pred;
		parseTransactionInteger(predCopy,P);
		int supp = largeItemSets[k]->at(all);
		float confidence = float(supp)/largeItemSets[P.size()-1]->at(pred);
		// if rule is strong, add it to strongRules with its support and confidence
		
		// sanity check
		// cout << "Minimum confidence: " << minConf << endl;

		if(confidence >= minConf){
			for(int j = 0 ; j < P.size() ; j++){
				if( j != P.size()-1){
					rule += to_string(P[j]) + ", ";
				}
				else{
					rule += to_string(P[j]);
				}
			}

			rule += " -> ";

			P.clear();
			parseTransactionInteger(get<1>(partitions[i]),P);
			for(int j = 0 ; j < P.size() ; j++){
				if( j != P.size()-1){
					rule += to_string(P[j]) + ", ";
				}
				else{
					rule += to_string(P[j]) + " " ;
				}
			}
			// print support and confidence
			stringstream sstSup;
			stringstream sstConf;
			sstSup << fixed << setprecision(2) << float(supp)/numOfTransactions;
			sstConf << fixed << setprecision(2) << confidence;
			rule += "(" + sstSup.str() + "," + sstConf.str() + ")";
			strongRules.push_back(rule);
		}
	}

}


// As in countItems, this function creates subsets of size k from can recursively and 
// for each subset of size k, its complement (items which are in can but not in subset)
// is calculated and added to partitions.
void Apriori::partitionFrequentItemSet(vector<int> can,int k, int subset[],int start, int end, int index, vector<tuple<string, string>>& partitions){
	// check if a subset of size k is reached
	if(index == k){
		// create first part and second part of candidate rule. first part is 
		// the subset and second part is all the missing ones in subset that are in can
		string firstPart = "";
		string secPart = "";
		bool in;
		// extract second part from can [those that are not in subset]
		for(int i = 0 ; i < can.size() ; i++){
			in = false;
			for(int j = 0 ; j < k ; j++){
				if(can[i]==subset[j]){
					in = true;
				}				
			}
			if(!in){
				secPart += to_string(can[i]) + " ";
			}
		}
		// first part is the subset
		for(int i = 0 ; i < k ; i++){
			firstPart += to_string(subset[i]) + " ";
		}
		//  add them as a tuple to partitions and return
		partitions.push_back(make_tuple(firstPart,secPart));
		return;
	}


	for(int i = start ;  i <= end && end-i+1>=k-index ; i++){
		subset[index] = can[i];
		partitionFrequentItemSet(can,k, subset, i+1, end, index+1,partitions);
	}

}



// Printing a list of frequent itemsets of different sizes
void Apriori::printLargeSets(){

	unordered_map<string,int>::iterator it;
	// cout << "Printing Large ItemSets " << endl;
	for(int i = 0 ; i < largeItemSets.size(); i++){
		cout << "Frequent_" << i+1 << " itemsets"<< endl;
		for(it = largeItemSets[i]->begin(); it!= largeItemSets[i]->end(); ++it){
			cout <<  it->first<<"(";
			printf("%.2f", float(it->second)/numOfTransactions);
			cout <<")" << endl;
		}
	}

}


// Printing a list of strong rules
void Apriori::printStrongRules(){
	for(int i = 0 ; i < strongRules.size() ; i++){
		cout << strongRules[i]<<endl;
	}
}


// Destructor for Apriori class 
void Apriori::Delete(){

	// release pointers in candidateItemSets
	vector<vector<string>*>::iterator it;
	// cout << "Deleting candidateItemSets" << endl;
	for(it = candidateItemSets.begin(); it!= candidateItemSets.end(); ++it){
		delete *it;
	}

	// release pointers in largeItemSets
	vector<unordered_map<string,int>*>::iterator itm;
	// cout << "Deleting largeItemSets " << endl;
	for(itm = largeItemSets.begin(); itm!= largeItemSets.end(); ++itm){
		delete *itm;
	}

}

// this function counts number of occurrences of a subset of size k in a transaction.
// It first finds all subsets of size k in trans recursively and for each subset,
// Trie created by candidates is traversed to count increment count of those subsets
// present in trans.
void Apriori::countItems(Trie* head, vector<int> trans,int k, int subset[],int start, int end, int index,unordered_map<string,int>* tempMap){
	// if subset of size k is reached, see if it is in Trie or not.
	if(index == k){
		Trie* t = head;
		// process items in candidates one by one to traverse the Trie
		for(int j=0 ; j < k ; j++){
			// if item is not in Trie then item is not in the transaction
			if(!t->hasChild(subset[j])){
				break;
			}
			else{
				t = t->getChild(subset[j]);
			}
			// if a leaf node is reached, the item is in transaction and should be counted once more.
			if(t->isLeafNode()){
				string temp = "";
				for(int l = 0 ; l < k ; l++){
					temp += to_string(subset[l]) + " " ;
				}
				//  count candidate
				if(tempMap->find(temp) != tempMap->end()){
					tempMap->at(temp) += 1;
				}
				else{
					tempMap->insert({temp,1});
				}

			}			
		}
		return;
	}
	for(int i = start ;  i <= end && end-i+1>=k-index ; i++){
		subset[index] = trans[i];
		countItems(head, trans,k, subset, i+1, end, index+1, tempMap);
	}
}


// function to build Trie(Prefix tree) based on Candidates.
void Apriori::createTrie(vector<string>* itemSets, Trie* r){

	vector<int> T;
	Trie* head;
	// for each candidate, traverse the Trie and add new nodes if needed (already not created)
	for(int i = 0; i < itemSets->size(); i++){
		T.clear();
		string data = itemSets->at(i);
		parseTransactionInteger(itemSets->at(i), T);
		vector<int>::iterator it;
		head = r;
		for(it=T.begin();it!=T.end();it++){
			// if item is not seen before, create a node and add to its parent
			if(!head->hasChild(*it)){
				Trie* c = new Trie(head->getDepth()+1);
				head->addChild(c, *it);
				head = c;
			}
			// if item is already seen, go to it until a leaf node is reached
			else{
				if(head->isLeafNode()){
					break;
				}
				head = head->getChild(*it);
			}
		}
	}
}



////////////////////////////////// MAIN FUNCTION //////////////////////////////////

int main(int argc, char* argv[]){

	// timer for benchmarking
	clock_t t1;
	// start timer
	t1 = clock();

	// parse input arguments
	float minsup = atof(argv[2]);
	float minconf = atof(argv[3]);
	string option;

	if(argc == 5){ // check if enough arguments is provided
		option = string(argv[4]);
	}
	else{
		option  = "n";
	}

	Apriori a(argv[1], minsup, minconf,option);

	cout << "Getting Info about dataset" << endl;
	a.getInfoAboutData();
	cout << "Number of Transactions: " << a.getNumberOfTransactions() << endl;

	cout << "Finding Frequent Patterns in the Dataset" << endl;
	a.findAllFrequentItemSets();

	cout << "Outputting Results" << endl;
	a.outputResults();
	a.Delete();
	// uncomment to print elapsed time
	// cout<< (clock()- t1)/double(CLOCKS_PER_SEC) << endl;

}
