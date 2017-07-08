# include <vector>
# include <stack>
# include <string>

enum NODETYPE {
	NODETYPE_UNDEFINED=100,
	NODETYPE_MATCH,
	NODETYPE_CASCADE,
	NODETYPE_PARALLEL,
	NODETYPE_REPEAT
};
/*
#define NODETYPE_UNDEFINED -111
#define NODETYPE_MATCH 111
#define NODETYPE_CASCADE 222
#define NODETYPE_PARALLEL 333
#define NODETYPE_REPEAT 444
*/

struct regexTreeNode {
	NODETYPE type;
	regexTreeNode* firstchild;
	regexTreeNode* sibling;
	regexTreeNode* parent;
	int min, max;
	bool infinite;
	std::string match;
};
// parse a regex string into a regex element tree
class RegexParser {
private:
	regexTreeNode* head;

	inline int opPriority(char op) {
		switch (op) {
		case '@':
			return -1;
		case '(':
			return 0;
		case ')':
			return 0;
		case '|':
			return 1;
		case '&':
			return 2;
		}
	};
	void trim(std::string& regex);
	regexTreeNode* allocateTreeNodeMatch(std::string match);
	regexTreeNode* allocateTreeNodeRepeat(int min, int max, bool infinite);
	regexTreeNode* allocateTreeNodeOprand(NODETYPE type);

	bool popPreviousOp(std::stack<regexTreeNode*>& nodewait, std::stack<char>& opwait, char oper);
	void deleteTree(regexTreeNode* node);
	void deleteInterNodes(std::stack<regexTreeNode*>& nodewait);
public:
	RegexParser() { head = NULL; };
	RegexParser(std::string regex) { parseRegex(regex); };
	~RegexParser() {
		deleteTree(head);
	};
	bool parseRegex(std::string regex);
	regexTreeNode* getTree() { return head; };
	void visualize();
};

