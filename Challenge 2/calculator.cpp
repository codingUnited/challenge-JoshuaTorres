// calculator.cpp
// Command-Line Calculator
//
// Features:
//  • Basic 2-number ops: + – * /
//  • Validation, divide-by-zero checks, chained operations
//  • Full infix parsing (Shunting-Yard), parentheses, exponents
//  • Functions: sin, cos, tan, sqrt, log, ln, exp
//  • Scientific notation (e.g., 1e-3), fixed-precision output
//  • REPL commands: help, clear, history, exit
//
// Usage:
//   1. Compile: g++ -std=c++17 -O2 calculator.cpp -o calculator
//   2. Run:     ./calculator
//   3. Type expressions or commands at the prompt.
//   4. “help” for full command list.

#include <iostream>
#include <string>
#include <sstream>
#include <stack>
#include <vector>
#include <map>
#include <functional>
#include <regex>
#include <cmath>
#include <iomanip>

using namespace std;

enum TokenType { NUMBER, OPERATOR, FUNCTION, LEFT_PAREN, RIGHT_PAREN };
struct Token {
    string     text;
    TokenType  type;
};

// Operator precedence and associativity
static const map<string,int> opPrec = {
    {"^", 4}, {"**", 4},
    {"*", 3}, {"/", 3},
    {"+", 2}, {"-", 2}
};
static const map<string,bool> opRightAssoc = {
    {"^", true}, {"**", true}
};

// Supported functions
static const vector<string> functions = {
    "sin","cos","tan","sqrt","log","ln","exp"
};

// Utility: is token a number?
bool isNumber(const string& s) {
    static const regex numRx(R"(^[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$)");
    return regex_match(s, numRx);
}

// Utility: is token an operator?
bool isOperator(const string& s) {
    return opPrec.count(s) > 0;
}

// Utility: is token a function name?
bool isFunction(const string& s) {
    return find(functions.begin(), functions.end(), s) != functions.end();
}

// Tokenize an expression string
vector<Token> tokenize(const string& expr) {
    vector<Token> tokens;
    size_t i = 0, n = expr.size();
    while (i < n) {
        if (isspace(expr[i])) { ++i; continue; }
        // Number or scientific
        if (isdigit(expr[i]) || expr[i]=='.') {
            size_t j = i;
            while (j<n && (isdigit(expr[j])||expr[j]=='.')) j++;
            if (j<n && (expr[j]=='e'||expr[j]=='E')) {
                j++;
                if (j<n && (expr[j]=='+'||expr[j]=='-')) j++;
                while (j<n && isdigit(expr[j])) j++;
            }
            tokens.push_back({expr.substr(i,j-i), NUMBER});
            i = j;
        }
        // Parens
        else if (expr[i]=='(') {
            tokens.push_back({"(", LEFT_PAREN}); ++i;
        }
        else if (expr[i]==')') {
            tokens.push_back({")", RIGHT_PAREN}); ++i;
        }
        // Two-char operator **
        else if (i+1<n && expr.substr(i,2)=="**") {
            tokens.push_back({"**", OPERATOR}); i+=2;
        }
        // Single-char operator
        else if (string("+-*/^").find(expr[i])!=string::npos) {
            tokens.push_back({string(1,expr[i]), OPERATOR}); ++i;
        }
        // Alphabetic: function or invalid
        else if (isalpha(expr[i])) {
            size_t j = i;
            while (j<n && isalpha(expr[j])) j++;
            string name = expr.substr(i,j-i);
            tokens.push_back({name,
                              isFunction(name)?FUNCTION:OPERATOR});
            i = j;
        }
        // Anything else
        else {
            tokens.push_back({string(1,expr[i]), OPERATOR}); ++i;
        }
    }
    return tokens;
}

