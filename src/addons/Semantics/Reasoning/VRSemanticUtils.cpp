#include "VRSemanticUtils.h"
#include "VREntity.h"
#include "VROntology.h"
#include "VRReasoner.h"
#include "VRProperty.h"
#include "core/utils/toString.h"

#include <stack>
#include <iostream>
#include <algorithm>

using namespace OSG;

Variable::Variable() {;}

string Variable::toString() {
    string s = value+"(" + concept + "){";
    for (auto i : entities) s += i.second->getName()+",";
    if (entities.size() > 0) s.pop_back();
    s +="}[";
    if (isAnonymous) s += "anonymous, ";
    if (isAssumption) s += "assumption, ";
    s += valid ? "valid" : "invalid";
    s +="]";
    return s;
}

/* Variable flags:
    isAssumption: no entities of the type have been found
    isAnonymous: no entities with that name have been found
    !valid: something is terribly wrong!
*/

Variable::Variable(VROntologyPtr onto, string concept, string var) {
    auto cl = onto->getConcept(concept);
    if (cl == 0) return;

    if ( auto i = onto->getEntity(var) ) {
        addEntity(i);
        this->concept = concept; // TODO: maybe the entity has a concept that inherits from the concept passed above?
        isAnonymous = false;
    } else { // get all entities of the required type
        for (auto i : onto->getEntities(concept)) addEntity(i);
        if (entities.size() == 0) {
            auto i = onto->addEntity(var, concept);
            addEntity(i);
            isAssumption = true; // TODO: put that in the evaluation
        }
        this->concept = concept;
    }

    value = var;
    valid = true;
}

Variable::Variable(VROntologyPtr onto, string val) {
    value = val;
}

shared_ptr<Variable> Variable::create(VROntologyPtr onto, string concept, string var) { return shared_ptr<Variable>( new Variable(onto, concept, var) ); }
shared_ptr<Variable> Variable::create(VROntologyPtr onto, string val) { return shared_ptr<Variable>( new Variable(onto, val) ); }

void Variable::addEntity(VREntityPtr e) {
    entities[e->ID] = e;
    evaluations[e->ID] = Evaluation();
}

bool Variable::has(VariablePtr other, VROntologyPtr onto) {
    map<VREntityPtr, vector<VREntityPtr>> matches;

    // get all matches
    for (auto i1 : entities) { // all entities of that variable
        for (auto i2 : other->entities) { // check each instance of the other variable
            for (auto p : i1.second->properties) { // all properties of each instance
                for (auto v : p.second) {
                    if (v->value == other->value) matches[i1.second].push_back(0); // TODO: direct match with other variable value
                    if (v->value == i2.second->getName()) {
                        matches[i1.second].push_back(i2.second);
                    }
                }
            }
        }
    }

    // remove non matched entities
    vector<VREntityPtr> toDiscard1;
    vector<VREntityPtr> toDiscard2;
    for (auto i1 : entities) { // all entities of that variable
        if (matches.count(i1.second) == 0) toDiscard1.push_back(i1.second);
    }

    for (auto i2 : other->entities) { // check each instance of the other variable
        bool found = false;
        for (auto ev : matches) {
            for (auto e : ev.second) {
                found = (e == i2.second);
                if (found) break;
            }
            if (found) break;
        }
        if (!found) toDiscard2.push_back(i2.second);
    }

    for (auto e : toDiscard1) discard(e);
    for (auto e : toDiscard2) other->discard(e);

    return (matches.size() > 0);
}

bool Variable::is(VariablePtr other, VPath& p1, VPath& p2) {
    if (!valid || !other->valid) return false;

    auto hasSameVal = [&](vector<string>& val1, vector<string>& val2) {
        for (string s1 : val1) {
            for (string s2 : val2) if (s1 == s2) return true;
        }
        return false;
    };

    auto hasSameVal2 = [&](vector<string>& val1) {
        bool res = false;
        for (auto e : other->entities) {
            vector<string> val2 = e.second->getAtPath(p2.nodes);
            //for (auto v : val2) cout << " var2 value: " << v << endl;
            auto r = hasSameVal(val1, val2);
            if (!r) evaluations[e.first].state = Evaluation::INVALID;
            if (r) res = true;
        }
        if (res) return true;

        for (string s : val1) if (s == other->value) return true;
        return false;
    };

    //cout << "Variable::is " << toString() << " at path " << p1.toString() << " =?= " << other->toString() << " at path " << p2.toString() << endl;
    bool res = false;
    for (auto e : entities) {
        vector<string> val1 = e.second->getAtPath(p1.nodes);
        //for (auto v : val1) cout << " var1 value: " << v << endl;
        auto r = hasSameVal2(val1);
        if (!r) evaluations[e.first].state = Evaluation::INVALID;
        if (r) res = true;
    }
    return res;
}

