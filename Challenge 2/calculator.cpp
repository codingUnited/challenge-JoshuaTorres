// calculator.cpp
// Joshua’s Command-Line Calculator
//
// What it does:
//   • Lets you type math expressions and get results instantly.
//   • Provides simple commands to help you: help, history, clear, exit.
//
// Supported:
//   - Operators: +   -   *   /   ^
//   - Parentheses: ( ... )
//   - Functions: sin(), cos(), tan(), sqrt(), log(), ln(), exp()
//   - Constants: pi (≈3.14159), e (≈2.71828)
//   - Scientific notation: 1e-3, 2E2
//   - Chaining: start with + - * / ^ to use last answer
//   - Adjustable decimal precision (6 digits by default)
//
// Not supported:
//   • Implicit multiplication (write 2*pi, not 2pi)
//   • Factorials, matrices, user-defined functions
//
// Note: trig functions use radians (e.g. sin(pi/2) = 1).
//
// Compile and run:
//   g++ -std=c++17 -O2 calculator.cpp -o calculator
//   ./calculator

#include <iostream>
#include <string>
#include <stack>
#include <vector>
#include <map>
#include <regex>
#include <cmath>
#include <iomanip>

using namespace std;

// Token types for parsing expressions
enum TokenType { NUMBER, OPERATOR, FUNCTION, LEFT_PAREN, RIGHT_PAREN };
struct Token {
    string text;       // literal text of the token
    TokenType type;    // what kind of token it is
};

// Operator precedence and associativity maps
static const map<string,int> opPrec = {
    {"^", 4}, {"**", 4},
    {"*", 3}, {"/", 3},
    {"+", 2}, {"-", 2}
};
static const map<string,bool> opRight = {
    {"^", true}, {"**", true}
};

// Recognized functions and constants
static const vector<string> functions = {
    "sin","cos","tan","sqrt","log","ln","exp"
};
static const map<string,double> constants = {
    {"pi", M_PI},
    {"e",  M_E}
};

// Check if a string matches a number (including scientific notation)
bool isNumber(const string& s) {
    static const regex numRx(R"(^[+-]?(\d+(\.\d*)?|\.\d+)([eE][+-]?\d+)?$)");
    return regex_match(s, numRx);
}

// Break an input expression into tokens
vector<Token> tokenize(const string& expr) {
    vector<Token> tokens;
    size_t i = 0, n = expr.size();

    while (i < n) {
        if (isspace(expr[i])) {
            ++i; 
            continue;        // skip whitespace
        }

        // Number or scientific notation
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
        // Left parenthesis
        else if (expr[i]=='(') {
            tokens.push_back({"(", LEFT_PAREN});
            ++i;
        }
        // Right parenthesis
        else if (expr[i]==')') {
            tokens.push_back({")", RIGHT_PAREN});
            ++i;
        }
        // Two-character exponent operator
        else if (i+1<n && expr.substr(i,2)=="**") {
            tokens.push_back({"**", OPERATOR});
            i += 2;
        }
        // Single-character operators + - * / ^
        else if (string("+-*/^").find(expr[i]) != string::npos) {
            tokens.push_back({string(1,expr[i]), OPERATOR});
            ++i;
        }
        // Alphabetic names: either function or constant
        else if (isalpha(expr[i])) {
            size_t j = i;
            while (j<n && isalpha(expr[j])) j++;
            string name = expr.substr(i, j-i);

            if (find(functions.begin(), functions.end(), name) != functions.end()) {
                tokens.push_back({name, FUNCTION});
            }
            else if (constants.count(name)) {
                // Replace constant with its numeric value
                tokens.push_back({to_string(constants.at(name)), NUMBER});
            }
            else {
                // Unknown identifier
                throw runtime_error("Unknown name: " + name);
            }
            i = j;
        }
        // Anything else is invalid
        else {
            throw runtime_error(string("Invalid character: ") + expr[i]);
        }
    }

    return tokens;
}

// Convert infix tokens to postfix (Reverse Polish Notation)
vector<Token> infixToPostfix(const vector<Token>& in) {
    vector<Token> out;
    stack<Token>  ops;

    for (auto &tok : in) {
        switch (tok.type) {
            case NUMBER:
                out.push_back(tok);
                break;

            case FUNCTION:
                ops.push(tok);
                break;

            case OPERATOR:
                // While top of ops stack has higher precedence, pop it first
                while (!ops.empty() &&
                      (ops.top().type == FUNCTION ||
                       (ops.top().type == OPERATOR &&
                        (opPrec.at(ops.top().text) > opPrec.at(tok.text) ||
                        (opPrec.at(ops.top().text) == opPrec.at(tok.text) &&
                         !opRight.count(tok.text))))))
                {
                    out.push_back(ops.top());
                    ops.pop();
                }
                ops.push(tok);
                break;

            case LEFT_PAREN:
                ops.push(tok);
                break;

            case RIGHT_PAREN:
                // Pop until matching left parenthesis
                while (!ops.empty() && ops.top().type != LEFT_PAREN) {
                    out.push_back(ops.top());
                    ops.pop();
                }
                if (!ops.empty()) ops.pop();  // remove "("
                if (!ops.empty() && ops.top().type == FUNCTION) {
                    out.push_back(ops.top());
                    ops.pop();               // pop the function too
                }
                break;
        }
    }

    // Pop any remaining operators
    while (!ops.empty()) {
        out.push_back(ops.top());
        ops.pop();
    }

    return out;
}

