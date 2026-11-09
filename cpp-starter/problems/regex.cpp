#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <unordered_set>
#include <map>
#include <algorithm>

#include "regex.hpp"

static void trim_cr(std::string &s) {
    if (!s.empty() && s.back() == '\r') {
        s.pop_back();
    }
}

void RegexProblem::initialize_parser(cxxopts::Options &options) {
    options.add_options()
        ("rgx", "Regex feladat sorszama (1-5)", cxxopts::value<int>());
}

bool RegexProblem::is_chosen_problem(const cxxopts::ParseResult &args) {
    return args.count("rgx") > 0;
}

int RegexProblem::run(const cxxopts::ParseResult &args) {
    std::string inputFilename = args["input"].as<std::string>();
    std::string outputFilename = args["output"].as<std::string>();
    int taskNum = args["rgx"].as<int>();

    std::ifstream inputFile(inputFilename);
    if (!inputFile) {
        std::cerr << "Hiba: Nem sikerult megnyitni a bemeneti fajlt: " << inputFilename << std::endl;
        return 1;
    }

    std::ofstream outputFile(outputFilename);
    if (!outputFile) {
        std::cerr << "Hiba: Nem sikerult megnyitni a kimeneti fajlt: " << outputFilename << std::endl;
        return 1;
    }

    std::regex logcatRegex(R"(^(\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}(?:\.\d+)?)\s+(\d+)\s+(?:(\d+)\s+)?([A-Z])\s+([^:]+):\s*(.*)$)");

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(inputFile, line)) {
        trim_cr(line);
        if (!line.empty()) {
            lines.push_back(line);
        }
    }
    inputFile.close();

    if (taskNum == 1) {
        int c1 = 0, c2 = 0, c3 = 0, c4 = 0, c5 = 0;
        int c6 = 0, c7 = 0, c8 = 0, c9 = 0, c10 = 0;

        std::regex regDate15(R"(^10-15)");
        std::regex regExactTime(R"(^10-15\s+10:18:51)");
        std::regex reg51Sec(R"(^\d{2}-\d{2}\s+\d{2}:\d{2}:51\b)");
        std::regex regLowMemory(R"(\blowmemorykiller\b)", std::regex::icase);
        std::regex regExactPid(R"(^\d{2}-\d{2}\s+10:18:51(?:\.\d+)?\s+221\b)");
        std::regex regLevelE(R"(\s+E\s+)");
        std::regex regPkgW(R"(\s+W\s+PackageManager\s*:)");
        std::regex regExoD(R"(\s+D\s+.*ExoPlayer.*:)");
        std::regex regEW(R"(\s+[EW]\s+)");
        std::regex regJavaPos(R"(\.java:)");
        std::regex regThreadWord(R"(\bThread\b)");

        for (const auto &l : lines) {
            std::smatch match;
            bool isLogcat = std::regex_match(l, match, logcatRegex);

            std::string dateTime = isLogcat ? match[1].str() : "";
            std::string pid      = isLogcat ? match[2].str() : "";
            std::string tid      = isLogcat ? match[3].str() : "";
            std::string level    = isLogcat ? match[4].str() : "";
            std::string tag      = isLogcat ? match[5].str() : "";
            std::string msg      = isLogcat ? match[6].str() : "";

            if (std::regex_search(l, regDate15)) c1++;

            if (std::regex_search(l, regExactTime)) c2++;

            if (std::regex_search(l, reg51Sec)) c3++;

            if ((isLogcat && tag == "lowmemorykiller") || std::regex_search(l, regLowMemory)) c4++;

            if (std::regex_search(l, regExactPid) || (dateTime.find("10:18:51") != std::string::npos && pid == "221")) c5++;

            if ((isLogcat && level == "E") || std::regex_search(l, regLevelE)) c6++;

            if ((isLogcat && level == "W" && tag == "PackageManager") || std::regex_search(l, regPkgW)) c7++;

            if ((isLogcat && level == "D" && tag.find("ExoPlayer") != std::string::npos) || std::regex_search(l, regExoD)) c8++;

            bool isEW = (isLogcat && (level == "E" || level == "W")) || std::regex_search(l, regEW);
            if (isEW && std::regex_search(l, regJavaPos)) c9++;

            std::string msgPart = isLogcat ? msg : (l.find(':') != std::string::npos ? l.substr(l.find(':') + 1) : "");
            if (std::regex_search(msgPart, regThreadWord)) c10++;
        }

        outputFile << c1 << "\n" << c2 << "\n" << c3 << "\n" << c4 << "\n" << c5 << "\n";
        outputFile << c6 << "\n" << c7 << "\n" << c8 << "\n" << c9 << "\n" << c10 << "\n";

    } else if (taskNum == 2) {
        // 2. feladat
        std::regex javaFileRegex(R"(([A-Za-z0-9_$-]+\.java))");
        bool inErrorContext = false;

        for (const auto &l : lines) {
            std::smatch match;
            if (std::regex_match(l, match, logcatRegex)) {
                std::string level = match[4].str();
                if (level == "E") {
                    inErrorContext = true;
                    std::string msg = match[6].str();
                    auto begin = std::sregex_iterator(msg.begin(), msg.end(), javaFileRegex);
                    auto end = std::sregex_iterator();
                    for (std::sregex_iterator i = begin; i != end; ++i) {
                        outputFile << i->str() << "\n";
                    }
                } else {
                    inErrorContext = false;
                }
            } else if (inErrorContext) {
                // Stack trace sora (nincs logcat fejléc)
                auto begin = std::sregex_iterator(l.begin(), l.end(), javaFileRegex);
                auto end = std::sregex_iterator();
                for (std::sregex_iterator i = begin; i != end; ++i) {
                    outputFile << i->str() << "\n";
                }
            }
        }

    } else if (taskNum == 3) {
        // 3. feladat
        std::vector<std::string> uniquePids;
        std::unordered_set<std::string> seenPids;

        for (const auto &l : lines) {
            std::smatch match;
            if (std::regex_match(l, match, logcatRegex)) {
                std::string pid = match[2].str();
                if (seenPids.find(pid) == seenPids.end()) {
                    seenPids.insert(pid);
                    uniquePids.push_back(pid);
                }
            }
        }

        for (const auto &pid : uniquePids) {
            outputFile << pid << "\n";
        }

    } else if (taskNum == 4) {
        // 4. feladat
        std::map<std::string, int> errorCounts;

        for (const auto &l : lines) {
            std::smatch match;
            if (std::regex_match(l, match, logcatRegex)) {
                if (match[4].str() == "E") {
                    std::string pid = match[2].str();
                    errorCounts[pid]++;
                }
            }
        }

        std::string maxPid = "";
        int maxErrors = -1;

        for (const auto &entry : errorCounts) {
            if (entry.second > maxErrors) {
                maxErrors = entry.second;
                maxPid = entry.first;
            }
        }

        if (!maxPid.empty()) {
            outputFile << "PID=" << maxPid << " (" << maxErrors << " errors)\n";
        }

    } else if (taskNum == 5) {
        // 5. feladat
        int mainThreadErrors = 0;

        for (const auto &l : lines) {
            std::smatch match;
            if (std::regex_match(l, match, logcatRegex)) {
                std::string pid = match[2].str();
                std::string tid = match[3].str();
                std::string level = match[4].str();

                if (level == "E" && !tid.empty() && pid == tid) {
                    mainThreadErrors++;
                }
            }
        }

        outputFile << mainThreadErrors << "\n";
    }

    return 0;
}