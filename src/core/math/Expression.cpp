#include "Expression.h"
#include "core/utils/toString.h"
#include <stack>

namespace OSG {
    template<> MathExpression::ValueBase* MathExpression::Value<Vec3d>::compL(MathExpression::ValueBase* n) { return 0; }
    template<> MathExpression::ValueBase* MathExpression::Value<Vec3d>::compLE(MathExpression::ValueBase* n) { return 0; }
    template<> MathExpression::ValueBase* MathExpression::Value<Vec3d>::compG(MathExpression::ValueBase* n) { return 0; }
    template<> MathExpression::ValueBase* MathExpression::Value<Vec3d>::compGE(MathExpression::ValueBase* n) { return 0; }
}

using namespace OSG;

MathExpression::ValueBase::~ValueBase() {}

template<typename T>
MathExpression::Value<T>::Value(const T& t) : value(t) {}

template<typename T>
MathExpression::Value<T>::~Value() {}

template<typename T>
string MathExpression::Value<T>::toString() { return ::toString(value); }

namespace OSG {
    template<> string MathExpression::Value<Vec3d>::toString() { return "[" + ::toString(value[0]) + "," + ::toString(value[1]) + "," + ::toString(value[2]) + "]"; }
}

template<typename T>
MathExpression::ValueBase* MathExpression::Value<T>::add(MathExpression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value + v2->value);
    return 0;
}

template<typename T>
MathExpression::ValueBase* MathExpression::Value<T>::sub(MathExpression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value - v2->value);
    return 0;
}

template<typename T>
MathExpression::ValueBase* MathExpression::Value<T>::mult(MathExpression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value * v2->value);
    return 0;
}

template<typename T>
MathExpression::ValueBase* MathExpression::Value<T>::div(MathExpression::ValueBase* n) {
    //if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value / v2->value);
    return 0;
}


template<typename T>
MathExpression::ValueBase* MathExpression::Value<T>::compE(MathExpression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value == v2->value);
    return 0;
}

template<typename T>
MathExpression::ValueBase* MathExpression::Value<T>::compL(MathExpression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value < v2->value);
    return 0;
}

template<typename T>
MathExpression::ValueBase* MathExpression::Value<T>::compLE(MathExpression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value <= v2->value);
    return 0;
}

template<typename T>
MathExpression::ValueBase* MathExpression::Value<T>::compG(MathExpression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value > v2->value);
    return 0;
}

template<typename T>
MathExpression::ValueBase* MathExpression::Value<T>::compGE(MathExpression::ValueBase* n) {
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value >= v2->value);
    return 0;
}

// vector only

template<typename T> MathExpression::ValueBase* MathExpression::Value<T>::cross(MathExpression::ValueBase* n) { return 0; }
template<typename T> MathExpression::ValueBase* MathExpression::Value<T>::dot(MathExpression::ValueBase* n) { return 0; }

namespace OSG {
    template<> MathExpression::ValueBase* MathExpression::Value<Vec3d>::cross(MathExpression::ValueBase* n) {
        if (auto v2 = dynamic_cast<Value<Vec3d>*>(n)) return new Value<Vec3d>(value.cross(v2->value));
        return 0;
    }

    template<> MathExpression::ValueBase* MathExpression::Value<Vec3d>::dot(MathExpression::ValueBase* n) {
        if (auto v2 = dynamic_cast<Value<Vec3d>*>(n)) return new Value<float>(value.dot(v2->value));
        return 0;
    }
}



MathExpression::Node::Node(string s) : param(s) {;}
MathExpression::Node::~Node() { if (value) delete value; }

void MathExpression::Node::setValue(float f) { value = new Value<float>(f); }
void MathExpression::Node::setValue(Vec3d v) { value = new Value<Vec3d>(v); }

void MathExpression::Node::setValue(string s) {
    int N = std::count(s.begin(), s.end(), ',');
    if (N == 0) {
        float f;
        bool b = toValue(s,f);
        if (b) setValue(f);
    }
    if (N == 2) {
        Vec3d f;
        bool b = toValue(s,f);
        if (b) setValue(f);
    }
}

