#include "Expression.h"
#include "core/utils/toString.h"
#include <stack>

namespace OSG {
    template<> Expression::ValueBase* Expression::Value<Vec3d>::compL(Expression::ValueBase* n) { return 0; }
    template<> Expression::ValueBase* Expression::Value<Vec3d>::compLE(Expression::ValueBase* n) { return 0; }
    template<> Expression::ValueBase* Expression::Value<Vec3d>::compG(Expression::ValueBase* n) { return 0; }
    template<> Expression::ValueBase* Expression::Value<Vec3d>::compGE(Expression::ValueBase* n) { return 0; }
}

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


template<typename T>
Expression::ValueBase* Expression::Value<T>::compE(Expression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value == v2->value);
    return 0;
}

template<typename T>
Expression::ValueBase* Expression::Value<T>::compL(Expression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value < v2->value);
    return 0;
}

template<typename T>
Expression::ValueBase* Expression::Value<T>::compLE(Expression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value <= v2->value);
    return 0;
}

template<typename T>
Expression::ValueBase* Expression::Value<T>::compG(Expression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value > v2->value);
    return 0;
}

template<typename T>
Expression::ValueBase* Expression::Value<T>::compGE(Expression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value >= v2->value);
    return 0;
}



Expression::Node::Node(string s) : param(s) {;}
Expression::Node::~Node() { if (value) delete value; }

void Expression::Node::setValue(float f) { value = new Value<float>(f); }
void Expression::Node::setValue(Vec3d v) { value = new Value<Vec3d>(v); }

void Expression::Node::setValue(string s) {
    int N = std::count(s.begin(), s.end(), ',');
    cout << "Expression::Node::setValue " << s << "  " << N << endl;
    if (N == 0) setValue(toFloat(s));
    if (N == 2) {
        setValue(toValue<Vec3d>(s));
    }
}

string Expression::Node::toString() {
    string res = value ? value->toString() : param;
    if (right) res = right->toString() + res;
    if (left) res += left->toString();
    return res;
}

string Expression::Node::treeToString(string indent) {
    string res = param;
    if (value) res += " ("+value->toString()+")";
    if (left) res += "\n"+indent+" " + left->treeToString(indent+" ");
    if (right) res += "\n"+indent+" " + right->treeToString(indent+" ");
    return res;
}

void Expression::Node::compute() { // compute value based on left and right values and param as operator
    if (!left || !right) return;
    if (left->value == 0 || right->value == 0) return;
    char op = param[0];
    if (op == '+') value = left->value->add(right->value);
    if (op == '-') value = left->value->sub(right->value);
    if (op == '*') value = left->value->mult(right->value);
    if (op == '/') value = left->value->div(right->value);
    if (op == '=') value = left->value->compE(right->value);
    if (op == '<') value = left->value->compL(right->value);
    //if (op == '<') value = left->value->compLE(right->value); // TODO
    if (op == '>') value = left->value->compG(right->value);
    //if (op == '>') value = left->value->compGE(right->value); // TODO
}


bool Expression::isMathToken(char c) {
    if (c == '+' || c == '-' || c == '*' || c == '/') return true;
    if (c == '(' || c == ')') return true;
    if (c == '<' || c == '>'|| c == '=') return true;
    return false;
}

void Expression::convToPrefixExpr() { // convert infix to prefix expression
    vector<string> tokens;

    // split into tokens
    string last;
    for (uint i=0; i<data.size(); i++) {
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
    prefixExpr = true;
}

void Expression::buildTree() { // build a binary expression tree from the prefix expression data
    if (!prefixExpr) return;
    stack<Node*> nodeStack;
    Node* node = 0;

    /*vector<string> tokens; // split string in tokens
    string token;
    for (auto c : data) {
        if (isMathToken(c)) {
            tokens.push_back(token);
            tokens.push_back(string()+c);
            token = "";
        } else token += c;
    }
    if (token.size()) tokens.push_back(token);*/

    vector<string> tokens = splitString(data, ' '); // prefix expression is delimited by spaces!

    for (uint i=0; i<tokens.size(); i++) {
        string t = tokens[tokens.size()-i-1];
        node = new Node(t);
        nodes.push_back(node);
        if ( t.size() == 1 && isMathToken(t[0]) ) { // found operator
            node->right = nodeStack.top(); nodeStack.pop();
            node->left = nodeStack.top(); nodeStack.pop();
            nodeStack.push(node);
        } else nodeStack.push(node);
    }
    tree = nodeStack.top(); nodeStack.pop();

    for (auto l : getLeafs()) l->setValue(l->param);
    //cout << "expression tree:\n" << tree->treeToString() << " " << tree->parent << endl;
}

Expression::Expression(string s) {
    set(s);

    OperatorHierarchy['+'] = 6;
    OperatorHierarchy['-'] = 6;
    OperatorHierarchy['*'] = 5;
    OperatorHierarchy['/'] = 5;
    OperatorHierarchy['('] = 2;
    OperatorHierarchy[')'] = 2;
    OperatorHierarchy['<'] = 1;
    OperatorHierarchy['>'] = 1;
    OperatorHierarchy['='] = 1;
}

Expression::~Expression() {}

ExpressionPtr Expression::create() { return ExpressionPtr( new Expression("") ); }

bool Expression::isMathExpression() {
    for (auto c : data) if (isMathToken(c)) return true;
    return false;
}

void Expression::makeTree() {
    convToPrefixExpr();
    buildTree();
}

vector<Expression::Node*> Expression::getLeafs() {
    vector<Node*> res;
    for (auto n : nodes) if (!n->left && !n->right) res.push_back(n);
    return res;
}

void Expression::set(string s) {
    data = s;
    data.erase(std::remove(data.begin(), data.end(), ' '), data.end());
    data.erase(std::remove(data.begin(), data.end(), '\t'), data.end());
    data.erase(std::remove(data.begin(), data.end(), '\n'), data.end());
}

string Expression::computeTree() { // compute result of binary expression tree
    std::function<void(Node*)> subCompute = [&](Node* n) {
        if (!n) return;
        subCompute(n->left);
        subCompute(n->right);
        n->compute();
    };
    subCompute(tree);
    if (!tree || !tree->value) return "";
    return tree->value->toString();
}

string Expression::compute() {
    makeTree();
    if (tree) cout << tree->treeToString() << endl;
    return computeTree();
}

string Expression::toString() { return tree->toString(); }





