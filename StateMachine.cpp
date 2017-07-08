# include "StateMachine.h"
# include <queue>

// # define eNFA_DEBUG
// # define FLOW_DEBUG
/*
bool StateMachine::canTransfer(std::vector<char> match, char input) {
	for (auto m : match)
		if (m == input)
			return true;
	return false;
} */

feed_output StateMachine::checkEmptyEdge() {
	bool flag = false;
	bool accept = false;
	int start = 0;
	int end = curr.size();
	std::vector<int> counts;
	while (start < end) {
		for (int i = start; i < end; i++) {
			auto c = curr[i];
			int count = 0;
			for (auto e : c->out) {
				if (edges[e]->match.size() == 0) {
					flag = true;
					count++;
					curr.push_back(states[edges[e]->to]);
					if (states[edges[e]->to]->type == STATE_ACCEPT || states[edges[e]->to]->type == STATE_BOTH)
						accept = true;
				}
			}
			counts.push_back(count);
		}
		start = end;
		end = curr.size();
	}
	// delete all curr branches with only 'empty' out
	int idx = 0;
	for (auto it = curr.begin(); it != curr.end();) {
		if (counts[idx] == (*it)->out.size())
			it = curr.erase(it);
		else it++;
		idx++;
	}

	if (!flag)
		return OUTPUT_NOTACCEPT;
	if (accept)
		return OUTPUT_ACCEPT;
	return OUTPUT_CONTINUE;
}

int StateMachine::addState(STATETYPE type) {
	state* s = new state;
	s->id = nextsid++;
	s->type = type;
	states[s->id] = s;
	if (type == STATE_START || type == STATE_BOTH) {
		if (!start) {
			start = s;
			startid = s->id;
			curr.push_back(s);
		}
		else {
			std::cout << "Start state has been set!" << std::endl;
			return -1;
		}
	}
	if (type == STATE_ACCEPT || type == STATE_BOTH)
		hasAccept = true;

	return s->id;
}

int StateMachine::addEdge(int from, int to, std::unordered_set<char> match) {
	if (states.count(from) == 0 || states.count(to) == 0) {
		std::cout << "Connected states donnot exist!" << std::endl;
		return -1;
	}
	edge* e = new edge;
	e->id = nexteid++;
	for (auto c : match)
		e->match.insert(c);
	state* sfrom = states[from];
	state* sto = states[to];
	sfrom->out.insert(e->id);
	sto->in.insert(e->id);
	e->from = from;
	e->to = to;
	// e->exclude = exclude;
	edges[e->id] = e;

	return e->id;
}

int StateMachine::addEdgeExclusive(int from, int to, std::unordered_set<char> match) {
	std::unordered_set<char> realmatch;
	for (int c = 0; c <= 127; c++) {
		if (match.count(c) == 0)
			realmatch.insert(c);
	}
	return addEdge(from, to, realmatch);
}

void StateMachine::deleteState(int id) {
	if (states.count(id)) {
		state* s = states[id];
		std::unordered_set<int> in = s->in;
		std::unordered_set<int> out = s->out;
		for (auto e : in) {
			deleteEdge(e);
		}
		for (auto e : out) {
			deleteEdge(e);
		}
		states.erase(id);
		delete s;
	}
}

void StateMachine::deleteEdge(int id) {
	if (edges.count(id)) {
		edge* e = edges[id];
		state* sf = states[e->from];
		state* st = states[e->to];
		// delete edge from s->in and s->out
		sf->out.erase(id);
		st->in.erase(id);
		delete e;
		edges.erase(id);
	}
}

