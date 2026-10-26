#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cctype>
#include <tuple>

#include "minimalize.hpp"

static void trim_cr(std::string &s) {
    if (!s.empty() && s.back() == '\r') {
        s.pop_back();
    }
}

static bool compareStateNames(const std::string& a, const std::string& b) {
    int numA = -1, numB = -1;
    size_t idxA = 0; while (idxA < a.size() && !isdigit(a[idxA])) idxA++;
    if (idxA < a.size()) {
        try { numA = std::stoi(a.substr(idxA)); } catch (...) {}
    }
    
    size_t idxB = 0; while (idxB < b.size() && !isdigit(b[idxB])) idxB++;
    if (idxB < b.size()) {
        try { numB = std::stoi(b.substr(idxB)); } catch (...) {}
    }

    if (numA != -1 && numB != -1 && numA != numB) {
        return numA < numB;
    }
    return a < b;
}

static std::string toSName(const std::string& name) {
    if (!name.empty() && name[0] == 'q') {
        return "s" + name.substr(1);
    }
    return "s" + name;
}

void MinimalizeProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("mini", "Automata minimalizalasa");
}

bool MinimalizeProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("mini") > 0;
}

int MinimalizeProblem::run(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();

    std::ifstream inputFile(inputFilename);
    if (!inputFile) {
        std::cerr << "Hiba: Nem sikerult megnyitni a bemeneti fajlt: " << inputFilename << std::endl;
        return 1;
    }

   
    std::string line;
    if (!std::getline(inputFile, line)) return 1;
    trim_cr(line);
    std::stringstream ssStates(line);
    std::vector<std::string> allStates;
    std::set<std::string> validStates;
    std::string st;
    while (ssStates >> st) {
        allStates.push_back(st);
        validStates.insert(st);
    }

    if (!std::getline(inputFile, line)) return 1;
    trim_cr(line);
    std::stringstream ssAlphabet(line);
    std::vector<char> alphabet;
    std::set<char> validAlphabet;
    char sym;
    while (ssAlphabet >> sym) {
        alphabet.push_back(sym);
        validAlphabet.insert(sym);
    }

    std::string startState;
    inputFile >> startState;
    std::getline(inputFile, line); 
    if (validStates.find(startState) == validStates.end()) {
        std::cerr << "Hiba: A kezdoallapot (" << startState << ") nem szerepel az allapotok kozott!" << std::endl;
        return 1;
    }

    if (!std::getline(inputFile, line)) return 1;
    trim_cr(line);
    std::stringstream ssAccept(line);
    std::set<std::string> acceptStates;
    while (ssAccept >> st) {
        if (validStates.find(st) == validStates.end()) {
            std::cerr << "Hiba: A vegallapot (" << st << ") nem szerepel az allapotok kozott!" << std::endl;
            return 1;
        }
        acceptStates.insert(st);
    }

    std::map<std::pair<std::string, char>, std::string> transitions;
    std::string fromState, toState;
    char symbol;
    while (inputFile >> fromState >> symbol >> toState) {
        if (validStates.find(fromState) == validStates.end() || validStates.find(toState) == validStates.end()) {
            std::cerr << "Hiba: Ervenytelen allapot az atmenetben: " << fromState << " " << symbol << " " << toState << std::endl;
            return 1;
        }
        if (validAlphabet.find(symbol) == validAlphabet.end()) {
            std::cerr << "Hiba: Ervenytelen szimbolum az atmenetben: " << symbol << std::endl;
            return 1;
        }
        transitions[{fromState, symbol}] = toState;
    }
    inputFile.close();


    std::vector<std::string> reachableStates;
    std::set<std::string> visited;
    std::vector<std::string> queue;

    queue.push_back(startState);
    visited.insert(startState);

    size_t head = 0;
    while (head < queue.size()) {
        std::string curr = queue[head++];
        for (char a : alphabet) {
            if (transitions.count({curr, a})) {
                std::string nextState = transitions[{curr, a}];
                if (visited.find(nextState) == visited.end()) {
                    visited.insert(nextState);
                    queue.push_back(nextState);
                }
            }
        }
    }

    for (const auto& s : allStates) {
        if (visited.count(s)) {
            reachableStates.push_back(s);
        }
    }


    int n = reachableStates.size();
    std::map<std::pair<std::string, std::string>, bool> marked;

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            std::string u = reachableStates[i];
            std::string v = reachableStates[j];
            bool uAccept = (acceptStates.count(u) > 0);
            bool vAccept = (acceptStates.count(v) > 0);
            if (uAccept != vAccept) {
                marked[{u, v}] = true;
                marked[{v, u}] = true;
            }
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                std::string u = reachableStates[i];
                std::string v = reachableStates[j];
                if (marked[{u, v}]) continue;

                for (char a : alphabet) {
                    std::string uNext = transitions.count({u, a}) ? transitions[{u, a}] : "";
                    std::string vNext = transitions.count({v, a}) ? transitions[{v, a}] : "";

                    if (!uNext.empty() && !vNext.empty() && uNext != vNext) {
                        if (marked[{uNext, vNext}]) {
                            marked[{u, v}] = true;
                            marked[{v, u}] = true;
                            changed = true;
                            break;
                        }
                    }
                }
            }
        }
    }


    std::map<std::string, std::string> repMap; 
    std::vector<std::vector<std::string>> groups;
    std::set<std::string> processed;

    for (int i = 0; i < n; ++i) {
        std::string u = reachableStates[i];
        if (processed.count(u)) continue;

        std::vector<std::string> group;
        group.push_back(u);
        processed.insert(u);

        for (int j = i + 1; j < n; ++j) {
            std::string v = reachableStates[j];
            if (!marked[{u, v}]) {
                group.push_back(v);
                processed.insert(v);
            }
        }

        std::sort(group.begin(), group.end(), compareStateNames);
        std::string minRep = group[0];
        for (const auto& member : group) {
            repMap[member] = minRep;
        }
        groups.push_back(group);
    }


    std::vector<std::string> newStates;
    std::set<std::string> newStatesSet;
    for (const auto& grp : groups) {
        std::string sName = toSName(grp[0]);
        if (newStatesSet.find(sName) == newStatesSet.end()) {
            newStatesSet.insert(sName);
            newStates.push_back(sName);
        }
    }
    std::sort(newStates.begin(), newStates.end(), compareStateNames);

    std::string newStartState = toSName(repMap[startState]);

    std::vector<std::string> newAcceptStates;
    std::set<std::string> newAcceptSet;
    for (const auto& acc : acceptStates) {
        if (repMap.count(acc)) {
            std::string sName = toSName(repMap[acc]);
            if (newAcceptSet.find(sName) == newAcceptSet.end()) {
                newAcceptSet.insert(sName);
                newAcceptStates.push_back(sName);
            }
        }
    }
    std::sort(newAcceptStates.begin(), newAcceptStates.end(), compareStateNames);

    struct NewTransition {
        std::string from;
        char symbol;
        std::string to;

        bool operator<(const NewTransition& other) const {
            if (from != other.from) return compareStateNames(from, other.from);
            if (symbol != other.symbol) return symbol < other.symbol;
            return compareStateNames(to, other.to);
        }
    };

    std::vector<NewTransition> newTransitions;
    std::set<std::tuple<std::string, char, std::string>> uniqueTrans;

    for (const auto& grp : groups) {
        std::string rep = grp[0];
        std::string sFrom = toSName(rep);

        for (char a : alphabet) {
            if (transitions.count({rep, a})) {
                std::string originalTo = transitions[{rep, a}];
                if (repMap.count(originalTo)) {
                    std::string sTo = toSName(repMap[originalTo]);
                    auto tup = std::make_tuple(sFrom, a, sTo);
                    if (uniqueTrans.find(tup) == uniqueTrans.end()) {
                        uniqueTrans.insert(tup);
                        newTransitions.push_back({sFrom, a, sTo});
                    }
                }
            }
        }
    }

    std::sort(newTransitions.begin(), newTransitions.end());


    std::ofstream outputFile(outputFilename);
    if (!outputFile) {
        std::cerr << "Hiba: Nem sikerult megnyitni a kimeneti fajlt: " << outputFilename << std::endl;
        return 1;
    }

    for (size_t i = 0; i < newStates.size(); ++i) {
        outputFile << newStates[i] << (i + 1 < newStates.size() ? " " : "");
    }
    outputFile << "\n";

    for (size_t i = 0; i < alphabet.size(); ++i) {
        outputFile << alphabet[i] << (i + 1 < alphabet.size() ? " " : "");
    }
    outputFile << "\n";

    outputFile << newStartState << "\n";

    for (size_t i = 0; i < newAcceptStates.size(); ++i) {
        outputFile << newAcceptStates[i] << (i + 1 < newAcceptStates.size() ? " " : "");
    }
    outputFile << "\n";

    for (const auto& tr : newTransitions) {
        outputFile << tr.from << " " << tr.symbol << " " << tr.to << "\n";
    }

    return 0;
}