// Evaluate a postfix expression stack
double evalPostfix(const vector<Token>& pf) {
    stack<double> st;

    for (auto &tok : pf) {
        if (tok.type == NUMBER) {
            st.push(stod(tok.text));  // convert text to double
        }
        else if (tok.type == FUNCTION) {
            double v = st.top(); st.pop();
            if      (tok.text=="sin")  st.push(sin(v));
            else if (tok.text=="cos")  st.push(cos(v));
            else if (tok.text=="tan")  st.push(tan(v));
            else if (tok.text=="sqrt") st.push(sqrt(v));
            else if (tok.text=="log")  st.push(log10(v));
            else if (tok.text=="ln")   st.push(log(v));
            else if (tok.text=="exp")  st.push(exp(v));
        }
        else if (tok.type == OPERATOR) {
            double b = st.top(); st.pop();
            double a = st.top(); st.pop();
            if      (tok.text=="+")  st.push(a + b);
            else if (tok.text=="-")  st.push(a - b);
            else if (tok.text=="*")  st.push(a * b);
            else if (tok.text=="/")  {
                if (b == 0) throw runtime_error("Cannot divide by zero");
                st.push(a / b);
            }
            else if (tok.text=="^"||tok.text=="**") {
                st.push(pow(a, b));
            }
        }
    }

    if (st.size() != 1) throw runtime_error("Invalid expression");
    return st.top();
}

// Print friendly help instructions to the user
void printHelp() {
    cout << "\n❓ Need help? Here’s how to get started:\n\n"
         << "1) Type any math problem and press Enter:\n"
         << "     2 + 2          (adds two numbers)\n"
         << "     3 * 4          (multiplies)\n"
         << "     sqrt(16)       (square root)\n"
         << "     sin(pi/2)      (trig in radians)\n\n"
         << "2) Use these symbols and words:\n"
         << "     +  -  *  /  ^    ( )\n"
         << "     sin(), cos(), tan(), sqrt(), log(), ln(), exp()\n"
         << "     pi, e           sci‑notation: 1e-3, 2E2\n\n"
         << "3) Special commands:\n"
         << "     help  or  ?     show this message\n"
         << "     history         list past inputs\n"
         << "     clear           erase history & last answer\n"
         << "     exit            quit the calculator\n\n"
         << "Enjoy! 😊\n\n";
}

int main() {
    // Friendly welcome instructing what to do first
    cout << "\n🎉 Welcome to Joshua’s Calculator! 🎉\n"
         << "Type a math problem and press Enter,\n"
         << "or type \"help\" for instructions.\n\n";

    vector<string> history;
    double lastResult = 0.0;
    bool   hasResult  = false;
    int    precision  = 6;
    string line;

    while (true) {
        // If we have a previous result, show it in the prompt
        if (hasResult) {
            cout << "[" << fixed << setprecision(precision)
                 << lastResult << "] ";
        }
        cout << "> ";
        if (!getline(cin, line)) break;   // EOF or error
        if (line.empty()) continue;       // skip blank lines

        // Handle each command
        if (line == "exit") {
            cout << "Goodbye!\n";
            break;
        }
        if (line == "help" || line == "?") {
            printHelp();
            continue;
        }
        if (line == "clear") {
            history.clear();
            hasResult = false;
            cout << "✓ Cleared all history and last result.\n\n";
            continue;
        }
        if (line == "history") {
            cout << "\n📜 You typed:\n";
            for (size_t i = 0; i < history.size(); ++i) {
                cout << "  " << (i+1) << ": " << history[i] << "\n";
            }
            cout << "\n";
            continue;
        }

        // Chain operations: if input starts with an operator, prepend last result
        if (hasResult && string("+-*/^").find(line[0]) != string::npos) {
            line = to_string(lastResult) + line;
        }

        // Try parsing & evaluating the expression
        try {
            auto tokens  = tokenize(line);
            auto postfix = infixToPostfix(tokens);
            double result = evalPostfix(postfix);

            // Show the result with fixed precision
            cout << fixed << setprecision(precision) << result << "\n";

            // Save to history and prepare for chaining
            history.push_back(line);
            lastResult = result;
            hasResult  = true;
        }
        catch (const exception &ex) {
            // Friendly error message
            cout << "⚠️  Error: " << ex.what() << "\n";
        }
    }

    return 0;
}
