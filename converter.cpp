#include <iostream>
#include <string>
#include <fstream>
#include <stack>
#include <algorithm>

using namespace std;

void CheckFile(bool iFile, const string& filename)
{
    if (!iFile) {
        cout << "Error: Cannot open file " << filename << "!" << endl;
        exit(0);
    }
}

void RemoveBracket(string &expr) {
    string temp;
    for (int i = 0; i < expr.length(); ++i) {
        if (expr[i] == '(') {
            string buf;
            i++;
            for (;i < expr.length(); ++i) {
                buf += expr[i];
                if (expr[i] == '&' || expr[i] == '|' || expr[i] == '^') {
                    temp += ('(' + buf.substr(0, buf.length()));
                    break;
                } else if (expr[i] == ')') {
                    temp += buf.substr(0, buf.length() - 1);
                    break;
                } else if (expr[i] == '(') {
                    temp += ('(' + buf.substr(0, buf.length() - 1));
                    i--;
                    break;
                }
            }
            continue;
        }
        temp += expr[i];
    }
    expr = temp;
}

int CountOperands(const string& rhs) {
    int count = 0;
    for (char i : rhs) {
        if (i == '&' || i == '|' || i == '^') {
            count++;
        }
    }
    return count + 1;
}

int main() {
    string filename;
    cout << "Input Verilog filename: ";
    cin >> filename;

    string output = filename.substr(0, filename.length() - 2) + "_out.v";

    ifstream iVerilog;
    iVerilog.open(filename);
    CheckFile((bool) iVerilog, filename);
    string line;

    ofstream oVerilog;
    oVerilog.open(output);
    ofstream assignTemp;
    assignTemp.open("temporary.v");

    int temp_num = 0;

    while (getline(iVerilog, line)) {
        if (line.length() > 2 && (line.find("(*") != std::string::npos || line.find("/*") != std::string::npos)) {
            continue;
        } else if (line.find("input") != std::string::npos || line.find("output") != std::string::npos ||
                   line.find("endmodule") != std::string::npos) {
            RemoveBracket(line);
            assignTemp << line << '\n';
        } else if (line.find("assign") != std::string::npos) {
            auto partition = line.find_first_of('=');
            string lhs = line.substr(0, partition - 1);
            string rhs = line.substr(partition);
            int num_operands = CountOperands(rhs);
            if (num_operands >= 2) {
                stack<char> rhs_stack;
                for (int i = 0; i < rhs.length(); ++i) {
                    if (rhs[i] == ')'/* && rhs[i + 1] != ';'*/) {
                        string temp_str;
                        while (rhs_stack.top() != '(') {
                            temp_str += rhs_stack.top();
                            rhs_stack.pop();
                        }
                        // reach the last '('
                        rhs_stack.pop();
                        // reverse the order of temp_str
                        reverse(temp_str.begin(), temp_str.end());
                        string temp_wire = "_temp" + to_string(temp_num) + "_";
                        oVerilog << "  wire " << temp_wire << ";\n";
                        assignTemp << "  assign " << temp_wire << " = " << temp_str << ";\n";
                        for (auto j:temp_wire) {
                            rhs_stack.push(j);
                        }
                        temp_num++;
                    } else {
                        rhs_stack.push(rhs[i]);
                    }
                }
                string temp_str;
                while (!rhs_stack.empty()) {
                    temp_str += rhs_stack.top();
                    rhs_stack.pop();
                }
                reverse(temp_str.begin(), temp_str.end());
                assignTemp << lhs << ' ' << temp_str << "\n";
            } else {
                RemoveBracket(line);
                assignTemp << line << '\n';
            }
        } else if (!line.empty()) {
            oVerilog << line << '\n';
        }
    }

    iVerilog.close();
    oVerilog.close();
    assignTemp.close();

    ifstream iFile;
    iFile.open("temporary.v");
    ofstream oFile;
    oFile.open(output, ios_base::app);

    while (getline(iFile, line)) {
        oFile << line << '\n';
    }

    return 0;
}
