#include "se/cbmc/exploration/explorer.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "se/cbmc/exploration/heuristic/breadth_first_heuristic.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <sstream>

using namespace se::cbmc;

std::ostream &Explorer::print(std::ostream &os) const {
    std::stringstream str;
    str << "(\n";
    str << "\tcontext queue: {";
    for (auto it = _contexts.begin(); it != _contexts.end(); ++it) {
        str << (*it)->getState().getVertex().getLabel();
        if (std::next(it) != _contexts.end()) {
            str << ", ";
        }
    }
    str << "},\n";
    str << "\tcoverage goals: ";
    str << "[";
    for (auto it = _goals.begin(); it != _goals.end(); ++it) {
        str << *it;
        if (std::next(it) != _goals.end()) {
            str << ", ";
        }
    }
    str << "]\n";
    str << ")";
    return os << str.str();
}

bool Explorer::isEmpty() const {
    return _contexts.empty();
}

void Explorer::push(std::unique_ptr<Context> context) {
    _contexts.push_back(std::move(context));
    std::push_heap(_contexts.begin(), _contexts.end(), BreadthFirstHeuristic());
}

std::unique_ptr<Context> Explorer::pop() {
    std::pop_heap(_contexts.begin(), _contexts.end(), BreadthFirstHeuristic());
    std::unique_ptr<Context> context = std::move(_contexts.back());
    _contexts.pop_back();
    return context;
}

bool Explorer::hasGoal(const std::string &goal) {
    return _goals.find(goal) != _goals.end();
}

void Explorer::removeGoal(const std::string &goal) {
    assert(hasGoal(goal));
    _goals.erase(goal);
}

void Explorer::checkGoals(Solver &solver, const Context &context) {
    auto logger = spdlog::get("CBMC");
    const State &state = context.getState();
    const z3::expr &assumption_literal = state.getAssumptionLiteral();
    // extract cycle, context-insensitive checking of goal reachability
    std::string goal = assumption_literal.to_string().substr(0, assumption_literal.to_string().find("__"));
    if (_goals.count(goal)) {
        SPDLOG_LOGGER_TRACE(logger, "Checking if goal {} is reachable...", goal);
        unsigned int cycle = context.getCycle();
        const Vertex &vertex = state.getVertex();
        z3::expr_vector expressions(solver.getContext());
        // initial valuations
        for (const auto &initial_valuation : state.getInitialValuations()) {
            if (initial_valuation.second.is_bool()) {
                expressions.push_back(solver.makeBooleanConstant(initial_valuation.first) == initial_valuation.second);
            } else if (initial_valuation.second.is_int()) {
                expressions.push_back(solver.makeIntegerConstant(initial_valuation.first) == initial_valuation.second);
            } else {
                throw std::runtime_error("Unexpected z3::sort encountered.");
            }
        }

        // control-flow constraints
        for (const auto &assumption_literals : state.getAssumptionLiterals()) {
            std::string assumption_literal_name = assumption_literals.first;
            z3::expr_vector preceding_assumption_literals(solver.getContext());
            for (const auto &preceding_assumption_literal : assumption_literals.second) {
                preceding_assumption_literals.push_back(preceding_assumption_literal);
            }
            expressions.push_back(z3::implies(solver.makeBooleanConstant(assumption_literal_name),
                                              z3::mk_or(preceding_assumption_literals).simplify()));
        }

        // assumption constraints
        for (const auto &assumptions : state.getAssumptions()) {
            std::string assumption_literal_name = assumptions.first;
            z3::expr_vector assumption_expressions(solver.getContext());
            for (const auto &assumption : assumptions.second) {
                assumption_expressions.push_back(assumption);
            }
            expressions.push_back(z3::implies(solver.makeBooleanConstant(assumption_literal_name),
                                              z3::mk_and(assumption_expressions)));
        }

        // hard constraints
        for (const auto &hard_constraints : state.getHardConstraints()) {
            std::string assumption_literal_name = hard_constraints.first;
            z3::expr_vector hard_constraint_expressions(solver.getContext());
            for (const auto &hard_constraint_expression : hard_constraints.second) {
                if (hard_constraint_expression.second.is_bool()) {
                    hard_constraint_expressions.push_back(
                            solver.makeBooleanConstant(hard_constraint_expression.first) ==
                            hard_constraint_expression.second);
                } else if (hard_constraint_expression.second.is_int()) {
                    hard_constraint_expressions.push_back(
                            solver.makeIntegerConstant(hard_constraint_expression.first) ==
                            hard_constraint_expression.second);
                } else {
                    throw std::runtime_error("Unsupported z3::sort encountered.");
                }
            }
            expressions.push_back(z3::implies(solver.makeBooleanConstant(assumption_literal_name),
                                              z3::mk_and(hard_constraint_expressions)));
        }


    }
}

void Explorer::initialize(const Cfg &cfg) {
    std::string scope = cfg.getName();
    std::set<std::string> visited_cfgs;
    initializeGoals(cfg, scope, visited_cfgs);
}

void Explorer::initializeGoals(const Cfg &cfg, const std::string &scope, std::set<std::string> &visited_cfgs) {
    std::string name = cfg.getName();
    for (auto it = cfg.verticesBegin(); it != cfg.verticesEnd(); ++it) {
        switch (it->getType()) {
            case Vertex::Type::ENTRY: {
                break;
            }
            case Vertex::Type::REGULAR: {
                unsigned int label = it->getLabel();
                if (auto if_instruction = dynamic_cast<const ir::IfInstruction *>(it->getInstruction())) {
                    std::vector<std::shared_ptr<Edge>> outgoing_edges = cfg.getOutgoingEdges(label);
                    assert(outgoing_edges.size() == 2);
                    for (const auto &outgoing_edge : outgoing_edges) {
                        _goals.emplace("b_" + scope + "_" + std::to_string(outgoing_edge->getTargetLabel()));
                    }
                } else if (auto call_instruction = dynamic_cast<const ir::CallInstruction *>(it->getInstruction())) {
                    std::string callee_scope = scope + "." + call_instruction->getVariableAccess().getName();
                    std::shared_ptr<Cfg> callee = cfg.getCallee(label);
                    const auto &edge = cfg.getIntraproceduralCallToReturnEdge(label);
                    initializeGoals(*callee, callee_scope, visited_cfgs);
                }
                break;
            }
            case Vertex::Type::EXIT: {
                break;
            }
        }
    }
}