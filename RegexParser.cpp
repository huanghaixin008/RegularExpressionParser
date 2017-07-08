# include "RegexParser.h"
# include <iostream>

// # define TREE_DEBUG

void RegexParser::trim(std::string& regex) {
	int start = 0;
	int end = regex.size()-1;

	for (; start < regex.length(); start++)
		if (start != ' ')
			break;	
	for (; end >= 0; end--)
		if (end != ' ')
			break;

	regex = regex.substr(start, end-start+1);
};

void RegexParser::deleteTree(regexTreeNode* node) {
	if (node == NULL)
		return;
	regexTreeNode* first = node->firstchild;
	while (first) {
		regexTreeNode* tmp = first->sibling;
		deleteTree(first);
		first = tmp;
	}
	delete node;
};

void RegexParser::deleteInterNodes(std::stack<regexTreeNode*>& nodewait) {
	while (nodewait.size()) {
		regexTreeNode* node = nodewait.top();
		nodewait.pop();
		deleteTree(node);
	}
}

regexTreeNode* RegexParser::allocateTreeNodeMatch(std::string match) {
	regexTreeNode* ret = new regexTreeNode;
	ret->type = NODETYPE_MATCH;
	ret->parent = NULL;
	ret->firstchild = NULL;
	ret->sibling = NULL;
	ret->match = match;

	return ret;
};

regexTreeNode* RegexParser::allocateTreeNodeRepeat(int min, int max, bool infinite) {
	regexTreeNode* ret = new regexTreeNode;
	ret->type = NODETYPE_REPEAT;
	ret->parent = NULL;
	ret->firstchild = NULL;
	ret->sibling = NULL;
	ret->min = min;
	ret->max = max;
	ret->infinite = infinite;

	return ret;
};

regexTreeNode* RegexParser::allocateTreeNodeOprand(NODETYPE type) {
	regexTreeNode* ret = new regexTreeNode;
	ret->type = type;
	ret->parent = NULL;
	ret->firstchild = NULL;
	ret->sibling = NULL;
	return ret;
};

