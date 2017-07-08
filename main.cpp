# include "StateMachine.h"
# include <iostream>
# include <unordered_map>
# include <time.h>

using namespace std;

int main() {
	StateMachine sm;
	// unordered_map<std::string, int> ums;
	// unordered_map<std::string, int> ume;
	// DFA tests
	// the first example in 'deterministic finite automaton' in wiki, accept the multiple of 3 (in binary)
	/*
	ums["S0"] = sm.addState(STATE_BOTH);
	ums["S1"] = sm.addState(STATE_INTER);
	ums["S2"] = sm.addState(STATE_INTER);

	ume["E0"] = sm.addEdge(ums["S0"], ums["S0"], { '0' });
	ume["E1"] = sm.addEdge(ums["S0"], ums["S1"], { '1' });
	ume["E2"] = sm.addEdge(ums["S1"], ums["S2"], { '0' });
	ume["E3"] = sm.addEdge(ums["S2"], ums["S2"], { '1' });
	ume["E4"] = sm.addEdge(ums["S2"], ums["S1"], { '0' });
	ume["E5"] = sm.addEdge(ums["S1"], ums["S0"], { '1' });

	sm.doneConstruct();
	
	if (sm.feed("011") == OUTPUT_ACCEPT)
		cout << "first test passes!" << endl;
	else cout << "first test fails!" << endl;
	
	if (sm.feed("0") == OUTPUT_ACCEPT)
		cout << "second test passes!" << endl;
	else cout << "second test fails!" << endl; 
	
	if (sm.feed("110000101111111110011") == OUTPUT_ACCEPT)
		cout << "third test passes!" << endl;
	else cout << "third test fails!" << endl;
	
	if (sm.feed("110000101111111110100") == OUTPUT_ACCEPT)
		cout << "fourth test passes!" << endl;
	else cout << "fourth test fails!" << endl;
	
	if (sm.feed("10010100110001011010110000100000000101") == OUTPUT_ACCEPT)
		cout << "fifth test passes!" << endl;
	else cout << "fifth test fails!" << endl;
	*/
	// e-NFA tests
	/*
	ums["S0"] = sm.addState(STATE_START);
	ums["S1"] = sm.addState(STATE_INTER);
	ums["S2"] = sm.addState(STATE_INTER);
	ums["S3"] = sm.addState(STATE_INTER);
	ums["S4"] = sm.addState(STATE_INTER);
	ums["S5"] = sm.addState(STATE_INTER);
	ums["S6"] = sm.addState(STATE_ACCEPT);

	ume["E0"] = sm.addEdge(ums["S0"], ums["S1"], { 'a'});
	ume["E1"] = sm.addEdge(ums["S1"], ums["S2"], {});
	ume["E2"] = sm.addEdge(ums["S2"], ums["S3"], {});
	ume["E3"] = sm.addEdge(ums["S3"], ums["S4"], { 'b' });
	ume["E4"] = sm.addEdge(ums["S4"], ums["S2"], {});
	ume["E5"] = sm.addEdge(ums["S2"], ums["S5"], {});
	ume["E6"] = sm.addEdge(ums["S5"], ums["S6"], { 'a'});

	sm.doneConstruct();
	
	if (sm.feed("aa") == OUTPUT_ACCEPT)
		cout << "test 1 passes!" << endl;
	else cout << "test 1 fails!" << endl;

	if (sm.feed("aaa") == OUTPUT_ACCEPT)
		cout << "test 2 passes!" << endl;
	else cout << "test 2 fails!" << endl;

	if (sm.feed("aba") == OUTPUT_ACCEPT)
		cout << "test 3 passes!" << endl;
	else cout << "test 3 fails!" << endl;

	if (sm.feed("abbbbbbbbbbbbbbba") == OUTPUT_ACCEPT)
		cout << "test 4 passes!" << endl;
	else cout << "test 4 fails!" << endl;
	
	if (sm.feed("accccca") == OUTPUT_ACCEPT)
		cout << "test 5 passes!" << endl;
	else cout << "test 5 fails!" << endl;
	
	if (sm.feed("abbbabbba") == OUTPUT_ACCEPT)
		cout << "test 6 passes!" << endl;
	else cout << "test 6 fails!" << endl;
	*/
	RegexParser parser;
	
	/*
	parser.parseRegex("25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9]");
	parser.visualize();
	*/

	/*
	parser.parseRegex(R"(\(\w+(\s+\w+="[^"]+")*\))");
	parser.visualize();
	*/
	/*
	parser.parseRegex("[0-9]+.[0-9]");
	parser.visualize();
	*/
	/*
	parser.parseRegex("((aa|bb)|((ab|ba)(aa|bb)*(ab|ba)))*");
	parser.visualize();
	*/
	auto a = time(0);
	// regex for digit and annotation
	std::cout << sm.constructByRegex(R"([0-9]+|/\*([^\*]|\*+[^\*/])*\*+/|//.*\n)") << std::endl;
	DFA dfa = sm.generateDFA();
	auto b = time(0);

	std::ifstream ifs("test.txt");
	std::cout << "Done openning file" << std::endl;
	auto c = time(0);
	std::vector<std::vector<int>> ret = sm.feed(ifs);
	auto d = time(0);
	std::cout << ret.size() << std::endl;
	auto e = time(0);
	ret = dfa.feed(ifs);
	auto f = time(0);

	std::cout << ret.size() << std::endl;
	int count = 0;
	for (auto tup : ret) {
		std::cout << tup[0] << " " << tup[1] << " " << tup[2] << std::endl;
	}
	
	std::cout << "construction time: " << b - a << ", sm match time: " << d - c << ", dfa match time: " <<  f - e << std::endl;
	system("pause");

	return 0;
}