// Convert infix tokens → postfix (Shunting-Yard)
vector<Token> infixToPostfix(const vector<Token>& input) {
    vector<Token> output;
    stack<Token>  ops;
    for (auto &tok: input) {
        if (tok.type == NUMBER) {
            output.push_back(tok);
        }
        else if (tok.type == FUNCTION) {
            ops.push(tok);
        }
        else if (tok.type == OPERATOR) {
            while (!ops.empty() && (
                       ops.top().type == FUNCTION ||
                      (ops.top().type == OPERATOR &&
                       (  opPrec.at(ops.top().text) >  opPrec.at(tok.text) ||
                        (opPrec.at(ops.top().text) == opPrec.at(tok.text) &&
                         !opRightAssoc.count(tok.text))
                       )
                      )
                   )) {
                output.push_back(ops.top());
                ops.pop();
            }
            ops.push(tok);
        }
        else if (tok.type == LEFT_PAREN) {
            ops.push(tok);
        }
        else if (tok.type == RIGHT_PAREN) {
            while (!ops.empty() && ops.top().type != LEFT_PAREN) {
                output.push_back(ops.top());
                ops.pop();
            }
            if (!ops.empty()) ops.pop(); // pop "("
            if (!ops.empty() && ops.top().type==FUNCTION) {
                output.push_back(ops.top());
                ops.pop();
            }
        }
    }
    while (!ops.empty()) {
        output.push_back(ops.top());
        ops.pop();
    }
    return output;
}

// Evaluate a postfix token list
double evalPostfix(const vector<Token>& pf) {
    stack<double> st;
    for (auto &tok: pf) {
        if (tok.type == NUMBER) {
            st.push(stod(tok.text));
        }
        else if (tok.type == FUNCTION) {
            if (st.empty()) throw runtime_error("Missing operand for function");
            double v = st.top(); st.pop();
            if      (tok.text=="sin")  st.push(sin(v));
            else if (tok.text=="cos")  st.push(cos(v));
            else if (tok.text=="tan")  st.push(tan(v));
            else if (tok.text=="sqrt") st.push(sqrt(v));
            else if (tok.text=="log")  st.push(log10(v));
            else if (tok.text=="ln")   st.push(log(v));
            else if (tok.text=="exp")  st.push(exp(v));
            else throw runtime_error("Unknown function: " + tok.text);
        }
        else if (tok.type == OPERATOR) {
            if (st.size() < 2) throw runtime_error("Missing operands");
            double b = st.top(); st.pop();
            double a = st.top(); st.pop();
            if      (tok.text=="+") st.push(a+b);
            else if (tok.text=="-") st.push(a-b);
            else if (tok.text=="*") st.push(a*b);
            else if (tok.text=="/") {
                if (b==0) throw runtime_error("Divide by zero");
                st.push(a/b);
            }
            else if (tok.text=="^"||tok.text=="**") {
                st.push(pow(a,b));
            }
        }
    }
    if (st.size() != 1) throw runtime_error("Invalid expression");
    return st.top();
}

int main() {
    cout << "Welcome to my Command-Line Calculator by Joshua Torres (Easy→Leet)\n";
    cout << "Type expressions or 'help' for commands.\n";

    vector<string> history;
    double lastResult = 0;
    bool hasResult = false;
    string line;

    while (true) {
        cout << "\n> ";
        if (!getline(cin, line) || line=="exit") {
            cout << "Goodbye!\n";
            break;
        }
        if (line.empty()) continue;

        // Commands
        if (line=="help") {
            cout << "help      Show this message\n"
                 << "clear     Reset history & last result\n"
                 << "history   List past expressions\n"
                 << "exit      Quit\n\n"
                 << "Supports + - * / ^ (), functions: sin,cos,tan,sqrt,log,ln,exp\n"
                 << "Scientific notation OK (e.g. 1e-3)\n"
                 << "Chaining: start with + - * / to use last result\n";
            continue;
        }
        if (line=="clear") {
            history.clear();
            hasResult = false;
            cout << "Cleared history & result.";
            continue;
        }
        if (line=="history") {
            cout << "History:";
            for (size_t i=0;i<history.size();++i)
                cout << "\n  " << (i+1) << ": " << history[i];
            continue;
        }

        // Chaining
        if (hasResult && !line.empty() && string("+-*/").find(line[0])!=string::npos)
            line = to_string(lastResult) + " " + line;

        try {
            auto tokens  = tokenize(line);
            auto postfix = infixToPostfix(tokens);
            double result = evalPostfix(postfix);
            cout << fixed << setprecision(6) << result;
            history.push_back(line);
            lastResult = result;
            hasResult = true;
        }
        catch (const out_of_range &) {
            cout << "Now you know that’s wrong, right 🤦🏾‍♂️\n";
        }
        catch (const exception &ex) {
            cout << "Error: " << ex.what() << "\n";
        }
    }
    return 0;
}
