#ifndef EXPRESSION_H_INCLUDED
#define EXPRESSION_H_INCLUDED

#include <string>
#include <map>

#include <OpenSG/OSGVector.h>
#include "core/math/VRMathFwd.h"

using namespace std;
OSG_BEGIN_NAMESPACE;

class Expression {
    public:
        static string decorate(string s, string color) {
            if (color == "red") return "\033[91m" + s + "\033[0m";
            if (color == "blue") return "\033[94m" + s + "\033[0m";
            if (color == "green") return "\033[92m" + s + "\033[0m";
            if (color == "orange") return "\033[93m" + s + "\033[0m";
            return s;
        }

        static bool isNumber(const string s) {
            return s.find_first_not_of( "0123456789" ) == string::npos;
        }

        class Token;

        static map<string, Token*> tokens;

        class Token {
            public:
                string token;
                int priority;
                vector<string> types;

            public:
                Token(string token, int priority, vector<string> types) {
                    this->token = token;
                    this->priority = priority;
                    this->types = types;
                }

                bool isA(string t) {
                    for (auto i : types) if (t == i) return true;
                    return false;
                }

                static void add(string token, int priority, vector<string> types) {
                    tokens[token] = new Token(token, priority, types);
                }

                static bool check(string c) {
                    for (auto i : tokens) if (c == i.second->token) return true;
                    return false;
                }

                static void initTokens() {
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

                    add(".", 4, {"delimiter", "math", "operator"});

                    add("*", 5, {"math", "operator"});
                    add("/", 5, {"math", "operator"});

                    add("+", 6, {"math", "operator"});
                    add("-", 6, {"math", "operator"});

                    add(",", 7, {"delimiter", "math", "operator"});
                    add(";", 8, {"delimiter", "operator"});
                    add(":", 9, {"inference", "operator"});
                }
        };

        class TreeNode {
            public:
                string chunk;
                Token* token = 0;
                string meaning;

                vector<TreeNode*> children;
                TreeNode* parent = 0;

            public:
                TreeNode(string chunk, Token* token = 0) {
                    this->chunk = chunk;
                    this->token = token;
                }

                void remChild(TreeNode* child) {
                    if (child->parent == this) child->parent = 0;
                    children.erase(std::remove(children.begin(), children.end(), child), children.end());
                }

                void addChild(TreeNode* child) {
                    if (child->parent) child->parent->remChild(child);
                    children.push_back(child);
                    child->parent = this;
                }

                TreeNode* getChild(int i) {
                    if (i >= 0 && i < children.size()) return children[i];
                }

                TreeNode* getSibling(int offset) {
                    if (!parent) return 0;
                    auto siblings = parent->children;
                    for (int i=0; i<siblings.size(); i++) {
                        if (siblings[i] == this) {
                            int k = i+offset;
                            if (k >= 0 && k < siblings.size()) return siblings[k];
                        }
                    }
                }

                string toString() {
                    return chunk + " " + decorate(meaning, "blue");
                }

                void prettyPrint(string padding = "") {
                    cout << padding << toString();
                    for (auto child : children) child->prettyPrint(padding + "   ");
                }
        };

        string data;
        vector<TreeNode*> nodes;
        TreeNode* tree = 0;

    public:
        Expression(string data) {
            this->data = data;
            tree = new TreeNode("");
        }

        void segment() {
            nodes.clear();
            string aggregate = "";
            for (auto c : data) {
                if (Token::check(""+c)) {
                    if (aggregate != "") nodes.push_back( new TreeNode(aggregate) );
                    nodes.push_back( new TreeNode(""+c, tokens[""+c]) );
                    aggregate = "";
                } else aggregate += c;
            }
            if (aggregate != "") nodes.push_back( new TreeNode(aggregate) );
        }

        void buildTree() {
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
                    if (!name->token) name->addChild(node);
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

        void classify(TreeNode* node = 0) {
            if (!node) node = tree;
            auto token = node->token;

            if (token) {
                if (token->token == "[") node->meaning = "Vector";
                if (token->token == "+") node->meaning = "Addition";
                if (token->token == "-") node->meaning = "Substraction";
                if (token->token == "*") node->meaning = "Multiplication";
                if (token->token == "/") node->meaning = "Division";
                if (token->token == ".") {
                    auto s1 = node->getChild(0);
                    auto s2 = node->getChild(1);
                    if (s1 && s2) {
                        if (isNumber(s1->chunk) && isNumber(s2->chunk)) node->meaning = "Float";
                        else node->meaning = "Path";
                    }
                }
            }

            if (!token) {
                auto c0 = node->getChild(0);
                if (c0 && c0->token && c0->token->token == "(") node->meaning = "Function";
            }

            for (auto child : node->children) classify(child);
        }

        void parse() {
            segment();
            buildTree();
            classify();
        }
};

class MathExpression {
    public:
        struct ValueBase {
            virtual ~ValueBase();
            virtual string toString() = 0;
            virtual ValueBase* add(ValueBase* n) = 0;
            virtual ValueBase* sub(ValueBase* n) = 0;
            virtual ValueBase* mult(ValueBase* n) = 0;
            virtual ValueBase* div(ValueBase* n) = 0;
            virtual ValueBase* compE(ValueBase* n) = 0;
            virtual ValueBase* compL(ValueBase* n) = 0;
            virtual ValueBase* compLE(ValueBase* n) = 0;
            virtual ValueBase* compG(ValueBase* n) = 0;
            virtual ValueBase* compGE(ValueBase* n) = 0;
            virtual ValueBase* cross(ValueBase* n) = 0;
            virtual ValueBase* dot(ValueBase* n) = 0;
        };

        template<typename T> struct Value : ValueBase {
            T value;
            Value(const T& t);
            ~Value();
            string toString();

            // TODO { only declare here, use partial template implementations!
            ValueBase* add(ValueBase* n);
            ValueBase* sub(ValueBase* n);
            ValueBase* mult(ValueBase* n);
            ValueBase* div(ValueBase* n);
            ValueBase* compE(ValueBase* n);
            ValueBase* compL(ValueBase* n);
            ValueBase* compLE(ValueBase* n);
            ValueBase* compG(ValueBase* n);
            ValueBase* compGE(ValueBase* n);
            ValueBase* cross(ValueBase* n);
            ValueBase* dot(ValueBase* n);
        };

        //ValueBase* add(Value<Vec3d>* v1, Value<Vec3d>* v2) { return new Value<T>(v1->value + v2->value); }

        struct Node {
            string param;
            ValueBase* value = 0;
            Node* parent = 0;
            Node* left = 0;
            Node* right = 0;

            Node(string s);
            ~Node();

            void setValue(float f);
            void setValue(Vec3d v);
            void setValue(string s);

            string toString();
            string toString2();
            string treeToString(string indent = "");
            void compute();
        };

    public:
        string data;
        string prefixMathExpression;
        Node* tree = 0;
        vector<Node*> nodes;
        map<string,int> OperatorHierarchy;
        bool prefixExpr = false;

        bool isMathToken(char c);
        static bool isMathFunction(string f);
        void convToPrefixExpr();
        void buildTree();

    public:
        MathExpression(string s);
        ~MathExpression();

        static MathExpressionPtr create();

        bool isMathMathExpression();
        void makeTree();
        vector<Node*> getLeafs();
        void set(string s);
        string computeTree();
        string compute();
        string toString();
        string treeAsString();
};

OSG_END_NAMESPACE;

#endif // EXPRESSION_H_INCLUDED