bool RegexParser::parseRegex(std::string regex) {
	std::stack<regexTreeNode*> nodewait;
	std::stack<char> opwait;

	trim(regex);
	int len = regex.length();
	bool last = false; // indicates that if the last checked element is a cascadeable element

	for (int i = 0; i < len; i++) {
		if (regex[i] == '?' || regex[i] == '*'
			|| regex[i] == '+' || regex[i] == '{') { // repeat
			int min, max;
			bool infinite = false;
			switch (regex[i]) {
			case '?':
				min = 0;
				max = 1;
				break;
			case '*':
				min = 0;
				max = INT_MAX;
				infinite = true;
				break;
			case '+':
				min = 1;
				max = INT_MAX;
				infinite = true;
				break;
			case '{':
				int j = i + 1;
				bool closed = false;
				bool front = true;
				std::string s1 = "";
				std::string s2 = "";
				while (j < len) {
					if (regex[j] == '}') {
						closed = true;
						i = j;
						break;
					}
					if (regex[j] == ',') {
						front = false;
						j++;
						continue;
					}
					if (front)
						s1 += regex[j];
					else s2 += regex[j];
					j++;
				}
				if (!closed) {
					std::cout << "ERROR1 in parsing: Unclosed bracket for repeating!" << std::endl; 
					deleteInterNodes(nodewait);
					return false;
				}
				min = atoi(s1.c_str());
				if (!front && s2 == "") {
					max = INT_MAX;
					infinite = true;
				}
				else if (front && s2 == "")
					max = min;
				else {
					max = atoi(s2.c_str());
					if (max < min) {
						std::cout << "ERROR2 in parsing: Invalid value for bracket repeating!" << std::endl;
						deleteInterNodes(nodewait);
						return false;
					}
				}
			}
			if (nodewait.size() == 0) {
				std::cout << "ERROR3 in parsing: Invalid repeating operator!" << std::endl;
				deleteInterNodes(nodewait);
				return false;
			}
			regexTreeNode* tnode = allocateTreeNodeRepeat(min, max, infinite);
			regexTreeNode* op = nodewait.top();
			nodewait.pop();
			tnode->firstchild = op;
			op->parent = tnode;
			nodewait.push(tnode);
			last = true;
		}
		else if (regex[i] == '(') {// considering invisible '&'
			if (last) {
				/*
				if (!popPreviousOp(nodewait, opwait, '&')) {
					std::cout << "ERROR7 in parsing: Invalid expression!" << std::endl;
					deleteInterNodes(nodewait);
					return false;
				}
				*/
#ifdef TREE_DEBUG
				std::cout << "Pushing op: &" << std::endl;
#endif
				opwait.push('&');
				last = false;
			}
#ifdef TREE_DEBUG
			std::cout << "Pushing op: (" << std::endl;
#endif
			opwait.push('(');
		}
		else if (regex[i] == ')') {
#ifdef TREE_DEBUG
			std::cout << "Encounter )" << std::endl;
#endif
			last = true;
			if (!popPreviousOp(nodewait, opwait, '&')) { // pop previous '&'
				std::cout << "ERROR5 in parsing: Invalid expression!" << std::endl;
				deleteInterNodes(nodewait);
				return false;
			}
			if (!popPreviousOp(nodewait, opwait, '|')) { // pop previous '|'
				std::cout << "ERROR5 in parsing: Invalid expression!" << std::endl;
				deleteInterNodes(nodewait);
				return false;
			}
			if (!opwait.size() || opwait.top() != '(') {
				std::cout << "ERROR5 in parsing: Invalid expression!" << std::endl;
				deleteInterNodes(nodewait);
				return false;
			} else opwait.pop();
		}
		else if (regex[i] == '|') { // PARALLEL ops
			if (!popPreviousOp(nodewait, opwait, '&')) { // pop remaining '&'
				std::cout << "ERROR6 in parsing: Invalid expression!" << std::endl;
				deleteInterNodes(nodewait);
				return false;
			}
#ifdef TREE_DEBUG
			std::cout << "Pushing op: |" << std::endl;
#endif
			opwait.push('|');
			last = false;
		}
		else { // match symbol
			// considering invisible '&'
			if (last) {
				/*
				if (!popPreviousOp(nodewait, opwait, '&')) {
					std::cout << "ERROR7 in parsing: Invalid expression!" << std::endl;
					deleteInterNodes(nodewait);
					return false;
				}
				*/
#ifdef TREE_DEBUG
				std::cout << "Pushing op: &"  << std::endl;
#endif
				opwait.push('&');
				last = false;
			}

			last = true;
			std::string tmp = "";
			if (regex[i] == '[') { // region match
				int j = i;
				while (true) {
					if (j == len) {
						std::cout << "ERROR8 in parsing: Unclosed bracket for regional matching!" << std::endl;
						deleteInterNodes(nodewait);
						return false;
					}
					tmp += regex[j];
					if (regex[j] == ']') {
						i = j;
						break;
					}
					j++;
				}
			}
			else if (regex[i] == '\\') {  // escape character
				if (i + 1 == len) {
					std::cout << "ERROR9 in parsing: Invalid escape character!" << std::endl;
					deleteInterNodes(nodewait);
					return false;
				}
				if ((/*regex[i + 1] == 'f' || regex[i + 1] == 'n' || regex[i + 1] == 'r' || || regex[i + 1] == 't' || regex[i + 1] == 'v' */
					regex[i + 1] == 's' || regex[i + 1] == 'S'  || regex[i + 1] == 'b' ||
					regex[i + 1] == 'B' || regex[i + 1] == 'd' || regex[i + 1] == 'D' || regex[i + 1] == 'w' ||
					regex[i + 1] == 'W')) // meta character
					tmp += regex[i];
				tmp += regex[i + 1];
				i++;
			}
			else tmp += regex[i];
			// construct a matching tree node from tmp, and push into stack
#ifdef TREE_DEBUG
			std::cout << "Pushing matching: " << tmp << std::endl;
#endif
			nodewait.push(allocateTreeNodeMatch(tmp));
		}
	}

	if (!popPreviousOp(nodewait, opwait, '&')) {  // pop remaining '&'
		std::cout << "ERROR10 in parsing: Invalid expression!" << std::endl;
		return false;
	}	
	if (!popPreviousOp(nodewait, opwait, '|')) { // pop remaining '|'
		std::cout << "ERROR10 in parsing: Invalid expression!" << std::endl;
		return false;
	}

	if (nodewait.size() != 1) {
		std::cout << "ERROR11 in parsing: Invalid expression!" << std::endl;
		deleteInterNodes(nodewait);
		return false;
	}

	deleteTree(head);
	head = nodewait.top();
	return true;
}

