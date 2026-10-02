#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>

#include "dfa.hpp"

void DfaProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("check", "Ellenorizendo szavak (vesszovel elvalasztva)", cxxopts::value<std::string>());
}

bool DfaProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("check") > 0;
}

int DfaProblem::run(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();
    std::string checkArg = args["check"].as<std::string>();

    std::ifstream inputFile(inputFilename);
    if (!inputFile) {
        std::cerr << "Hiba: Nem sikerult megnyitni a bemeneti fajlt: " << inputFilename << std::endl;
        return 1;
    }
    std::string tempLine;
    std::getline(inputFile, tempLine);
    std::getline(inputFile, tempLine);

    std::string startState;
    inputFile >> startState;
    std::getline(inputFile, tempLine);

    std::string acceptLine;
    std::getline(inputFile, acceptLine);
    std::stringstream acceptStream(acceptLine);
    std::set<std::string> acceptStates;
    
    std::string stateName;
    while (acceptStream >> stateName) {
        acceptStates.insert(stateName);
    }

    std::map<std::pair<std::string, char>, std::string> transitions;
    std::string fromState, toState;
    char symbol;

    while (inputFile >> fromState >> symbol >> toState) {
        transitions[{fromState, symbol}] = toState;
    }

    inputFile.close(); 

    std::ofstream outputFile(outputFilename);
    if (!outputFile) {
        std::cerr << "Hiba: Nem sikerult megnyitni a kimeneti fajlt: " << outputFilename << std::endl;
        return 1;
    }

    std::stringstream checkStream(checkArg);
    std::string word;

    while (std::getline(checkStream, word, ',')) {
        if (!word.empty() && word.back() == '\r') {
            word.pop_back();
        }

        std::string currentState = startState;
        bool isValid = true;
        for (char ch : word) {
            if (transitions.count({currentState, ch}) > 0) {
                currentState = transitions[{currentState, ch}]; 
            } else {
                isValid = false; 
                break;
            }
        }

        if (isValid && acceptStates.count(currentState) > 0) {
            outputFile << "IGEN\n";
        } else {
            outputFile << "NEM\n";
        }
    }
    return 0;
}