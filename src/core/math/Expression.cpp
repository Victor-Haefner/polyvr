#include "Expression.h"
#include "core/utils/toString.h"
#include <stack>
#include <iostream>
#include <algorithm>
#include <OpenSG/OSGVector.h>

using namespace OSG;


map<string, Expression::Token*> Expression::tokens = map<string, Expression::Token*>();

Expression::Token::Token(string token, int priority, vector<string> types) {
    this->token = token;
    this->priority = priority;
    this->types = types;
}

bool Expression::Token::is(string t) { return token == t; }

bool Expression::Token::isA(string t) {
    for (auto i : types) if (t == i) return true;
    return false;
}

void Expression::Token::add(string token, int priority, vector<string> types) {
    tokens[token] = new Token(token, priority, types);
}

bool Expression::Token::check(string c) {
    initTokens();
    for (auto i : tokens) if (c == i.second->token) return true;
    return false;
}

void Expression::Token::initTokens() {
    if (tokens.size() > 0) return;
    add("=", 2, {"bracket", "math"});
    add("<", 2, {"bracket", "math", "openingBracket"});
    add(">", 2, {"bracket", "math", "closingBracket"});

    add("(", 3, {"bracket", "math", "openingBracket"});
    add(")", 3, {"bracket", "math", "closingBracket"});
    add("[", 3, {"bracket", "math", "openingBracket"});
    add("]", 3, {"bracket", "math", "closingBracket"});
    add("{", 3, {"bracket"});
    add("}", 3, {"bracket"});

    add("*", 5, {"math", "operator"});
    add("/", 5, {"math", "operator"});

    add("+", 6, {"math", "operator"});
    add("-", 6, {"math", "operator"});

    add(".", 7, {"delimiter", "math", "operator"});
    add(",", 7.5, {"delimiter", "math", "operator"});

    add(";", 8, {"delimiter", "operator"});
    add(":", 9, {"inference", "operator"});
}

Expression::TreeNode::TreeNode(string chunk, Token* token) {
    this->chunk = chunk;
    this->token = token;
}

void Expression::TreeNode::remChild(TreeNode* child) {
    if (child->parent == this) child->parent = 0;
    children.erase(std::remove(children.begin(), children.end(), child), children.end());
}

void Expression::TreeNode::addChild(TreeNode* child) {
    if (child->parent) child->parent->remChild(child);
    children.push_back(child);
    child->parent = this;
}

Expression::TreeNode* Expression::TreeNode::getChild(int i) {
    if (i >= 0 && i < (int)children.size()) return children[i];
    return 0;
}

Expression::TreeNode* Expression::TreeNode::getSibling(int offset) {
    if (!parent) return 0;
    auto siblings = parent->children;
    for (unsigned int i=0; i<siblings.size(); i++) {
        if (siblings[i] == this) {
            unsigned int k = i+offset;
            if (k >= 0 && k < siblings.size()) return siblings[k];
        }
    }
    return 0;
}

void Expression::TreeNode::setValue(string s) {
    chunk = s;
}

string Expression::TreeNode::toString() {
    string s = chunk;
    for (auto m : meaning) s += " " + decorate(m, "blue");
    return s;
}

void Expression::TreeNode::prettyPrint(string padding) {
    cout << padding << toString() << endl;
    for (auto child : children) child->prettyPrint(padding + "   ");
}

string Expression::TreeNode::prettyString(string padding) {
    string res = padding + toString() + "\n";
    for (auto child : children) res += child->prettyString(padding + "   ");
    return res;
}

bool Expression::TreeNode::is(string s) {
    if (token) if (token->is(s)) return true;
    return chunk == s;
}

bool Expression::TreeNode::isA(string s) {
    for (auto m : meaning) if (m == s) return true;
    if (token) if (token->isA(s)) return true;
    return false;
}

string Expression::TreeNode::collapseString() {
    int N = children.size();
    string result = chunk;
    if (N == 2 && !isA("bracket")) result = children[0]->collapseString() + chunk + children[1]->collapseString();
    else for (auto child : children) result += child->collapseString();
    return result;
}

Expression::Expression(string data) {
    tree = new TreeNode("");
    set(data);
}

Expression::~Expression() {}

ExpressionPtr Expression::create(string data) { return ExpressionPtr( new Expression(data) ); }

void Expression::substitute(string var, string val) {
    if (!tree) return;
    for (auto& n : nodes) {
        if (n->is(var)) n->setValue(val);
    }
}