feed_output StateMachine::feed(std::string input) {
	if (!legit) {
		std::cout << "State machine is still under construction!" << std::endl;
		return OUTPUT_ERROR;
	}
	char c;
	for (int i = 0; i < input.length();i++) {
		c = input[i];
		switch (feed(c, true)) {
		case OUTPUT_ACCEPT:
			if (i == input.length() - 1) {
				resetState();
				return OUTPUT_ACCEPT;
			}
		case OUTPUT_CONTINUE:
			break;
		case OUTPUT_NOTACCEPT:
			resetState();
			return OUTPUT_NOTACCEPT;
		case OUTPUT_ERROR:
			resetState();
			return OUTPUT_ERROR;
		}
	}

	resetState();
	return OUTPUT_NOTACCEPT;
}

feed_output StateMachine::feed(char input, bool greedy) {
	if (!legit) {
		std::cout << "State machine is still under construction!" << std::endl;
		resetState();
		return OUTPUT_ERROR;
	}

	int len = curr.size();
	bool flag = false;
	bool accept = false;
	for (int i = 0; i < len;i++) {
		auto c = curr[i];
		bool multi = false;
		for (auto& e : c->out) {
			if (edges[e]->match.count(input) >  0) {
				flag = true;
				state* tmps = states[edges[e]->to];
				if (!multi) {  
					curr[i] = states[edges[e]->to];
					multi = true;
				}
				else curr.push_back(states[edges[e]->to]); // create new curr branch
				//std::cout << "match " << curr->id  << " " << curr->type << std::endl;
				if (tmps->type == STATE_ACCEPT || tmps->type == STATE_BOTH) {
					//std::cout << "in accept" << std::endl;
					accept = true;
					if (!greedy) { // delay return to check empty edge
						resetState();
						return OUTPUT_ACCEPT;
					}
				}
			}
		}
	}

	if (flag) {
		switch (checkEmptyEdge()){
		case OUTPUT_ACCEPT:
			if (!greedy)
				resetState();
			return OUTPUT_ACCEPT;
		default:
			if (accept)
				return OUTPUT_ACCEPT;
			else return OUTPUT_CONTINUE;
		}
	}
	else {
		resetState();
		return OUTPUT_NOTACCEPT;
	}
}

