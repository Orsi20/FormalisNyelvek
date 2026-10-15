#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <algorithm>

#include "determinize.hpp"

using namespace std;

struct Transition {
    int from;
    string symbol;
    int to;

    bool operator<(const Transition &other) const {
        if (from != other.from) return from < other.from;
        if (symbol != other.symbol) return symbol < other.symbol;
        return to < other.to;
    }
};

void DeterminizeProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("det", "Determinize the input automaton");
}

bool DeterminizeProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("det") > 0;
}

int DeterminizeProblem::run(const cxxopts::ParseResult &args) {
    if (args.count("input") == 0 || args.count("output") == 0) {
        cerr << "Hiba: Hiányzó parancssori argumentumok!" << endl;
        return 1;
    }

    string inputFilename = args["input"].as<string>();
    string outputFilename = args["output"].as<string>();

    ifstream inputFile(inputFilename);
    if (!inputFile) {
        cerr << "Hiba a bemeneti fajl megnyitasakor: " << inputFilename << endl;
        return 1;
    }

    string line;

    if (!getline(inputFile, line)) {
        cerr << "Hiba a bemeneti fajl olvasasakor!" << endl;
        return 1;
    }
    stringstream ss1(line);
    set<string> valid_states;
    string token;
    while (ss1 >> token) {
        valid_states.insert(token);
    }

    if (!getline(inputFile, line)) {
        cerr << "Hiba az abece olvasasakor!" << endl;
        return 1;
    }
    stringstream ss2(line);
    vector<string> alphabet;
    set<string> valid_symbols;
    while (ss2 >> token) {
        alphabet.push_back(token);
        valid_symbols.insert(token);
    }

    if (!getline(inputFile, line)) {
        cerr << "Hiba a kezdoallapot olvasasakor!" << endl;
        return 1;
    }
    stringstream ss3(line);
    string start_state;
    ss3 >> start_state;
    if (valid_states.find(start_state) == valid_states.end()) {
        cerr << "Ervenytelen kezdoallapot: " << start_state << endl;
        return 1;
    }

    if (!getline(inputFile, line)) {
        cerr << "Hiba a vegallapotok olvasasakor!" << endl;
        return 1;
    }
    stringstream ss4(line);
    set<string> nfa_final_states;
    while (ss4 >> token) {
        if (valid_states.find(token) == valid_states.end()) {
            cerr << "Ervenytelen vegallapot: " << token << endl;
            return 1;
        }
        nfa_final_states.insert(token);
    }

    map<pair<string, string>, set<string>> nfa_transitions;

    while (getline(inputFile, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string from, sym, to;
        if (!(ss >> from >> sym >> to)) {
            continue;
        }

        if (valid_states.find(from) == valid_states.end()) {
            cerr << "Ervenytelen forras allapot: " << from << endl;
            return 1;
        }
        if (valid_symbols.find(sym) == valid_symbols.end()) {
            cerr << "Ervenytelen szimbolum: " << sym << endl;
            return 1;
        }
        if (valid_states.find(to) == valid_states.end()) {
            cerr << "Ervenytelen cel allapot: " << to << endl;
            return 1;
        }

        nfa_transitions[{from, sym}].insert(to);
    }

    vector<set<string>> dfa_subsets;
    map<set<string>, int> subset_map;
    queue<int> worklist;
    vector<Transition> dfa_transitions;

    set<string> initial_set = { start_state };
    dfa_subsets.push_back(initial_set);
    subset_map[initial_set] = 0;
    worklist.push(0);

    while (!worklist.empty()) {
        int current_idx = worklist.front();
        worklist.pop();

        const auto &current_set = dfa_subsets[current_idx];

        for (const auto &sym : alphabet) {
            set<string> next_set;

            for (const auto &state : current_set) {
                auto it = nfa_transitions.find({state, sym});
                if (it != nfa_transitions.end()) {
                    for (const auto &target : it->second) {
                        next_set.insert(target);
                    }
                }
            }

            if (!next_set.empty()) {
                int target_idx;
                auto map_it = subset_map.find(next_set);
                if (map_it == subset_map.end()) {
                    target_idx = static_cast<int>(dfa_subsets.size());
                    dfa_subsets.push_back(next_set);
                    subset_map[next_set] = target_idx;
                    worklist.push(target_idx);
                } else {
                    target_idx = map_it->second;
                }

                dfa_transitions.push_back({current_idx, sym, target_idx});
            }
        }
    }

    sort(dfa_transitions.begin(), dfa_transitions.end());

    ofstream outputFile(outputFilename);
    if (!outputFile) {
        cerr << "Hiba a kimeneti fajl megnyitasakor: " << outputFilename << endl;
        return 1;
    }

    for (size_t i = 0; i < dfa_subsets.size(); ++i) {
        outputFile << "s" << i << (i + 1 < dfa_subsets.size() ? " " : "");
    }
    outputFile << "\n";

    for (size_t i = 0; i < alphabet.size(); ++i) {
        outputFile << alphabet[i] << (i + 1 < alphabet.size() ? " " : "");
    }
    outputFile << "\n";

    outputFile << "s0\n";

    vector<string> dfa_final_states;
    for (size_t i = 0; i < dfa_subsets.size(); ++i) {
        bool is_final = false;
        for (const auto &state : dfa_subsets[i]) {
            if (nfa_final_states.count(state) > 0) {
                is_final = true;
                break;
            }
        }
        if (is_final) {
            dfa_final_states.push_back("s" + to_string(i));
        }
    }

    for (size_t i = 0; i < dfa_final_states.size(); ++i) {
        outputFile << dfa_final_states[i] << (i + 1 < dfa_final_states.size() ? " " : "");
    }
    outputFile << "\n";

    for (const auto &tr : dfa_transitions) {
        outputFile << "s" << tr.from << " " << tr.symbol << " s" << tr.to << "\n";
    }

    return 0;
}