void Expression::segment() {
    nodes.clear();
    string aggregate = "";
    for (auto c : data) {
        string C; C += c;
        if (Token::check(C)) {
            if (aggregate != "") nodes.push_back( new TreeNode(aggregate) );
            nodes.push_back( new TreeNode(C, tokens[C]) );
            aggregate = "";
        } else aggregate += c;
    }
    if (aggregate != "") nodes.push_back( new TreeNode(aggregate) );
}

void Expression::buildTree() {
    tree = new TreeNode("");
    TreeNode* parent = tree;

    // process brackets
    for (auto node : nodes) {
        parent->addChild(node);
        auto token = node->token;
        if (token) {
            if (token->isA("openingBracket")) parent = node;
            if (token->isA("closingBracket")) parent = parent->parent;
        }
    }

    // process function names
    for (auto node : nodes) {
        auto token = node->token;
        if (token && token->isA("openingBracket")) {
            auto name = node->getSibling(-1);
            if (name && !name->token) name->addChild(node);
        }
    }


    // process math operators && delimiters
    vector<TreeNode*> operators;
    for (auto node : nodes) if(node->token && node->token->isA("operator")) operators.push_back(node);

    auto compPriority = [](const TreeNode* a, const TreeNode* b) -> bool {
        return a->token->priority > b->token->priority;
    };

    sort(operators.begin(), operators.end(), compPriority);

    for (auto op : operators) {
        auto operand1 = op->getSibling(-1);
        auto operand2 = op->getSibling( 1);
        op->addChild(operand1);
        op->addChild(operand2);
    }
}

void Expression::classify(TreeNode* node) {
    if (!node) node = tree;
    auto token = node->token;

    if (token) {
        if (token->token == "[") { node->meaning.push_back("Vector"); node->meaning.push_back("NumericValue"); }
        if (token->token == "+") node->meaning.push_back("Addition");
        if (token->token == "-") node->meaning.push_back("Substraction");
        if (token->token == "*") node->meaning.push_back("Multiplication");
        if (token->token == "/") node->meaning.push_back("Division");
        if (token->token == ".") {
            auto s1 = node->getChild(0);
            auto s2 = node->getChild(1);
            if (s1 && s2) {
                if (isNumber(s1->chunk) && isNumber(s2->chunk)) { node->meaning.push_back("Float"); node->meaning.push_back("NumericValue"); }
                else node->meaning.push_back("Path");
            }
        }
    }

    if (!token) {
        auto c0 = node->getChild(0);
        if (c0 && c0->token && c0->token->token == "(") node->meaning.push_back("Function");
        if (isNumber(node->chunk)) { node->meaning.push_back("Integer"); node->meaning.push_back("NumericValue"); }
    }

    for (auto child : node->children) classify(child);
}

void Expression::parse() {
    segment();
    buildTree();
    classify();
}

string Expression::decorate(string s, string color) {
    if (color == "red") return "\033[91m" + s + "\033[0m";
    if (color == "green") return "\033[92m" + s + "\033[0m";
    if (color == "orange") return "\033[93m" + s + "\033[0m";
    if (color == "blue") return "\033[94m" + s + "\033[0m";
    return s;
}

bool Expression::isNumber(const string s) {
    if (s == "") return false;
    return s.find_first_not_of( "0123456789" ) == string::npos;
}

vector<Expression::TreeNode*> Expression::getLeafs() { return nodes; }

void Expression::set(string s) {
    data = s;
    data.erase(std::remove(data.begin(), data.end(), ' '), data.end());
    data.erase(std::remove(data.begin(), data.end(), '\t'), data.end());
    data.erase(std::remove(data.begin(), data.end(), '\n'), data.end());
}

string Expression::toString() { return tree->collapseString(); }
string Expression::treeToString() { return tree->prettyString(); }





namespace OSG {
    template<> MathExpression::ValueBase* MathExpression::Value<Vec3d>::compL(MathExpression::ValueBase* n) { return 0; }
    template<> MathExpression::ValueBase* MathExpression::Value<Vec3d>::compLE(MathExpression::ValueBase* n) { return 0; }
    template<> MathExpression::ValueBase* MathExpression::Value<Vec3d>::compG(MathExpression::ValueBase* n) { return 0; }
    template<> MathExpression::ValueBase* MathExpression::Value<Vec3d>::compGE(MathExpression::ValueBase* n) { return 0; }
}

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
    if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value / v2->value);
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

    template<> MathExpression::ValueBase* MathExpression::Value<Vec3d>::div(MathExpression::ValueBase* n) {
        return 0;
    }
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