string MathExpression::Node::toString() {
    string res = value ? value->toString() : param;
    if (isMathFunction(res) && left && right) {
        res += "{" + left->toString() + "," + right->toString() + "}";
    } else {
        if (left) res = left->toString() + res;
        if (right) res += right->toString();
    }
    return res;
}

string MathExpression::Node::toString2() {
    string res = param + "(" + (value ? value->toString() : "") + ")";
    if (right) res = right->toString() + res;
    if (left) res += left->toString();
    return res;
}

string MathExpression::Node::treeToString(string indent) {
    string res = param;
    if (value) res += " ("+value->toString()+")";
    if (left) res += "\n"+indent+" " + left->treeToString(indent+" ");
    if (right) res += "\n"+indent+" " + right->treeToString(indent+" ");
    return res;
}

void MathExpression::Node::compute() { // compute value based on left and right values and param as operator
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
    if (param == "cross") value = left->value->cross(right->value);
    if (param == "dot") value = left->value->dot(right->value);
}


bool MathExpression::isMathToken(char c) {
    if (c == '+' || c == '-' || c == '*' || c == '/') return true;
    if (c == '(' || c == ')') return true;
    if (c == '[' || c == ']') return true; // delimits vector
    if (c == '{' || c == '}') return true; // delimits function arguments
    if (c == '<' || c == '>'|| c == '=') return true;
    return false;
}

bool MathExpression::isMathFunction(string f) {
    if (f == "cos") return true;
    if (f == "sin") return true;
    if (f == "cross") return true;
    if (f == "length") return true;
    if (f == "dot") return true;
    return false;
}

void MathExpression::convToPrefixExpr() { // convert infix to prefix expression
    vector<string> tokens;

    // split into tokens
    string last;
    bool inVector = false;
    for (uint i=0; i<data.size(); i++) {
        char c = data[i];
        if (isMathToken(c) || (c == ',' && !inVector)) {
            if (c == '[') inVector = true;
            if (c == ']') inVector = false;
            if (last.size() > 0 ) tokens.push_back(last);
            last = "";
            string t; t+=c;
            tokens.push_back(t);
        } else last += c;
    }
    if (last.size() > 0 ) tokens.push_back(last);

    stack<string> OperandStack;
    stack<string> OperatorStack;

    auto topOperator = [&](bool pop = 0) -> string {
        if (OperatorStack.size()) {
            auto t = OperatorStack.top();
            if (pop) OperatorStack.pop();
            return t;
        }
        return "";
    };

    auto processTriple = [&]() {
        if (OperandStack.size() > 1) {
            string Operator = topOperator(1);
            string RightOperand = OperandStack.top(); OperandStack.pop();
            string LeftOperand = OperandStack.top(); OperandStack.pop();
            string op; op += Operator;
            string tmp = op +" "+ LeftOperand +" "+ RightOperand;
            OperandStack.push( tmp );
        }
    };

    auto isSecondaryToken = [&](string t) {
        if (t == ",") return true;
        return false;
    };

    auto checkCharacter = [&](string c) {
        auto t = topOperator();
        if (t == "") return false; // something went wrong!
        return bool(topOperator() != c);
    };

    auto operatorsToStr = [&]() {
        auto stackCopy = OperatorStack;
        string s;
        while (!stackCopy.empty( ) ) {
            s = stackCopy.top() + " " + s;
            stackCopy.pop( );
        }
        return s;
    };

    //cout << "MathExpression::convToPrefixExpr data: " << data << endl;
    for (auto t : tokens) {
        //cout << " token " << t << "   \toperators: " << operatorsToStr() << endl;
        if (isMathFunction(t)) { OperatorStack.push(t); continue; }

        if (!isSecondaryToken(t)) {
            if ( t.size() != 1 || !isMathToken(t[0]) ) {
                OperandStack.push(t); continue;
            }
        }

        if ( t == "[" || t == "]" ) continue;

        if ( t == "," ) { // single comma delimits function parameters, acts like "}{"
            while( checkCharacter("{") ) processTriple();
            t = topOperator(1);
            OperatorStack.push("{");
            continue;
        }

        if ( t == "{" || t == "(" || OperatorStack.size() == 0 || OperatorHierarchy[t] < OperatorHierarchy[topOperator()] ) {
            OperatorStack.push(t); continue;
        }

        if ( t == ")" ) {
            while( checkCharacter("(") ) processTriple();
            t = topOperator(1);
            continue;
        }

        if ( t == "}" ) {
            while( checkCharacter("{") ) processTriple();
            t = topOperator(1);
            continue;
        }

        if ( OperatorHierarchy[t] >= OperatorHierarchy[topOperator()] ) {
            int i=0;
            while( OperatorStack.size() != 0 && OperatorHierarchy[t] >= OperatorHierarchy[topOperator()] && i < 50 ) {
                processTriple();
                i++;
            }
            OperatorStack.push(t);
        }
    }

    while( OperatorStack.size() ) processTriple();
    if (OperandStack.size()) {
        prefixMathExpression = OperandStack.top(); // store prefix expression
        prefixExpr = true;
        //cout << " resulting prefix espression: " << prefixMathExpression << endl;
    }
}

