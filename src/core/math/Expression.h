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
        class Token {
            public:
                string token;
                int priority;
                vector<string> types;

            public:
                Token(string token, int priority, vector<string> types);

                bool is(string t);
                bool isA(string t);

                static void add(string token, int priority, vector<string> types);
                static bool check(string c);
                static void initTokens();
        };

        class TreeNode {
            public:
                string chunk;
                Token* token = 0;
                vector<string> meaning;

                vector<TreeNode*> children;
                TreeNode* parent = 0;

            public:
                TreeNode(string chunk, Token* token = 0);

                void remChild(TreeNode* child);
                void addChild(TreeNode* child);

                TreeNode* getChild(int i);
                TreeNode* getSibling(int offset);

                void setValue(string s);
                string toString();
                void prettyPrint(string padding = "");
                string prettyString(string padding = "");

                bool is(string s);
                bool isA(string s);
                string collapseString();
        };

        static string decorate(string s, string color);
        static bool isNumber(const string s);

        static map<string, Token*> tokens;

        string data;
        vector<TreeNode*> nodes;
        TreeNode* tree = 0;

    public:
        Expression(string data);
        ~Expression();

        static ExpressionPtr create();

        void segment();
        void buildTree();
        void classify(TreeNode* node = 0);
        void parse();

        vector<TreeNode*> getLeafs();
        void set(string s);

        string toString();
        string treeToString();
};

class MathExpression : public Expression {
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

    public:
        map<TreeNode*, ValueBase*> values;

        bool isMathToken(char c);
        static bool isMathFunction(string f);

    public:
        MathExpression(string s);
        ~MathExpression();

        static MathExpressionPtr create();

        bool isMathExpression();
        string compute();
};

OSG_END_NAMESPACE;

#endif // EXPRESSION_H_INCLUDED