bool Variable::operator==(Variable v) {
    if (!v.valid || !valid) return false;
    for (uint i=0; i<entities.size(); i++) {
        if (v.entities[i] != entities[i]) return false;
    }
    return true;
}

void Variable::discard(VREntityPtr e) {
    if (!entities.count(e->ID)) return;
    entities.erase(e->ID);
    evaluations.erase(e->ID);
}

VPath::VPath(string p) {
    nodes = VRReasoner::split(p, '.');
    if (nodes.size() == 0) return;
    root = nodes[0];
    first = nodes[nodes.size()-1];
}

string VPath::toString() {
    string s;
    for (auto p : nodes) s += p + '.';
    if (nodes.size() > 0) s.pop_back();
    return s;
}

int VPath::size() {
    return nodes.size();
}

Context::Context() {}

Context::Context(VROntologyPtr onto) {
    this->onto = onto;

    //cout << "Init context:" << endl;
    for (auto i : onto->entities) {
        if (i.second->getConcepts().size() == 0) { cout << "Context::Context instance " << i.second->getName() << " has no concepts!" << endl; continue; }
        vars[i.second->getName()] = Variable::create( onto, i.second->getConcepts()[0]->getName(), i.second->getName() );
        //cout << " add instance " << i.second->toString() << endl;
    }

    for ( auto r : onto->getRules()) {
        //cout << "Context prep rule " << r->toString() << endl;
        Query q(r->toString());
        if (!q.request) continue;

        for (Term& t : q.request->terms) {
            t.var = Variable::create(onto,t.path.root);

            for (auto& s : q.statements) {
                if (s->isSimpleVerb()) continue;
                s->terms[0].var = Variable::create(onto, s->terms[0].path.root);
                auto var = s->terms[0].var;
                if (var->value != t.var->value) continue;

                t.var->concept = s->verb;
                //cout << " Set concept of " << t.var->value << " to " << s->verb << endl;
                break;
            }
        }

        rules[r->toString()] = q;
    }
}

// TODO: parse concept statements here
Query::Query() {}

Query::Query(string q) {
    vector<string> parts = VRReasoner::split(q, ':');
    if (parts.size() > 0) request = VRStatement::create(parts[0]);
    if (parts.size() > 1) {
        parts = VRReasoner::split(parts[1], ';');
        for (uint i=0; i<parts.size(); i++) statements.push_back(VRStatement::create(parts[i], i));
    }
}

string Query::toString() {
    string res = request->toString() + " :";
    for (auto s : statements) res += " " + s->toString();
    return res;
}

class mathExpression { // TODO: put this in own header!
    public:
        struct ValueBase {
            virtual string toString() = 0;
            virtual ValueBase* add(ValueBase* n) = 0;
            virtual ValueBase* sub(ValueBase* n) = 0;
            virtual ValueBase* mult(ValueBase* n) = 0;
            virtual ValueBase* div(ValueBase* n) = 0;
        };

