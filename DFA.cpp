# include "StateMachine.h"

construct_output DFA::doneConstruct() {
	if (start < 0)
		return ERROR_NOSTART;
	if (accept.size() == 0)
		return ERROR_NOACCEPT;

	legit = true;
	return CONSTRUCT_SUCCESS;
}

int DFA::addState(STATETYPE type){
	int id = nextid++;
	switch (type)
	{
	case STATE_BOTH:
	case STATE_START:		
		if (start < 0) {
			start = id;
			curr = start;
		}
		else {
			std::cout << "Start state has been set!" << std::endl;
			return -1;
		}
		if (type == STATE_START)
			break;
	case STATE_ACCEPT:
		accept.insert(id);
		break;
	case STATE_INTER:
		break;
	default:
		break;
	}

	return id;
}

void DFA::addTransition(int from, int to, char match) {
	if (transition[match][from] > 0) {
		std::cout << "Transition has been set!" << std::endl;
		return;
	}

	transition[match][from] = to;
}

feed_output DFA::feed(char input, bool greedy) {
	if (!legit) {
		std::cout << "DFA is still under construction!" << std::endl;
		return OUTPUT_ERROR;
	}

	int to = transition[input][curr];
	if (to < 0) { // not accept
		reset();
		return OUTPUT_NOTACCEPT;
	}
	else {
		curr = to;
		if (accept.count(curr) > 0) {
			if (!greedy)
				reset();
			return OUTPUT_ACCEPT;
		}
		else return OUTPUT_CONTINUE;
	}
}

feed_output DFA::feed(std::string input) {
	if (!legit) {
		std::cout << "DFA is still under construction!" << std::endl;
		return OUTPUT_ERROR;
	}

	char c;
	for (int i = 0; i < input.length(); i++) {
		c = input[i];
		switch (feed(c, true))
		{
		case OUTPUT_ACCEPT:
			if (i == input.length() - 1) {
				reset();
				return OUTPUT_ACCEPT;
			}
			break;
		case OUTPUT_CONTINUE:
			if (i != input.length() - 1)
				break;
		case OUTPUT_NOTACCEPT:
			reset();
			return OUTPUT_NOTACCEPT;
		default:
			break;
		}
	}
}

std::vector<std::vector<int>> DFA::feed(std::ifstream & fin) {
	std::vector<std::vector<int>> ret;  // line number, line pos, total length
	if (!legit) {
		std::cout << "DFA is still under construction!" << std::endl;
		return ret;
	}

	int line = 0;
	int linepos = 0;
	int lastEndLine = -1;
	int lastEndPos = -1;

	char c;
	while ((c = fin.get()) != EOF) {
		auto pos = fin.tellg();
		char currc = c;

		int tline = line, tpos = linepos, len = 0;
		int latestAcceptLine = -1, latestAcceptLinepos = -1, latestAcceptLen = -1;

		bool endflag = true;
		do {
			switch (feed(c, true)) {
			case OUTPUT_ACCEPT:
				len++;
				// store accept and greedily continue
				latestAcceptLine = tline;
				latestAcceptLinepos = tpos;
				latestAcceptLen = len;
				break;
			case OUTPUT_NOTACCEPT:
				// actually add result to ret when there's no way for greedy
				if (latestAcceptLine > lastEndLine || latestAcceptLinepos > lastEndPos) {
					lastEndLine = latestAcceptLine;
					lastEndPos = latestAcceptLinepos;
					ret.push_back(std::vector < int > {line, linepos, latestAcceptLen});
					latestAcceptLine = latestAcceptLinepos = latestAcceptLen = -1;
				}
				endflag = false;
				break;
			case OUTPUT_CONTINUE:
				len++;
				break;
			case OUTPUT_ERROR:
				reset();
				return ret;
			}
			tpos++;
			if (c == '\n') {
				tline++;
				tpos = 0;
			}
		} while ((c = fin.get()) != EOF && endflag);
		// handle lastAccept
		if (latestAcceptLen > 0 && (latestAcceptLine > lastEndLine || latestAcceptLinepos > lastEndPos))
			ret.push_back(std::vector < int > {line, linepos, latestAcceptLen});

		linepos++;
		if (currc == '\n') {
			line++;
			linepos = 0;
		}

		fin.seekg(pos);
	}

	reset();
	fin.clear();
	fin.seekg(0);
	return ret;
}

void DFA::visualizeMachine() {
	using namespace std;

	cout << "legit: " << legit << ", nextid: " << nextid 
		<<  ", size: " << size  << ", start: " << start 
		<< ",  curr: " << curr << endl;

	cout << "accept: " << endl;
	for (auto s : accept)
		cout << s << " ";
	cout << endl;

	cout << "transition: " << endl;
	for (int i = 0; i < SYMBOL_SIZE; i++) {
		for (int j = 0; j < size; j++) {
			cout << transition[i][j] << " ";
		}
		cout << endl;
	}
}