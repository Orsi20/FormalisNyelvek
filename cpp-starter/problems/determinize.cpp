#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <tuple>

#include "determinize.hpp"

using namespace std;

void DeterminizeProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("det", "Determinize the automaton");
}

bool DeterminizeProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("det") > 0;
}

int DeterminizeProblem::run(const cxxopts::ParseResult &args) {
    string inputFilename = args["input"].as<string>();
    string outputFilename = args["output"].as<string>();

    ifstream inputFile(inputFilename);

    if (!inputFile) {
        cerr << "Error opening input file: " << inputFilename << endl;
        return 1;
    }

    string line;
    vector<string> states;
    vector<string> alphabet;
    string startState;
    set<string> finalStates;

    if (!getline(inputFile, line)) {
        cerr << "Invalid input file" << endl;
        return 1;
    }

    {
        istringstream row(line);
        string state;

        while (row >> state) {
            states.push_back(state);
        }
    }

    if (!getline(inputFile, line)) {
        cerr << "Invalid input file" << endl;
        return 1;
    }

    {
        istringstream row(line);
        string symbol;

        while (row >> symbol) {
            alphabet.push_back(symbol);
        }
    }

    if (!getline(inputFile, startState)) {
        cerr << "Invalid input file" << endl;
        return 1;
    }

    startState.erase(0, startState.find_first_not_of(" \t\r\n"));

    if (startState.empty()) {
        cerr << "Invalid start state" << endl;
        return 1;
    }

    startState.erase(startState.find_last_not_of(" \t\r\n") + 1);

    if (!getline(inputFile, line)) {
        cerr << "Invalid input file" << endl;
        return 1;
    }

    {
        istringstream row(line);
        string state;

        while (row >> state) {
            finalStates.insert(state);
        }
    }

    set<string> validStates(states.begin(), states.end());
    set<string> validAlphabet(alphabet.begin(), alphabet.end());

    if (validStates.size() != states.size()) {
        cerr << "Duplicate state" << endl;
        return 1;
    }

    if (validAlphabet.size() != alphabet.size()) {
        cerr << "Duplicate alphabet symbol" << endl;
        return 1;
    }

    if (validStates.find(startState) == validStates.end()) {
        cerr << "Invalid start state: " << startState << endl;
        return 1;
    }

    for (const string &state : finalStates) {
        if (validStates.find(state) == validStates.end()) {
            cerr << "Invalid final state: " << state << endl;
            return 1;
        }
    }

    sort(alphabet.begin(), alphabet.end());

    map<pair<string, string>, set<string>> transitions;

    string from, symbol, to;

    while (inputFile >> from >> symbol >> to) {
        if (validStates.find(from) == validStates.end()) {
            cerr << "Invalid transition state: " << from << endl;
            return 1;
        }

        if (validStates.find(to) == validStates.end()) {
            cerr << "Invalid transition state: " << to << endl;
            return 1;
        }

        if (validAlphabet.find(symbol) == validAlphabet.end()) {
            cerr << "Invalid transition symbol: " << symbol << endl;
            return 1;
        }

        transitions[make_pair(from, symbol)].insert(to);
    }

    vector<set<string>> newStates;
    map<set<string>, int> stateNumbers;

    set<string> initialState;
    initialState.insert(startState);

    newStates.push_back(initialState);
    stateNumbers[initialState] = 0;

    vector<tuple<int, string, int>> newTransitions;

    for (int i = 0; i < (int)newStates.size(); i++) {
        set<string> currentState = newStates[i];

        for (const string &currentSymbol : alphabet) {
            set<string> destination;

            for (const string &state : currentState) {
                auto it = transitions.find(
                    make_pair(state, currentSymbol)
                );

                if (it != transitions.end()) {
                    destination.insert(
                        it->second.begin(),
                        it->second.end()
                    );
                }
            }

            if (destination.empty()) {
                continue;
            }

            if (stateNumbers.find(destination) == stateNumbers.end()) {
                int number = (int)newStates.size();

                newStates.push_back(destination);
                stateNumbers[destination] = number;
            }

            int destinationNumber = stateNumbers[destination];

            newTransitions.push_back(
                make_tuple(i, currentSymbol, destinationNumber)
            );
        }
    }

    sort(newTransitions.begin(), newTransitions.end());

    ofstream outputFile(outputFilename);

    if (!outputFile) {
        cerr << "Error opening output file: " << outputFilename << endl;
        return 1;
    }

    for (int i = 0; i < (int)newStates.size(); i++) {
        if (i > 0) {
            outputFile << " ";
        }

        outputFile << "s" << i;
    }

    outputFile << endl;

    for (int i = 0; i < (int)alphabet.size(); i++) {
        if (i > 0) {
            outputFile << " ";
        }

        outputFile << alphabet[i];
    }

    outputFile << endl;
    outputFile << "s0" << endl;

    bool first = true;

    for (int i = 0; i < (int)newStates.size(); i++) {
        bool isFinal = false;

        for (const string &state : newStates[i]) {
            if (finalStates.find(state) != finalStates.end()) {
                isFinal = true;
                break;
            }
        }

        if (isFinal) {
            if (!first) {
                outputFile << " ";
            }

            outputFile << "s" << i;
            first = false;
        }
    }

    outputFile << endl;

    for (const auto &transition : newTransitions) {
        outputFile << "s" << get<0>(transition) << " "
                   << get<1>(transition) << " "
                   << "s" << get<2>(transition) << endl;
    }

    return 0;
}