bool RegexParser::popPreviousOp(std::stack<regexTreeNode*>& nodewait, std::stack<char>& opwait, char oper) {
	if (opwait.size() == 0)
		return true;

	NODETYPE type;
	switch (oper) {
	case '|':
		type = NODETYPE_PARALLEL;
		break;
	case '&':
		type = NODETYPE_CASCADE;
		break;
	}

	int opcount = 0;
	char op;
	do {
		op = opwait.top();
		opwait.pop();
		if (op != oper) { // meet unexpected operator
			opwait.push(op);
			break;
		}
		opcount++;
	} while (opwait.size());
	if (opcount == 0)
		return true;

	int nodecount = 0;
	std::vector<regexTreeNode*> tmpv;
	while (nodecount < opcount + 1 && nodewait.size()) {
		tmpv.insert(tmpv.begin(), nodewait.top());
		nodewait.pop();
		nodecount++;
	}
	if (nodecount < opcount + 1) {
		for (auto node : tmpv)  // for deleting intermediate nodes
			nodewait.push(node);
		return false;
	}
#ifdef TREE_DEBUG
	std::cout << "Poping op begin: " << oper << " opcount: " << opcount << " nodecount: " << nodecount << " nodesize: " << nodewait.size() << std::endl;
#endif
	regexTreeNode* node = allocateTreeNodeOprand(type);
	for (int i = 0; i < nodecount - 1;i++)
		tmpv[i]->sibling = tmpv[i + 1];
	tmpv[nodecount - 1]->sibling = NULL;
	node->firstchild = tmpv[0];
	nodewait.push(node);
#ifdef TREE_DEBUG
	std::cout << "Poping op done: " << oper  << " nodesize: " << nodewait.size() << std::endl;
#endif

	return true;
}

void RegexParser::visualize() {
	using namespace std;
	cout << "===== visualizeParseTree =====" << endl;
	if (head == NULL) {
		cout << "EMPTY TREE" << endl;
		return;
	}

	vector<regexTreeNode*> buffer(1, head);
	vector<regexTreeNode*> buffer2;
	bool exchange = true;
	vector<regexTreeNode*>* bufferExtract = &buffer;
	vector<regexTreeNode*>* bufferPut = &buffer2;

	int level = 0;
	while (bufferExtract->size()) {
		cout << level << ": ";
		level++;
		for (auto node : *bufferExtract) {
			int count_child = 0;
			regexTreeNode* first = node->firstchild;
			while (first) {
				regexTreeNode* tmp = first->sibling;
				bufferPut->push_back(first);
				first = tmp;
				count_child++;
			}
			cout << "(" << node->type << ", " << count_child;
			if (node->type == NODETYPE_MATCH)
				cout << ", " << node->match;
			cout << ") ";
		}
		cout << endl;
		bufferExtract->clear();
		// exchange Extract and Put
		if (exchange) {
			bufferExtract = &buffer2;
			bufferPut = &buffer;
		}
		else {
			bufferExtract = &buffer;
			bufferPut = &buffer2;
		}
		exchange = !exchange;
	}
}