#ifndef PROBLEMS_MINIMALIZE_HPP
#define PROBLEMS_MINIMALIZE_HPP

#include "../problem.hpp"

class MinimalizeProblem : public Problem {
public:
    void initialize_parser(cxxopts::Options &options) override;
    bool is_chosen_problem(const cxxopts::ParseResult &args) override;
    int run(const cxxopts::ParseResult &args) override;
};

#endif // PROBLEMS_MINIMALIZE_HPP