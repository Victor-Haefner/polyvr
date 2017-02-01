#include "Expression.h"
#include "core/utils/toString.h"
#include <stack>

using namespace OSG;

Expression::ValueBase::~ValueBase() {}

template<typename T>
Expression::Value<T>::Value(const T& t) : value(t) {}

template<typename T>
Expression::Value<T>::~Value() {}

template<typename T>
string Expression::Value<T>::toString() { return ::toString(value); }

template<typename T>
Expression::ValueBase* Expression::Value<T>::add(Expression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value + v2->value);
    return 0;
}

template<typename T>
Expression::ValueBase* Expression::Value<T>::sub(Expression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value - v2->value);
    return 0;
}

template<typename T>
Expression::ValueBase* Expression::Value<T>::mult(Expression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value * v2->value);
    return 0;
}

template<typename T>
Expression::ValueBase* Expression::Value<T>::div(Expression::ValueBase* n) {
    //if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value / v2->value);
    return 0;
}



Expression::Node::Node(string s) : param(s) {;}
Expression::Node::~Node() { if (value) delete value; }

void Expression::Node::setValue(float f) { value = new Value<float>(f); }
void Expression::Node::setValue(Vec3f v) { value = new Value<Vec3f>(v); }
void Expression::Node::setValue(string s) {
    cout << " Node::setValue " << param << " " << s << endl;
    int N = std::count(s.begin(), s.end(), ' ');
    if (N == 0) setValue(toFloat(s));
    if (N == 2) setValue(toVec3f(s));
}

string Expression::Node::toString(string indent) {
    string res = param;
    if (value) res += " ("+value->toString()+")";
    if (left) res += "\n"+indent+" " + left->toString(indent+" ");
    if (right) res += "\n"+indent+" " + right->toString(indent+" ");
    return res;
}

void Expression::Node::compute() { // compute value based on left and right values and param as operator
    if (left->value == 0 || right->value == 0) return;
    char op = param[0];
    if (op == '+') value = left->value->add(right->value);
    if (op == '-') value = left->value->sub(right->value);
    if (op == '*') value = left->value->mult(right->value);
    if (op == '/') value = left->value->div(right->value);
}


bool Expression::isMathToken(char c) {
    if (c == '+' || c == '-' || c == '*' || c == '/') return true;
    if (c == '(' || c == ')') return true;
    return false;
}

void Expression::convToPrefixExpr() { // convert infix to prefix expression
    vector<string> tokens;

    // split into tokens
    string last;
    for (int i=0; i<data.size(); i++) {
        char c = data[i];
        if (isMathToken(c)) {
            if (last.size() > 0 ) tokens.push_back(last);
            last = "";
            string t; t+=c;
            tokens.push_back(t);
        } else last += c;
    }
    if (last.size() > 0 ) tokens.push_back(last);

    stack<string> OperandStack;
    stack<char> OperatorStack;

    auto processTriple = [&]() {
        char Operator = OperatorStack.top(); OperatorStack.pop();
        auto RightOperand = OperandStack.top(); OperandStack.pop();
        auto LeftOperand = OperandStack.top(); OperandStack.pop();
        string op; op += Operator;
        string tmp = op +" "+ LeftOperand +" "+ RightOperand;
        OperandStack.push( tmp );
    };

    for (auto t : tokens) {
        if (t.size() != 1 || !isMathToken(t[0]) ) {
            OperandStack.push(t); continue;
        }
        char o = t[0];

        if ( o == '(' || OperatorStack.size() == 0 || OperatorHierarchy[o] < OperatorHierarchy[OperatorStack.top()] ) {
            OperatorStack.push(o); continue;
        }

        if ( o == ')' ) {
            while( OperatorStack.top() != '(' ) processTriple();
            o = OperatorStack.top(); OperatorStack.pop();
            continue;
        }

        if ( OperatorHierarchy[o] >= OperatorHierarchy[OperatorStack.top()] ) {
            while( OperatorStack.size() != 0 and OperatorHierarchy[o] >= OperatorHierarchy[OperatorStack.top()] ) {
                processTriple();
            }
            OperatorStack.push(o);
        }
    }

    while( OperatorStack.size() ) processTriple();
    data = OperandStack.top(); // store prefix expression
}

void Expression::buildTree() { // build a binary expression tree from the prefix expression data
    stack<Node*> nodeStack;
    Node* node = 0;
    vector<string> tokens = splitString(data,' ');
    for (int i=0; i<tokens.size(); i++) {
        string t = tokens[tokens.size()-i-1];
        node = new Node(t);
        nodes.push_back(node);
        if ( t.size() == 1 && isMathToken(t[0]) ) { // found operator
            node->right = nodeStack.top(); nodeStack.pop();
            node->left = nodeStack.top(); nodeStack.pop();
            nodeStack.push(node);
        } else nodeStack.push(node);
    }
    node = nodeStack.top(); nodeStack.pop();
    tree = node;
    //cout << tree->toString() << endl;
}

Expression::Expression(string s) {
    data = s;

    OperatorHierarchy['+'] = 6;
    OperatorHierarchy['-'] = 6;
    OperatorHierarchy['*'] = 5;
    OperatorHierarchy['/'] = 5;
    OperatorHierarchy['('] = 2;
    OperatorHierarchy[')'] = 2;
}

Expression::~Expression() {}

bool Expression::isMathExpression() {
    for (auto c : data) if (isMathToken(c)) return true;
    return false;
}

void Expression::computeTree() {
    convToPrefixExpr();
    buildTree();
}

vector<Expression::Node*> Expression::getLeafs() {
    vector<Node*> res;
    for (auto n : nodes) if (!n->left && !n->right) res.push_back(n);
    return res;
}

string Expression::compute() { // compute result of binary expression tree
    std::function<void(Node*)> subCompute = [&](Node* n) {
        if (!n || !n->left || !n->right) return;
        subCompute(n->left);
        subCompute(n->right);
        n->compute();
    };
    subCompute(tree);
    if (tree->value) return tree->value->toString();
    else return "";
}