void MathExpression::buildTree() { // build a binary expression tree from the prefix expression data
    if (!prefixExpr) return;
    stack<Node*> nodeStack;
    Node* node = 0;

    /*vector<string> tokens; // split string in tokens
    string token;
    for (auto c : prefixMathExpression) {
        if (isMathToken(c)) {
            tokens.push_back(token);
            tokens.push_back(string()+c);
            token = "";
        } else token += c;
    }
    if (token.size()) tokens.push_back(token);*/

    //cout << "MathExpression::buildTree from prefix expression: " << prefixMathExpression << endl;
    vector<string> tokens = splitString(prefixMathExpression, ' '); // prefix expression is delimited by spaces!

    for (uint i=0; i<tokens.size(); i++) {
        string t = tokens[tokens.size()-i-1];
        node = new Node(t);
        nodes.push_back(node);
        if ( (t.size() == 1 && isMathToken(t[0])) || isMathFunction(t) ) { // found operator
            node->left = nodeStack.top(); nodeStack.pop();
            node->right = nodeStack.top(); nodeStack.pop();
            nodeStack.push(node);
        } else nodeStack.push(node);
    }
    tree = nodeStack.top(); nodeStack.pop();

    for (auto l : getLeafs()) l->setValue(l->param);
    //cout << "expression tree:\n" << tree->treeToString() << " " << tree->parent << endl;
}

MathExpression::MathExpression(string s) {
    set(s);

    OperatorHierarchy["dot"] = 0;
    OperatorHierarchy["cross"] = 0;
    OperatorHierarchy["{"] = 7;
    OperatorHierarchy["}"] = 7;
    OperatorHierarchy["+"] = 6;
    OperatorHierarchy["-"] = 6;
    OperatorHierarchy["*"] = 5;
    OperatorHierarchy["/"] = 5;
    OperatorHierarchy["("] = 2;
    OperatorHierarchy[")"] = 2;
    OperatorHierarchy["<"] = 1;
    OperatorHierarchy[">"] = 1;
    OperatorHierarchy["="] = 1;
}

MathExpression::~MathExpression() {}

MathExpressionPtr MathExpression::create() { return MathExpressionPtr( new MathExpression("") ); }

bool MathExpression::isMathMathExpression() {
    for (auto c : data) if (isMathToken(c)) return true;
    return false;
}

void MathExpression::makeTree() {
    convToPrefixExpr();
    buildTree();
}

vector<MathExpression::Node*> MathExpression::getLeafs() {
    vector<Node*> res;
    for (auto n : nodes) if (!n->left && !n->right) res.push_back(n);
    return res;
}

void MathExpression::set(string s) {
    data = s;
    data.erase(std::remove(data.begin(), data.end(), ' '), data.end());
    data.erase(std::remove(data.begin(), data.end(), '\t'), data.end());
    data.erase(std::remove(data.begin(), data.end(), '\n'), data.end());
}

string MathExpression::computeTree() { // compute result of binary expression tree
    //if (tree) cout << tree->treeToString() << endl;
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

string MathExpression::compute() {
    makeTree();
    return computeTree();
}

string MathExpression::toString() { return tree->toString(); }
string MathExpression::treeAsString() {
    if (!tree) makeTree();
    return tree ? tree->treeToString() : "No tree";
}