        template<class T> struct Value : ValueBase {
            T value;
            Value(const T& t) : value(t) {;}
            string toString() { return ::toString(value); }

            // TODO: only declare here, use partial template implementations!
            ValueBase* add(ValueBase* n) {
                if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value + v2->value);
                return 0;
            }
            ValueBase* sub(ValueBase* n) {
                if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value - v2->value);
                return 0;
            }
            ValueBase* mult(ValueBase* n) {
                if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value * v2->value);
                return 0;
            }
            ValueBase* div(ValueBase* n) {
                //if (auto v2 = dynamic_cast<Value<T>*>(n)) return new Value<T>(value / v2->value);
                return 0;
            }
        };

        //ValueBase* add(Value<Vec3f>* v1, Value<Vec3f>* v2) { return new Value<T>(v1->value + v2->value); }

        struct Node {
            string param;
            ValueBase* value = 0;
            Node* parent = 0;
            Node* left = 0;
            Node* right = 0;

            Node(string s) : param(s) {;}
            ~Node() { if (value) delete value; }

            void setValue(float f) { value = new Value<float>(f); }
            void setValue(Vec3f v) { value = new Value<Vec3f>(v); }
            void setValue(string s) {
                cout << " Node::setValue " << param << " " << s << endl;
                int N = std::count(s.begin(), s.end(), ' ');
                if (N == 0) setValue(toFloat(s));
                if (N == 2) setValue(toVec3f(s));
            }

            string toString(string indent = "") {
                string res = param;
                if (value) res += " ("+value->toString()+")";
                if (left) res += "\n"+indent+" " + left->toString(indent+" ");
                if (right) res += "\n"+indent+" " + right->toString(indent+" ");
                return res;
            }

            void compute() { // compute value based on left and right values and param as operator
                if (left->value == 0 || right->value == 0) return;
                char op = param[0];
                if (op == '+') value = left->value->add(right->value);
                if (op == '-') value = left->value->sub(right->value);
                if (op == '*') value = left->value->mult(right->value);
                if (op == '/') value = left->value->div(right->value);
            }
        };

    public:
        string data;
        Node* tree = 0;
        vector<Node*> nodes;
        map<char,int> OperatorHierarchy;

        bool isMathToken(char c) {
            if (c == '+' || c == '-' || c == '*' || c == '/') return true;
            if (c == '(' || c == ')') return true;
            return false;
        }

        void convToPrefixExpr() { // convert infix to prefix expression
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

        void buildTree() { // build a binary expression tree from the prefix expression data
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

    public:
        mathExpression(string s) {
            data = s;

            OperatorHierarchy['+'] = 6;
            OperatorHierarchy['-'] = 6;
            OperatorHierarchy['*'] = 5;
            OperatorHierarchy['/'] = 5;
            OperatorHierarchy['('] = 2;
            OperatorHierarchy[')'] = 2;
        }

        bool isMathExpression() {
            for (auto c : data) if (isMathToken(c)) return true;
            return false;
        }

        void computeTree() {
            convToPrefixExpr();
            buildTree();
        }

        vector<Node*> getLeafs() {
            vector<Node*> res;
            for (auto n : nodes) if (!n->left && !n->right) res.push_back(n);
            return res;
        }

        string compute() { // compute result of binary expression tree
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
};

Term::Term(string s) : path(s), str(s) { // parse term content
    // check for mathematical expression
    mathExpression me(s);
    if (me.isMathExpression()) {
        cout << " Term::Term '"+s+"' is math expression!\n";
        me.computeTree(); // build RDP tree
        for (auto l : me.getLeafs()) {
            VPath p(l->param);
            if (p.size() == 2) { // certainly not a number, go get the value!
                l->setValue("0"); // TODO: replace the variable with the numeric value!
            }
            if (p.size() == 1) {
                l->setValue(p.root); // default is to use path root, might just be a number
            }
        }
        if (me.tree) cout << me.tree->toString() << endl;
        string res = me.compute();
        cout << " Term::Term '"+s+"' results to " << res << endl;
        path = VPath(res ); // override old data
    }
}

bool Term::valid() { return var->valid; }

void Query::checkState() {
    int r = 1;
    for (auto i : statements) if(i->state == 0) r = 0;
    request->state = r;
}

void Query::substituteRequest(VRStatementPtr replace) { // replaces the roots of all paths of the terms of each statement
    for (auto statement : statements) {
        for (auto& ts : statement->terms) {
            for (int i=0; i<request->terms.size(); i++) {
                auto& t1 = request->terms[i];
                auto& t2 = replace->terms[i];
                if (t1.path.root == ts.path.root) {
                    ts.path.root = t2.path.root;
                    ts.path.nodes[0] = t2.path.nodes[0];
                    ts.str = ts.path.toString();
                }
            }
        }
    }

    request = replace;
}