std::vector<std::vector<int>> StateMachine::feed(std::ifstream & fin) {
	std::vector<std::vector<int>> ret;  // line number, line pos, total length
	if (!legit) {
		std::cout << "State machine is still under construction!" << std::endl;
		resetState();
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
				resetState();
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

	resetState();
	fin.clear();
	fin.seekg(0);
	return ret;
}

construct_output StateMachine::doneConstruct(bool extend) {
	if (!start || curr.size() == 0) {
		std::cout << "START state isn't specified!" << std::endl;
		legit = false;
		return ERROR_NOSTART;
	}
	if (!hasAccept) {
		std::cout << "ACCEPT state isn't specified!" << std::endl;
		legit = false;
		return ERROR_NOACCEPT;
	}

	for (auto s : states) {
		if (s.second->in.size() == 0) {
			if (s.second != start) {
				legit = false;
				return ERROR_NOACCEPT;
			}
		}
	}

	legit = true; 
	// extend is true: check if accept empty string (a.k.a start accept)
	if (extend && checkEmptyEdge() == OUTPUT_ACCEPT)
		startAccept = true;
	return CONSTRUCT_SUCCESS;
}

StateMachine::machineEntry StateMachine::constructFromTree(regexTreeNode* node) {
	machineEntry me(-1, -1);

	if (node->firstchild) {
		std::vector<machineEntry> childEntry;
		regexTreeNode* tmp = node->firstchild;
		while (tmp) {
			machineEntry me = constructFromTree(tmp);
			if (me.startid == -1 && me.endid == -1)
				return machineEntry(-1, -1);
			childEntry.push_back(me);
			// special duplication for repeat, TODO great performance problem
			if (node->type == NODETYPE_REPEAT) { // REPEAT has only one child
				int limit = node->infinite ? node->min : node->max;
				for (int i = 1; i < limit; i++) {
					me = constructFromTree(tmp);
					childEntry.push_back(me);
				}
				/*
				// one more for loop
				if (node->infinite) {
					me = constructFromTree(tmp);
					childEntry.push_back(me);
				} */
			}
			tmp = tmp->sibling;
		}
		// TODO construct with childs
		switch (node->type)
		{
		case NODETYPE_REPEAT:
			constructRepeat(me, childEntry, node->infinite, node->min, node->max);
			break;
		case NODETYPE_PARALLEL:
			constructParallel(me, childEntry);
			break;
		case NODETYPE_CASCADE:
			constructCascade(me, childEntry);
			break;
		default:
			break;
		}

		return me;
	}
	else {
		// construct leave nodes and leaves must be matches
		// bool exclude = false;
		std::unordered_set<char> vmatch;
		constructMatch(me, node->match, vmatch);

		return me;
	}
}

construct_output StateMachine::constructByRegex(std::string regex) {
	clear();

	regexTreeNode* head;
	if (parser.parseRegex(regex))
		head = parser.getTree();
	else return ERROR_PARSEERROR;
#ifdef FLOW_DEBUG
	visualizeParseTree();
#endif
	machineEntry entry = constructFromTree(head);
	if (entry.startid < 0 && entry.endid < 0) {
		clear();
		delete head;
		return ERROR_PARSEERROR;
	}
	hasAccept = true;
	states[entry.startid]->type = STATE_START;
	if (states[entry.endid]->type == STATE_START)	
		states[entry.endid]->type = STATE_BOTH;
	else states[entry.endid]->type = STATE_ACCEPT;
#ifdef eNFA_DEBUG
	std::cout << "start state: " << entry.startid << std::endl;
	std::cout << "end state: " << entry.endid << std::endl;
#endif
	start = states[entry.startid];
	startid = entry.startid;
	curr.push_back(start);
	if (doneConstruct(false) != CONSTRUCT_SUCCESS) {
		clear();
		delete head;
		return ERROR_PARSEERROR;
	}
#ifdef FLOW_DEBUG
	visualizeMachine("C:/Users/huanghaixin008/Desktop");
#endif
	// eliminate undeterministic
	// eNFA to NFA
	eliminateEmptyEdge(); 
	if (doneConstruct(false) != CONSTRUCT_SUCCESS) {
		clear();
		delete head;
		return ERROR_PARSEERROR;
	}
#ifdef FLOW_DEBUG
	visualizeMachine("C:/Users/huanghaixin008/Desktop");
#endif

	// NFA to DFA
	transformToDFA();
	if (doneConstruct() != CONSTRUCT_SUCCESS) {
		clear();
		delete head;
		return ERROR_PARSEERROR;
	}
#ifdef FLOW_DEBUG
	visualizeMachine("C:/Users/huanghaixin008/Desktop");
#endif
	
	return CONSTRUCT_SUCCESS;
}

construct_output StateMachine::constructMatch(machineEntry & me, std::string match, std::unordered_set<char> &vmatch) {
	// parse match string
	bool exclude = false;
#ifdef eNFA_DEBUG
	std::cout << "***constructMatch***" << std::endl;
#endif
	if (match.length() == 1) { // pure match
		if (match[0] == '.') {
			exclude = true;
			vmatch.insert('\n');
		}
		else vmatch.insert(match[0]);
	}
	else if (match[0] == '[') { // regional match
		int i = 1;
		if (match[1] == '^') {
			i = 2;
			exclude = true;
		}
		for (; i < match.length() - 1; i++) {
			if (match[i] == '-') {
				int leftbound = exclude ? 2 : 1;
				int rightbound = match.length() - 2;
				if (i <= leftbound || i >= rightbound) { // invalid '-'
					return ERROR_PARSEERROR;
				}
				char last = match[i - 1];
				char next = match[i + 1];
				if (next < last) { // invalid '-'
					return ERROR_PARSEERROR;
				}
				i++;
				// add regional match
				for (char c = last + 1; c < next; c++) { // last has been added and next will be added
					vmatch.insert(c);
				}
			}
			if (match[i] == '\\')
				continue;
			vmatch.insert(match[i]);
		}
	}
	else if (match[0] == '\\') { // meta word
		switch (match[1]) {
		case 'S':
		case 'B':
			exclude = true;
		case 's':
		case 'b':
			vmatch.insert('\f');
			vmatch.insert('\n');
			vmatch.insert('\r');
			vmatch.insert('\t');
			vmatch.insert('\v');
			break;
		case 'D':
			exclude = true;
		case 'd':
			for (char c = '0'; c <= '9'; c++)
				vmatch.insert(c);
			break;
		case 'W':
			exclude = true;
		case 'w':
			for (char c = '0'; c <= '9'; c++)
				vmatch.insert(c);
			for (char c = 'A'; c <= 'Z'; c++)
				vmatch.insert(c);
			for (char c = 'a'; c <= 'z'; c++)
				vmatch.insert(c);
			vmatch.insert('_');
			break;
		default:
			vmatch.insert(match[1]);
			break;
		}
	}

	me.startid = addState(STATE_INTER);
	me.endid = addState(STATE_INTER);
	if (exclude)
		addEdgeExclusive(me.startid, me.endid, vmatch);
	else addEdge(me.startid, me.endid, vmatch);
#ifdef eNFA_DEBUG
	std::cout << "addState: " << me.startid << std::endl;
	std::cout << "addState: " << me.endid << std::endl;
	std::cout << "addEdge from " << me.startid << " to " << me.endid;
	std::cout << " Exclude: " << exclude << ";;; Match: ";
	for (char c : vmatch)
		std::cout << c << ", ";
	std::cout << std::endl;
#endif
	return CONSTRUCT_SUCCESS;
}

construct_output StateMachine::constructRepeat(machineEntry & me, const std::vector<machineEntry>& childEntry, bool infinite, int min, int max) {
#ifdef eNFA_DEBUG
	std::cout << "***constructRepeat***" << std::endl;
#endif
	int size = childEntry.size();

	std::unordered_set<char> tmp;
	me.endid = addState(STATE_INTER);
#ifdef eNFA_DEBUG
	std::cout << "addState: " << me.endid << std::endl;
#endif
	// connect each child
	for (int i = 0; i < size - 1; i++) {
		addEdge(childEntry[i].endid, childEntry[i + 1].startid, tmp); // empty edge
#ifdef eNFA_DEBUG
		std::cout << "addEdge from " << childEntry[i].endid << " to " << childEntry[i + 1].startid;
		std::cout << ";;; Match: ";
		for (char c : tmp)
			std::cout << c << ", ";
		std::cout << std::endl;
#endif
	}

	if (infinite) {
		if (min == 0)
			me.startid = me.endid;
		else me.startid = childEntry[0].startid;
		addEdge(me.endid, childEntry[size - 1].startid, tmp);
#ifdef eNFA_DEBUG
		std::cout << "addEdge from " << me.endid << " to " << childEntry[size - 1].startid;
		std::cout << ";;; Match: ";
		for (char c : tmp)
			std::cout << c << ", ";
		std::cout << std::endl;
#endif
		addEdge(childEntry[size - 1].endid, me.endid, tmp);
#ifdef eNFA_DEBUG
		std::cout << "addEdge from " << childEntry[size - 1].endid << " to " << me.endid;
		std::cout << ";;; Match: ";
		for (char c : tmp)
			std::cout << c << ", ";
		std::cout << std::endl;
#endif
		return CONSTRUCT_SUCCESS;
	}
	else {
		me.startid = childEntry[0].startid;
		if (min == 0) {
			addEdge(me.startid, me.endid, tmp);
#ifdef eNFA_DEBUG
			std::cout << "addEdge from " << me.startid << " to " << me.endid;
			std::cout << ";;; Match: ";
			for (char c : tmp)
				std::cout << c << ", ";
			std::cout << std::endl;
#endif
		}
		for (int i = 1; i <= max; i++) {
			if (i >= min) {
				addEdge(childEntry[i - 1].endid, me.endid, tmp);
#ifdef eNFA_DEBUG
				std::cout << "addEdge from " << childEntry[i - 1].endid << " to " << me.endid;
				std::cout << ";;; Match: ";
				for (char c : tmp)
					std::cout << c << ", ";
				std::cout << std::endl;
#endif
			}
		}
		return CONSTRUCT_SUCCESS;
	}
}

construct_output StateMachine::constructParallel(machineEntry & me, const std::vector<machineEntry>& childEntry) {
#ifdef eNFA_DEBUG
	std::cout << "***constructParallel***" << std::endl;
#endif
	int size = childEntry.size();
	if (size == 1) {
		me = childEntry[0];
		return CONSTRUCT_SUCCESS;
	}
	me.startid = addState(STATE_INTER);
#ifdef eNFA_DEBUG
	std::cout << "addState: " << me.startid << std::endl;
#endif
	me.endid = addState(STATE_INTER);
#ifdef eNFA_DEBUG
	std::cout << "addState: " << me.endid << std::endl;
#endif
	std::unordered_set<char> tmp;
	for (int i = 0; i < size; i++) {
		addEdge(me.startid, childEntry[i].startid, tmp); // empty edge
#ifdef eNFA_DEBUG
		std::cout << "addEdge from " << me.startid << " to " << childEntry[i].startid;
		std::cout << ";;; Match: ";
		for (char c : tmp)
			std::cout << c << ", ";
		std::cout << std::endl;
#endif
		addEdge(childEntry[i].endid, me.endid, tmp);
#ifdef eNFA_DEBUG
		std::cout << "addEdge from " << childEntry[i].endid << " to " << me.endid;
		std::cout << ";;; Match: ";
		for (char c : tmp)
			std::cout << c << ", ";
		std::cout << std::endl;
#endif
	}

	return CONSTRUCT_SUCCESS;
}

construct_output StateMachine::constructCascade(machineEntry & me, const std::vector<machineEntry>& childEntry) {
#ifdef eNFA_DEBUG
	std::cout << "***constructCascade***" << std::endl;
#endif
	int size = childEntry.size();
	if (size == 1) {
		me = childEntry[0];
		return CONSTRUCT_SUCCESS;
	}
	me.startid = childEntry[0].startid;
	me.endid = childEntry[size - 1].endid;
	std::unordered_set<char> tmp;
	for (int i = 0; i < size - 1; i++) {
		addEdge(childEntry[i].endid, childEntry[i+1].startid, tmp); // empty edge
#ifdef eNFA_DEBUG
		std::cout << "addEdge from " << childEntry[i].endid << " to " << childEntry[i + 1].startid;
		std::cout << ";;; Match: ";
		for (char c : tmp)
			std::cout << c << ", ";
		std::cout << std::endl;
#endif
	}

	return CONSTRUCT_SUCCESS;
}

construct_output StateMachine::eliminateEmptyEdge() {
	if (!legit)
		return ERROR_PARSEERROR;

	// 1. find all valid states
	std::unordered_set<int> validstateid;
	validstateid.insert(startid);
	for (auto e : states) {
		if (e.second == start)
			continue;
		for (auto ine : e.second->in) {
			if (edges[ine]->match.size() != 0) { // has not-e in
				validstateid.insert(e.first);
				break;
			}
		}
	}
	// 2. find closure, link
	std::vector<int> nfrom;
	std::vector<int> nto;
	std::vector<std::unordered_set<char>> nmatch;
	std::unordered_set<int> closure;
	for (int sid : validstateid) {
		state* begin = states[sid]; 
		if (findClosure(closure, begin)) {
			if (begin->type == STATE_START || begin->type == STATE_BOTH)
				begin->type = STATE_BOTH;
			else begin->type = STATE_ACCEPT;
		}
		// connect outlinks from begin
		for (auto it = closure.begin(); it != closure.end(); it++) {
			for (auto e : states[*it]->out) {
				edge* ed = edges[e];
				if (ed->match.size() > 0 && closure.count(ed->to) == 0) {
					nfrom.push_back(sid);
					nto.push_back(ed->to);
					nmatch.push_back(ed->match);
				}
			}
		}
		closure.clear();
	}
	// add new edges
	for (int i = 0; i < nfrom.size(); i++) {
		// is there existing edges?
		bool flag = false;
		state* s = states[nfrom[i]];
		for (auto e : s->out) {
			if (edges[e]->to == nto[i]) {
				flag = true;
				for (auto c : nmatch[i]) {
					if (edges[e]->match.count(c) == 0)
						edges[e]->match.insert(c);
				}
			}
		}
		if (!flag)
			addEdge(nfrom[i], nto[i], nmatch[i]);
	}

	//  and 3. delete invalid things	
	for (auto it = states.begin(); it != states.end();) {
		if (validstateid.count(it->first) == 0) {
			auto tmp = it;
			it++;
			deleteState(tmp->first);
			continue;
		}
		it++;
	}	
	for (auto it = edges.begin(); it != edges.end();) {
		if (it->second->match.size() == 0) {
			auto tmp = it;
			it++;
			deleteEdge(tmp->first);
			continue;
		}
		it++;
	}

	return CONSTRUCT_SUCCESS;
}

bool StateMachine::findClosure(std::unordered_set<int>& closure, state* begin) {
	bool accept = false;
	for (auto e : begin->out) {
		if (edges[e]->match.size() == 0) {
			if (closure.count(edges[e]->to) == 0) {
				STATETYPE type = states[edges[e]->to]->type;
				if (type == STATE_ACCEPT || type == STATE_BOTH)
					accept = true;
				closure.insert(edges[e]->to);
				if (findClosure(closure, states[edges[e]->to]))
					accept = true;
			}
		}
	}
	return accept;
}

// TODO this function's like shit, may need optimization
construct_output StateMachine::transformToDFA() {
	if (!legit)
		return ERROR_PARSEERROR;

	typedef std::vector<int> STATE_SET;
	std::map<STATE_SET, int> DFAstatus; // states set, id in generated DFA
	std::set<STATE_SET> DFAaccept;
	std::map<std::pair<STATE_SET, STATE_SET>, int> DFAedge; // pair(from, to), idx
	std::vector<std::unordered_set<char>> DFAedgematch;
	// std::vector<bool> DFAedgeexclude;
	STATE_SET startstate = STATE_SET(1, start->id);
	if (start->type == STATE_BOTH)
		DFAaccept.insert(startstate);
	std::queue<STATE_SET> q;

	q.push(startstate);
	DFAstatus[startstate] = -1;
	while (!q.empty()) {
		STATE_SET tmpv = q.front();
		q.pop();

		// find all out matches from tmpv
		std::unordered_set<char> allmatch;
		// std::vector<std::unordered_set<char>> allmatchexc; // all match exclude
		for (auto s : tmpv) {
			for (auto e : states[s]->out) {
				for (auto c : edges[e]->match)
					allmatch.insert(c);
			}
		}
		// find all reachable states
		for (auto c : allmatch) {
			std::unordered_set<int> tmps;
			for (auto s : tmpv) {
				for (auto e : states[s]->out) {
					if (edges[e]->match.count(c) > 0)
						tmps.insert(edges[e]->to);
				}
			}
			if (tmps.size() == 0)
				continue;

			STATE_SET tstateset;
			bool accept = false;
			for (auto s : tmps) {
				if (states[s]->type == STATE_ACCEPT || states[s]->type == STATE_BOTH)
					accept = true;
				tstateset.push_back(s);
			}
			sort(tstateset.begin(), tstateset.end());
			if (accept)
				DFAaccept.insert(tstateset);

			// add new edge
			std::pair<STATE_SET, STATE_SET> tp = make_pair(tmpv, tstateset);
			if (DFAedge.count(tp) == 0) {
				DFAedge[tp] = DFAedgematch.size();
				DFAedgematch.push_back(std::unordered_set<char>({ c }));
				// DFAedgeexclude.push_back(false);
			}
			else {
				DFAedgematch[DFAedge[tp]].insert(c);
			}
			// add new state if not exist yet
			if (DFAstatus.count(tstateset) == 0) {
				q.push(tstateset);
				DFAstatus[tstateset] = -1;
			}
		}
	}

	// construct new DFA
	clear();
	for (auto it = DFAstatus.begin(); it != DFAstatus.end(); it++) {
		STATETYPE type = STATE_INTER;
		if (it->first == startstate)
			type = STATE_START;
		if (DFAaccept.count(it->first) > 0)
			if (type == STATE_START)
				type = STATE_BOTH;
			else type = STATE_ACCEPT;
		DFAstatus[it->first] = addState(type);
	}
	for (auto entry : DFAedge) {
		addEdge(DFAstatus[entry.first.first], DFAstatus[entry.first.second], DFAedgematch[entry.second]);
	}

	return CONSTRUCT_SUCCESS;
}

DFA StateMachine::generateDFA() {
	DFA dfa(states.size());
	std::unordered_map<int, int> oldid2newid;

	for (auto p : states) {
		oldid2newid[p.first] = dfa.addState(p.second->type);
	}
	for (auto p : edges) {
		auto match = p.second->match;
		int from = p.second->from;
		int to = p.second->to;
		for (auto c : match) {
			dfa.addTransition(oldid2newid[from], oldid2newid[to], c);
		}
	}

	dfa.doneConstruct();

	return dfa;
}

void StateMachine::visualizeMachine(std::string out) {
	using namespace std;

	if (out.length()) {
		std::string outstate = out + "/states.csv";
		std::string outedge = out + "/edges.csv";
		ofstream ofsstate(outstate);
		ofstream ofsedge(outedge);

		ofsstate << "id" << endl;
		for (auto s : states)
			ofsstate << s.first << endl;
		ofsedge << "Id,Source,Target,Label" << endl;
		for (auto e : edges) {
			ofsedge << e.first << "," << e.second->from << "," << e.second->to << ",";
			for (auto c : e.second->match) {
				ofsedge << c << ";";
			}
			ofsedge << endl;
		}
	}

	cout << "===== visualizeMachine =====" << endl;
	cout << "Start: " << start->id << endl;
	cout << "States: " << endl;
	for (auto s : states) {
		cout << "id: " << s.first;
		switch (s.second->type) {
		case STATE_ACCEPT:
			cout << ", ACCEPT";
			break;
		case STATE_START:
			cout << ", START";
			break;
		case STATE_BOTH:
			cout << ", BOTH";
			break;
		case STATE_INTER:
			cout << ", INTER";
			break;
		}
		cout << ";;" << endl;
		cout << "in: ";
		for (auto e : s.second->in)
			cout << e << ", ";
		cout << endl;
		cout << "out: ";
		for (auto e : s.second->out)
			cout << e << ", ";
		cout << endl;
	}
	cout << endl;

	cout << "Edges: " << endl;
	for (auto e : edges) {
		cout << "id: " << e.first;
		cout << " from: " << e.second->from << " to: " << e.second->to << endl;
		cout << "match: ";
		for (auto c : e.second->match) {
			cout << c << ", ";
		}
		cout << endl;
	}
	cout << "===== visualizeMachine End =====" << endl;
}

void StateMachine::clear() {
	start = NULL; nextsid = 0; nexteid = 0;  legit = false; hasAccept = false; startAccept = false;
	for (auto it = states.begin(); it != states.end();) {
		auto tmp = it;
		it++;
		deleteState(tmp->first);
	}	
	for (auto it = edges.begin(); it != edges.end();) {
		auto tmp = it;
		it++;
		deleteEdge(tmp->first);
	}
	curr.clear();
}