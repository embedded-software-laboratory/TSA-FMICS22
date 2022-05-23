#include "se/shadow/exploration/explorer.h"
#include "ir/instruction/sequence_instruction.h"
#include "se/shadow/exploration/heuristic/breadth_first_heuristic.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

using namespace se::shadow;

Explorer::Explorer()
    : _contexts(std::vector<std::unique_ptr<Context>>()), _statement_coverage(0),
      _covered_statements(std::map<unsigned int, bool>()), _branch_coverage(0),
      _covered_branches(std::map<unsigned int, std::pair<bool, bool>>()) {}

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
    str << std::boolalpha;
    str << "\tcovered statements: {";
    for (auto it = _covered_statements.begin(); it != _covered_statements.end(); ++it) {
        str << it->first << ": " << it->second;
        if (std::next(it) != _covered_statements.end()) {
            str << ", ";
        }
    }
    str << "},\n";
    str << "\tcovered branches: {";
    for (auto it = _covered_branches.begin(); it != _covered_branches.end(); ++it) {
        str << it->first << ": (" << it->second.first << ", " << it->second.second << ")";
        if (std::next(it) != _covered_branches.end()) {
            str << ", ";
        }
    }
    str << "},\n";
    str << "\tcoverage values: (";
    str << "statement: " << _statement_coverage << ", "
        << "branch: " << _branch_coverage;
    str << ")\n";
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

std::pair<bool, bool> Explorer::updateCoverage(unsigned int label, const Context &context) {
    auto logger = spdlog::get("Shadow");

    const State &state = context.getState();
    const Vertex &vertex = state.getVertex();
    unsigned int succeeding_label = vertex.getLabel();
    const Frame &frame = context.getFrame();
    const Cfg &cfg = frame.getCfg();

    _covered_statements.at(label) = true;

    for (const auto &outgoing_edge : cfg.getOutgoingEdges(label)) {
        if (succeeding_label == outgoing_edge->getTargetLabel()) {
            if (outgoing_edge->getType() == Edge::Type::TRUE_BRANCH) {
                _covered_branches.at(label).first = true;
            } else if (outgoing_edge->getType() == Edge::Type::FALSE_BRANCH) {
                _covered_branches.at(label).second = true;
            } else {
                // do nothing
            }
        }
    }

    // XXX recompute coverage values
    double statement_coverage = _statement_coverage;
    std::size_t covered_statements = std::count_if(_covered_statements.begin(), _covered_statements.end(),
                                                   [](const auto &p) { return p.second; });
    _statement_coverage = (double) covered_statements / (double) _covered_statements.size();

    double branch_coverage = _branch_coverage;
    unsigned int covered_branches = 0;
    for (const auto &label_to_coverage : _covered_branches) {
        if (label_to_coverage.second.first) {
            covered_branches += 1;
        }
        if (label_to_coverage.second.second) {
            covered_branches += 1;
        }
    }
    _branch_coverage = (double) covered_branches / (double) (2 * _covered_branches.size());

    SPDLOG_LOGGER_TRACE(logger,
                        "Updated statement and branch coverage: ({}, {}) after execution of vertex at label: {}.",
                        _statement_coverage, _branch_coverage, label);

    return std::make_pair(statement_coverage < _statement_coverage, branch_coverage < _branch_coverage);
}

void Explorer::initialize(const Cfg &cfg) {
    std::set<std::string> visited_cfgs;
    initializeCoverage(cfg, visited_cfgs);
}

