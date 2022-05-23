#include "se/simulator/execution/executor.h"
#include "ir/expression/field_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

using namespace se::simulator;

Executor::Executor(Evaluator::ShadowProcessingMode shadow_processing_mode, shadow::Solver &solver)
    : _solver(&solver), _evaluator(std::make_unique<Evaluator>(shadow_processing_mode, *_solver)),
      _whole_program_inputs(), _context(nullptr) {}

bool Executor::isWholeProgramInput(const std::string &flattened_name) const {
    return _whole_program_inputs.find(flattened_name) != _whole_program_inputs.end();
}

void Executor::execute(Context &context) {
    auto logger = spdlog::get("Simulator");
    SPDLOG_LOGGER_TRACE(logger, "Executing context: \n{}", context);

    State &state = context.getState();
    const Vertex &vertex = state.getVertex();
    const Frame &frame = context.getFrame();
    const Cfg &cfg = frame.getCfg();

    switch (vertex.getType()) {
        case Vertex::Type::ENTRY: {
            switch (cfg.getType()) {
                case Cfg::Type::PROGRAM: {
                    handleProgramEntryVertex(cfg, vertex, state);
                    break;
                }
                case Cfg::Type::FUNCTION_BLOCK: {
                    handleFunctionBlockEntryVertex(cfg, vertex, state);
                    break;
                }
                case Cfg::Type::FUNCTION: {
                    handleFunctionEntryVertex();
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected cfg type encountered.");
            }
            break;
        }
        case Vertex::Type::REGULAR: {
            handleRegularVertex(vertex, context);
            break;
        }
        case Vertex::Type::EXIT: {
            switch (cfg.getType()) {
                case Cfg::Type::PROGRAM: {
                    handleProgramExitVertex(frame, context, state);
                    break;
                }
                case Cfg::Type::FUNCTION_BLOCK: {
                    handleFunctionBlockExitVertex(frame, context, state);
                    break;
                }
                case Cfg::Type::FUNCTION: {
                    handleFunctionExitVertex();
                    break;
                }
                default:
                    throw std::runtime_error("Unexpected cfg type encountered.");
            }
            break;
        }
        default:
            throw std::runtime_error("Unexpected vertex type encountered.");
    }
}

void Executor::initialize(const Cfg &cfg) {
    const ir::Interface &interface = cfg.getInterface();
    for (auto it = interface.inputVariablesBegin(); it != interface.inputVariablesEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        _whole_program_inputs.emplace(std::move(flattened_name));
    }
}

void Executor::handleProgramEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state) {
    unsigned int label = vertex.getLabel();
    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
    assert(succeeding_labels.size() == 1);
    unsigned int next_label = succeeding_labels.at(0);
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

void Executor::handleFunctionBlockEntryVertex(const Cfg &cfg, const Vertex &vertex, State &state) {
    unsigned int label = vertex.getLabel();
    std::vector<unsigned int> succeeding_labels = cfg.getSucceedingLabels(label);
    assert(succeeding_labels.size() == 1);
    unsigned int next_label = succeeding_labels.at(0);
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

void Executor::handleFunctionEntryVertex() {
    throw std::logic_error("Not implemented yet.");
}

void Executor::handleRegularVertex(const Vertex &vertex, Context &context) {
    _context = &context;
    ir::Instruction *instruction = vertex.getInstruction();
    assert(instruction != nullptr);
    instruction->accept(*this);
}

void Executor::handleProgramExitVertex(const Frame &frame, Context &context, State &state) {
    unsigned int cycle = context.getCycle();
    unsigned int next_cycle = cycle + 1;
    const Cfg &cfg = frame.getCfg();

    // prepare "initial" valuations for the next cycle, relate "local" variables with each other and keep
    // "whole-program" input variables unconstrained
    for (auto it = cfg.flattenedInterfaceBegin(); it != cfg.flattenedInterfaceEnd(); ++it) {
        std::string flattened_name = it->getFullyQualifiedName();
        std::string contextualized_name = flattened_name + "__" + std::to_string(next_cycle);

        // distinguish between "whole-program" input variables and local/output variables
        if (isWholeProgramInput(flattened_name)) {
            // XXX overwritten/updated in engine
            state.setConcreteValuation(contextualized_name, _solver->makeRandomValue(it->getDataType()));
        } else {
            z3::expr concrete_valuation = state.getConcreteValuation(flattened_name + "__" + std::to_string(cycle));
            state.setConcreteValuation(contextualized_name, concrete_valuation);
        }
    }

    // update control-flow
    unsigned int next_label = frame.getReturnLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
    context.setCycle(next_cycle);
}

void Executor::handleFunctionBlockExitVertex(const Frame &frame, Context &context, State &state) {
    unsigned int next_label = frame.getReturnLabel();
    context.popFrame();
    const Frame &caller_frame = context.getFrame();
    const Cfg &caller_cfg = caller_frame.getCfg();
    const Vertex &next_vertex = caller_cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

void Executor::handleFunctionExitVertex() {
    throw std::logic_error("Not implemented yet.");
}

void Executor::visit(const ir::AssignmentInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();

    // evaluate rhs of the assignment
    z3::expr evaluated_expression = _evaluator->evaluate(expression, *_context);

    // encode lhs of the assignment
    const Frame &frame = _context->getFrame();
    const Cfg &cfg = frame.getCfg();
    const ir::VariableReference &variable_reference = instruction.getVariableReference();
    std::string name = variable_reference.getName();
    std::string flattened_name = frame.getScope() + "." + name;

    unsigned int cycle = _context->getCycle();
    std::string contextualized_name = flattened_name + "__" + std::to_string(cycle);

    // update concrete and symbolic valuations
    State &state = _context->getState();
    state.updateConcreteValuation(contextualized_name, evaluated_expression);

    // update control-flow
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Edge &edge = cfg.getIntraproceduralEdge(label);
    unsigned int next_label = edge.getTargetLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}

void Executor::visit(const ir::CallInstruction &instruction) {
    const Frame &frame = _context->getFrame();
    const Cfg &caller = frame.getCfg();

    State &state = _context->getState();
    const Vertex &vertex = state.getVertex();

    unsigned int label = vertex.getLabel();
    const Edge &intraprocedural_edge = caller.getIntraproceduralCallToReturnEdge(label);
    unsigned int return_label = intraprocedural_edge.getTargetLabel();
    const Edge &interprocedural_edge = caller.getInterproceduralCallEdge(label);
    unsigned int next_label = interprocedural_edge.getTargetLabel();

    const ir::VariableAccess &variable_access = instruction.getVariableAccess();
    std::shared_ptr<ir::Variable> variable = variable_access.getVariable();
    const ir::DataType &data_type = variable->getDataType();
    assert(data_type.getKind() == ir::DataType::Kind::DERIVED_TYPE);
    std::string type_representative_name = data_type.getName();
    const Cfg &callee = caller.getCfg(type_representative_name);
    std::string scope = frame.getScope() + "." + variable->getName();
    std::shared_ptr<Frame> callee_frame = std::make_shared<Frame>(callee, scope, return_label);
    _context->pushFrame(callee_frame);

    // update control-flow
    const Vertex &next_vertex = callee.getVertex(next_label);
    state.setVertex(next_vertex);
}

void Executor::visit(const ir::IfInstruction &instruction) {
    const ir::Expression &expression = instruction.getExpression();

    // evaluate condition concretely
    z3::expr evaluated_expression = _evaluator->evaluate(expression, *_context);
    assert(evaluated_expression.is_bool() && (evaluated_expression.is_true() || evaluated_expression.is_false()));

    // determine control-flow
    const Frame &frame = _context->getFrame();
    const Cfg &cfg = frame.getCfg();
    State &state = _context->getState();
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Edge &true_edge = cfg.getTrueEdge(label);
    unsigned int next_positive_label = true_edge.getTargetLabel();
    const Vertex &next_positive_vertex = cfg.getVertex(next_positive_label);
    const Edge &false_edge = cfg.getFalseEdge(label);
    unsigned int next_negative_label = false_edge.getTargetLabel();
    const Vertex &next_negative_vertex = cfg.getVertex(next_negative_label);

    if (evaluated_expression.is_true()) {
        state.setVertex(next_positive_vertex);
    } else {
        state.setVertex(next_negative_vertex);
    }
}

void Executor::visit(const ir::SequenceInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void Executor::visit(const ir::WhileInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void Executor::visit(const ir::GotoInstruction &instruction) {
    throw std::logic_error("Not implemented yet.");
}

void Executor::visit(const ir::HavocInstruction &instruction) {
    // encode lhs of the assignment
    const Frame &frame = _context->getFrame();
    const Cfg &cfg = frame.getCfg();
    const ir::VariableReference &variable_reference = instruction.getVariableReference();
    std::string name = variable_reference.getName();
    std::string flattened_name = frame.getScope() + "." + name;

    unsigned int cycle = _context->getCycle();
    std::string contextualized_name = flattened_name + "__" + std::to_string(cycle);

    std::shared_ptr<ir::Variable> variable = nullptr;
    if (const auto *variable_access = dynamic_cast<const ir::VariableAccess *>(&variable_reference)) {
        variable = variable_access->getVariable();
    } else if (const auto *field_access = dynamic_cast<const ir::FieldAccess *>(&variable_reference)) {
        variable = field_access->getVariableAccess().getVariable();
    } else {
        throw std::runtime_error("Unexpected variable reference type encountered.");
    }
    assert(variable != nullptr);
    const auto &data_type = variable->getDataType();
    z3::expr expression = _solver->makeConstant(contextualized_name, data_type);

    // update concrete valuation to a random, type-specific value
    State &state = _context->getState();
    state.updateConcreteValuation(contextualized_name, _solver->makeRandomValue(data_type));

    // update control-flow
    const Vertex &vertex = state.getVertex();
    unsigned int label = vertex.getLabel();
    const Edge &edge = cfg.getIntraproceduralEdge(label);
    unsigned int next_label = edge.getTargetLabel();
    const Vertex &next_vertex = cfg.getVertex(next_label);
    state.setVertex(next_vertex);
}