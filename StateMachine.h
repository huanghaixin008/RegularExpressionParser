# include <vector>
# include <stack>
# include <unordered_set>
# include <unordered_map>
# include <set>
# include <map>
# include <string>
# include <iostream>
# include <fstream>
# include "RegexParser.h"

enum STATETYPE {
	STATE_START,
	STATE_ACCEPT,
	STATE_BOTH,
	STATE_INTER
};

enum feed_output {
	OUTPUT_ACCEPT = 1000,
	OUTPUT_NOTACCEPT,
	OUTPUT_CONTINUE,
	OUTPUT_ERROR
};

enum construct_output {
	ERROR_NOACCEPT = 7100,
	ERROR_NOSTART,
	ERROR_SINGULARSTATE,
	ERROR_PARSEERROR,
	CONSTRUCT_SUCCESS
};

// an efficient DFA implementation
#define SYMBOL_SIZE 256

class DFA {
private:
	bool legit;
	int nextid;
	int size;
	int start;
	int curr;
	std::unordered_set<int> accept;

	int* transition[SYMBOL_SIZE];
public:
	DFA(int _size) {
		size = _size;
		start = -1;
		curr = -1;
		nextid = 0;
		for (int i = 0; i < SYMBOL_SIZE; i++) {
			transition[i] = new int [size];
			for (int j = 0; j < size; j++)
				transition[i][j] = -1;
		}
	}
	DFA(const DFA & dfa) {
		start = dfa.start;
		curr = dfa.curr;
		nextid = dfa.nextid;
		legit = dfa.legit;
		size = dfa.size;
		for (auto s : dfa.accept)
			accept.insert(s);
		for (int i = 0; i < SYMBOL_SIZE; i++) {
			transition[i] = new int [size];
			for (int j = 0; j < size; j++)
				transition[i][j] = dfa.transition[i][j];
		}
	}
	~DFA() {
		for (int i = 0; i < SYMBOL_SIZE; i++) {
			delete[] transition[i];
		}
	}

	void reset() { curr = start; };

	int addState(STATETYPE type);
	void addTransition(int from, int to, char match);
	construct_output doneConstruct();
	void visualizeMachine();

	feed_output feed(std::string input);
	feed_output feed(char input, bool greedy = false); // greedy: do not stop matching and reset after accept, allowing greedy match
	std::vector<std::vector<int>> feed(std::ifstream & fin);
};

// an e - Nondeterministic Finite Automata
class StateMachine {
	struct edge;
	struct state;
	struct state {
		int id;
		STATETYPE type;
		std::unordered_set<int> in;
		std::unordered_set<int> out;
	};
	struct edge {
		int id;
		int from, to;
		std::unordered_set<char> match;
	};
	struct machineEntry {  // define entries of a sub graph
		int startid;
		int endid;
		machineEntry(int a, int b) : startid(a), endid(b) {};
	};
private:
	int nextsid;
	int nexteid;
	bool legit, hasAccept;
	bool startAccept;
	RegexParser parser;
	int startid;
	state* start;
	std::vector<state*> curr;
	std::unordered_map<int, state*> states;
	std::unordered_map<int, edge*> edges;

	void resetState() {
		curr.clear();
		curr.push_back(start);
	};
	// bool canTransfer(std::vector<char> match, char input);
	feed_output checkEmptyEdge();
	// construct a tree from a regex tree (recursively)
	machineEntry constructFromTree(regexTreeNode* node);
	construct_output constructMatch(machineEntry& me, const std::string match, std::unordered_set<char> &vmatch);
	construct_output constructRepeat(machineEntry& me, const std::vector<machineEntry>& childEntry, bool infinite, int min, int max);
	construct_output constructParallel(machineEntry& me, const std::vector<machineEntry>& childEntry);
	construct_output constructCascade(machineEntry& me, const std::vector<machineEntry>& childEntry);

	construct_output eliminateEmptyEdge();
	construct_output transformToDFA();
	bool findClosure(std::unordered_set<int>& closure, state* begin);
public:
	StateMachine() { start = NULL; nextsid = 0; nexteid = 0; legit = false; hasAccept = false; startAccept = false; };
	StateMachine(std::string regex) {
		constructByRegex(regex);
	};
	~StateMachine() {
		for (auto it = states.begin(); it != states.end();it++)
			delete it->second;
		for (auto it = edges.begin(); it != edges.end(); it++)
			delete it->second;
	};
	feed_output feed(std::string input);
	feed_output feed(char input, bool greedy=false); // greedy: do not stop matching and reset after accept, allowing greedy match
	std::vector<std::vector<int>> feed(std::ifstream & fin);
	// construct manually
	int addState(STATETYPE type);
	int addEdge(int from, int to, std::unordered_set<char> match);
	int addEdgeExclusive(int from, int to, std::unordered_set<char> match);
	void deleteState(int id);
	void deleteEdge(int id);
	void clear();
	construct_output doneConstruct(bool extend=true); // TODO, return error info
	// construct by regex
	construct_output constructByRegex(std::string regex);
	// TODO DFA generateDFA();
	DFA generateDFA();
	void visualizeParseTree() { parser.visualize(); };
	void visualizeMachine(std::string out="");
};