void Explorer::initialize(
        const Cfg &cfg, const Context &context,
        const std::map<unsigned int, std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                                           std::vector<unsigned int>>>> &execution_history) {
    auto logger = spdlog::get("Shadow");
    std::set<std::string> visited_cfgs;
    initializeCoverage(cfg, visited_cfgs);
    for (unsigned int cycle = 0; cycle <= context.getCycle(); ++cycle) {
        const std::vector<std::pair<std::pair<std::string, std::reference_wrapper<const Cfg>>,
                                    std::vector<unsigned int>>> &local_execution_history = execution_history.at(cycle);
        for (auto it = local_execution_history.begin(); it != local_execution_history.end(); ++it) {
            for (unsigned int label : it->second) {
                if (label == context.getState().getVertex().getLabel()) {
                    // XXX even though simulation went past the divergent context, we only initialize the coverage up
                    // XXX to the point where the divergent context was enqueued in order to drive the execution also
                    // XXX in the other parts or detect uncovered behavior after a change
                    goto initialize_coverage;
                }
                if (_covered_statements.find(label) != _covered_statements.end()) {
                    _covered_statements.at(label) = true;
                }
                if (_covered_branches.find(label) != _covered_branches.end()) {
                    // XXX might be in a callee -_-"
                    for (const auto &edge : it->first.second.get().getOutgoingEdges(label)) {
                        if (std::find(it->second.begin(), it->second.end(), edge->getTargetLabel()) !=
                            it->second.end()) {
                            if (edge->getType() == Edge::Type::TRUE_BRANCH) {
                                _covered_branches.at(label).first = true;
                            } else if (edge->getType() == Edge::Type::FALSE_BRANCH) {
                                _covered_branches.at(label).second = true;
                            } else {
                                throw std::runtime_error("Unexpected edge type encountered.");
                            }
                        }
                    }
                }
            }
        }
    }
initialize_coverage:
    // XXX recompute coverage values
    double statement_coverage = _statement_coverage;
    std::size_t covered_statements = std::count_if(_covered_statements.begin(), _covered_statements.end(),
                                                   [](const auto &p) { return p.second; });
    _statement_coverage = (double) covered_statements / (double) _covered_statements.size();

    double branch_coverage = _branch_coverage;
    unsigned int covered_branches = 0;
    for (const auto &label_to_coverage : _covered_branches) {
        if (label_to_coverage.second.first) {
            covered_branches += 1;
        }
        if (label_to_coverage.second.second) {
            covered_branches += 1;
        }
    }
    _branch_coverage = (double) covered_branches / (double) (2 * _covered_branches.size());

    SPDLOG_LOGGER_TRACE(logger, "Initial statement and branch coverage: ({}, {}).", _statement_coverage,
                        _branch_coverage);
}

void Explorer::initializeCoverage(const Cfg &cfg, std::set<std::string> &visited_cfgs) {
    std::string name = cfg.getName();
    assert(visited_cfgs.find(name) == visited_cfgs.end());
    visited_cfgs.insert(name);

    // XXX recurse on callees
    for (auto it = cfg.calleesBegin(); it != cfg.calleesEnd(); ++it) {
        if (visited_cfgs.count(it->getName()) == 0) {
            initializeCoverage(*it, visited_cfgs);
        }
    }

    for (auto it = cfg.verticesBegin(); it != cfg.verticesEnd(); ++it) {
        switch (it->getType()) {
            case Vertex::Type::ENTRY: {
                _covered_statements.emplace(it->getLabel(), false);
                break;
            }
            case Vertex::Type::REGULAR: {
                ir::Instruction *instruction = it->getInstruction();
                assert(instruction != nullptr);
                switch (instruction->getKind()) {
                    case ir::Instruction::Kind::ASSIGNMENT: {
                        // XXX fall-through
                    }
                    case ir::Instruction::Kind::HAVOC: {
                        // XXX fall-through
                    }
                    case ir::Instruction::Kind::CALL: {
                        _covered_statements.emplace(it->getLabel(), false);
                        break;
                    }
                    case ir::Instruction::Kind::SEQUENCE: {
                        _covered_statements.emplace(it->getLabel(), false);
                        auto *sequence_instruction = dynamic_cast<ir::SequenceInstruction *>(instruction);
                        if (sequence_instruction->containsIfInstruction()) {
                            _covered_branches.emplace(it->getLabel(), std::make_pair(false, false));
                        }
                        break;
                    }
                    case ir::Instruction::Kind::IF: {
                        // XXX fall-through
                    }
                    case ir::Instruction::Kind::WHILE: {
                        _covered_statements.emplace(it->getLabel(), false);
                        _covered_branches.emplace(it->getLabel(), std::make_pair(false, false));
                        break;
                    }
                    case ir::Instruction::Kind::GOTO:
                    default:
                        throw std::runtime_error("Unexpected instruction encountered during coverage initialization.");
                }
                break;
            }
            case Vertex::Type::EXIT: {
                _covered_statements.emplace(it->getLabel(), false);
                break;
            }
        }
    }
}

void Explorer::reset() {
    _contexts.clear();
    _statement_coverage = 0;
    _covered_statements.clear();
    _branch_coverage = 0;
    _covered_branches.clear();
}