MathExpression::MathExpression(string s) : Expression(s) {}
MathExpression::~MathExpression() {}

MathExpressionPtr MathExpression::create(string s) { return MathExpressionPtr( new MathExpression(s) ); }

bool MathExpression::isMathExpression() {
    for (auto c : data) if (isMathToken(c)) return true;
    return false;
}

string MathExpression::compute() {
    auto setValue = [&](TreeNode* node) {
        if (node->isA("Float")) {
            double f;
            bool b = toValue(node->collapseString(),f);
            if (b) values[node] = new Value<double>(f);
        }

        if (node->isA("Integer")) {
            double f;
            bool b = toValue(node->collapseString(),f);
            if (b) values[node] = new Value<double>(f);
        }

        if (node->isA("Vector")) {
            Vec3d f;
            bool b = toValue(node->collapseString(),f);
            cout << "MathExpression::compute::setValue " << f << "   " << node->collapseString() << endl;
            if (b) values[node] = new Value<Vec3d>(f);
        }
    };

    auto getList = [&](TreeNode* root) {
        vector<ValueBase*> results;
        function<void(TreeNode*)> aggregateList= [&](TreeNode* node) {
            if (node->is("(")) for (auto c : node->children) aggregateList(c);
            if (node->is(",")) for (auto c : node->children) aggregateList(c);
            else if (values.count(node)) results.push_back( values[node] );
        };
        aggregateList(root);
        return results;
    };

    auto computeOperator = [&](TreeNode* node) {
        if (!node) return;
        if (!node->token) return;
        if (node->children.size() < 2) return;
        auto left  = node->children[0];
        auto right = node->children[1];
        if (values.count(left) == 0 || values.count(right) == 0) return;
        auto leftV = values[left];
        auto rightV = values[right];
        string op = node->token->token;
        ValueBase* value = 0;
        if (op == "+") value = leftV->add(rightV);
        if (op == "-") value = leftV->sub(rightV);
        if (op == "*") value = leftV->mult(rightV);
        if (op == "/") value = leftV->div(rightV);
        if (op == "=") value = leftV->compE(rightV);
        if (op == "<") value = leftV->compL(rightV);
        //if (op == "<") value = leftV->compLE(rightV); // TODO
        if (op == ">") value = leftV->compG(rightV);
        //if (op == ">") value = leftV->compGE(rightV); // TODO
        if (op == "cross") value = leftV->cross(rightV);
        if (op == "dot") value = leftV->dot(rightV);
        if (value) values[node] = value;
        else cout << "Warning! operation " << op << " failed! " << leftV->toString() << " with " << rightV->toString() << endl;
    };

    auto computeFunction = [&](TreeNode* node) {
        if (!node) return;
        if (node->children.size() == 0) return;

        auto args = getList(node->children[0]);

        string op = node->chunk;
        ValueBase* value = 0;
        if (op == "cross" && args.size() == 2) value = args[0]->cross(args[1]);
        if (op == "dot" && args.size() == 2) value = args[0]->dot(args[1]);
        if (!value) return;

        values[node] = value;

        /*cout << node->chunk << " computeFunction " << op << " ( ";
        for (auto arg : args) cout << arg->toString() << " ";
        cout << ") = " << value->toString() << endl;*/
    };

    //if (tree) cout << tree->treeToString() << endl;
    std::function<void(TreeNode*)> subCompute = [&](TreeNode* node) {
        if (!node) return;
        for (auto child : node->children) subCompute(child);

        if (node->isA("NumericValue")) setValue(node);
        if (node->isA("operator")) computeOperator(node);
        if (node->isA("Function")) computeFunction(node);
    };

    parse();
    subCompute(tree);

    cout << "MathExpression::compute, found " << values.size() << " values!" << endl;

    if (!tree ) return "No tree!";
    if (tree->children.size() == 0) return "Tree has no child!";

    vector<ValueBase*> results;

    function<void(TreeNode*)> aggregateResults = [&](TreeNode* node) {
        if (values.count(node)) results.push_back( values[node] );
        if (node->is(",")) {
            for (auto c : node->children) aggregateResults(c);
        }
    };

    auto n0 = tree->children[0];
    aggregateResults(n0);
    //for (auto v : values) cout << " node: " << v.first->chunk << ", value: " << v.second->toString() << endl;
    if (results.size() == 0) return "No value found";

    string res;
    for (auto v : results) {
        if (v != results[0]) res += ",";
        res += v->toString();
    }
    return res;
}





