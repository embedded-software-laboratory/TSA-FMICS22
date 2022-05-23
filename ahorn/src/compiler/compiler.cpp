#include "compiler/compiler.h"
#include "compiler/analyzer.h"
#include "compiler/ast.h"
#include "compiler/statement/assert_statement.h"
#include "compiler/statement/assignment_statement.h"
#include "compiler/statement/assume_statement.h"
#include "compiler/statement/case_statement.h"
#include "compiler/statement/if_statement.h"
#include "compiler/statement/invocation_statement.h"
#include "compiler/statement/while_statement.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/constant/nondeterministic_constant.h"
#include "ir/expression/field_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/goto_instruction.h"
#include "ir/instruction/havoc_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "ir/instruction/while_instruction.h"
#include "ir/variable.h"

#include "IEC61131Lexer.h"

#include <memory>

using namespace ir;

std::unique_ptr<compiler::Project> Compiler::parse(const std::string &source_code) {
    // lexical analysis via ANTLR4
    antlr4::ANTLRInputStream input(source_code);
    IEC61131Lexer lexer(&input);
    antlr4::CommonTokenStream token_stream(&lexer);
    token_stream.fill();
    // syntax analysis via ANTLR4
    IEC61131Parser parser(&token_stream);
    antlr4::tree::ParseTree *source_code_context = parser.sourceCode();
    auto ast = std::make_unique<Ast>();
    // walk the parse tree
    auto project = std::unique_ptr<compiler::Project>(ast->visit(source_code_context).as<compiler::Project *>());
    // clear the parser
    parser.reset();
    return project;
}

std::unique_ptr<compiler::Project> Compiler::analyze(std::unique_ptr<compiler::Project> project) {
    auto semantic_analyzer = std::make_unique<Analyzer>(*project);
    semantic_analyzer->analyze();
    return project;
}

std::unique_ptr<Project> Compiler::compile(const std::string &source_code) {
    std::unique_ptr<compiler::Project> compiler_project = parse(source_code);
    auto analyzed_compiler_project = analyze(std::move(compiler_project));
    auto pou_compiler = std::make_unique<PouCompiler>(*analyzed_compiler_project);
    std::unique_ptr<Project> ir_project = pou_compiler->compile();
    return ir_project;
}

std::unique_ptr<Project> Compiler::PouCompiler::compile() {
    // compile each type representative pou into a module
    for (auto it = _project.pousBegin(); it != _project.pousEnd(); ++it) {
        compile(*it);
    }
    // build ir project
    return std::make_unique<Project>(std::move(_name_to_module));
}

void Compiler::PouCompiler::compile(const Pou &pou) {
    std::string fully_qualified_name = pou.getFullyQualifiedName();
    if (_name_to_module.count(fully_qualified_name))
        return;
    // XXX resolve compile dependency, assumption: call graph is acyclic (PLC-specific property!)
    // XXX the order of compilation does not matter but makes compilation easier
    variable_vector_t variables;
    module_ref_map_t name_to_module;// referenced modules are local to this particular module
    for (auto it = pou.getInterface().variablesBegin(); it != pou.getInterface().variablesEnd(); ++it) {
        if (it->getDataType().getKind() == DataType::Kind::DERIVED_TYPE) {
            // Variable : DataType, e.g., f : Fb1
            std::string variable_name = it->getName();
            std::string type_representative_name = it->getDataType().getFullyQualifiedName();
            // compile type representative pou
            const Pou &type_representative_pou = _project.getPou(type_representative_name);
            compile(type_representative_pou);
            const Module &module = *_name_to_module.at(type_representative_name);
            name_to_module.emplace(module.getFullyQualifiedName(), module);
        }
        // XXX share ownership
        variables.push_back(pou.getInterface().getVariable(it->getName()));
    }
    // XXX at this point, the interface is completely compiled and can be flattened, we pass module map by copy
    // XXX exposed as a member, used for the compilation of invocations
    _interface = std::make_unique<Interface>(std::move(variables));
    Module::Kind kind;
    switch (pou.getKind()) {
        case Pou::Kind::PROGRAM: {
            kind = Module::Kind::PROGRAM;
            break;
        }
        case Pou::Kind::FUNCTION_BLOCK: {
            kind = Module::Kind::FUNCTION_BLOCK;
            break;
        }
        case Pou::Kind::FUNCTION: {
            kind = Module::Kind::FUNCTION;
            break;
        }
        default: {
            throw std::logic_error("Invalid POU type.");
        }
    }
    int entry_label = _label++;
    auto &body = pou.getBody();
    for (auto it = body.statementListBegin(); it != body.statementListEnd(); ++it) {
        it->accept(*this);
    }
    int exit_label = _label++;
    auto module = std::make_unique<Module>(std::move(fully_qualified_name), kind, std::move(_interface),
                                           std::move(name_to_module), std::move(_label_to_instruction), entry_label,
                                           exit_label);
    _name_to_module.emplace(module->getFullyQualifiedName(), std::move(module));
}

void Compiler::PouCompiler::visit(AssertStatement &statement) {
    throw std::logic_error("Not implemented yet.");
}

void Compiler::PouCompiler::visit(AssignmentStatement &statement) {
    auto expression = statement.getExpression().clone();
    if (dynamic_cast<NondeterministicConstant *>(expression.get())) {
        _label_to_instruction.emplace(_label++,
                                      std::make_unique<HavocInstruction>(statement.getVariableReference().clone()));
    } else {
        _label_to_instruction.emplace(_label++, std::make_unique<AssignmentInstruction>(
                                                        statement.getVariableReference().clone(), expression->clone()));
    }
    if (_last_statement) {
        _label_to_instruction.emplace(_label++, std::make_unique<GotoInstruction>(_next_label_placeholders.top()));
    } else {
        int label = _label++;
        _label_to_instruction.emplace(label, std::make_unique<GotoInstruction>(_label));
    }
}

void Compiler::PouCompiler::visit(AssumeStatement &statement) {
    throw std::logic_error("Not implemented yet.");
}

void Compiler::PouCompiler::visit(IfStatement &statement) {
    bool last_statement = _last_statement;
    if (last_statement) {
        _last_statement = false;
    } else {
        _next_label_placeholders.push(--_next_label_placeholder);
    }
    std::vector<std::reference_wrapper<const ElseIfStatement>> non_empty_else_if_statements;
    // drop empty else if statements
    for (const auto &else_if_statement : statement.getElseIfStatements()) {
        if (!else_if_statement.second.empty()) {
            non_empty_else_if_statements.emplace_back(else_if_statement);
        }
    }
    if (!statement.hasThenStatements() && non_empty_else_if_statements.empty() && !statement.hasElseStatements()) {
        // XXX drop completely empty if-else-if-else statement
        if (!last_statement) {
            _next_label_placeholders.pop();
        }
        return;
    }
    if (statement.hasThenStatements()) {
        int if_label = _label++;
        // compile then statements
        auto goto_then = std::make_unique<GotoInstruction>(_label);
        for (auto it = statement.thenStatementsBegin(); it != std::prev(statement.thenStatementsEnd()); ++it) {
            it->accept(*this);
        }
        _last_statement = true;
        std::prev(statement.thenStatementsEnd())->accept(*this);
        _last_statement = false;
        if (non_empty_else_if_statements.empty()) {
            if (statement.hasElseStatements()) {
                auto goto_else = std::make_unique<GotoInstruction>(_label);
                _label_to_instruction.emplace(if_label,
                                              std::make_unique<IfInstruction>(statement.getExpression().clone(),
                                                                              goto_then->clone(), goto_else->clone()));
            } else {
                // target the same label as the last instruction of the compiled then statements
                if (auto *call_instruction =
                            dynamic_cast<CallInstruction *>(std::prev(_label_to_instruction.end())->second.get())) {
                    auto goto_else = std::make_unique<GotoInstruction>(call_instruction->getIntraproceduralLabel());
                    _label_to_instruction.emplace(
                            if_label, std::make_unique<IfInstruction>(statement.getExpression().clone(),
                                                                      goto_then->clone(), goto_else->clone()));
                } else if (auto *goto_of_last_instruction = dynamic_cast<GotoInstruction *>(
                                   std::prev(_label_to_instruction.end())->second.get())) {
                    auto goto_else = std::make_unique<GotoInstruction>(goto_of_last_instruction->getLabel());
                    _label_to_instruction.emplace(
                            if_label, std::make_unique<IfInstruction>(statement.getExpression().clone(),
                                                                      goto_then->clone(), goto_else->clone()));
                } else {
                    throw std::logic_error("Invalid instruction, expected call or goto.");
                }
            }
        } else {
            auto goto_else_if = std::make_unique<GotoInstruction>(_label);
            _label_to_instruction.emplace(if_label,
                                          std::make_unique<IfInstruction>(statement.getExpression().clone(),
                                                                          goto_then->clone(), goto_else_if->clone()));
        }
    }
    if (!non_empty_else_if_statements.empty()) {
        // compile non-empty else if statements
        auto const &else_if_statements = statement.getElseIfStatements();
        for (auto it = else_if_statements.begin(); it != else_if_statements.end(); ++it) {
            int else_if_label = _label++;
            auto &else_if_expression = it->first;
            auto goto_else_if_then = std::make_unique<GotoInstruction>(_label);
            for (auto it1 = std::begin(it->second); std::next(it1) != std::end(it->second); ++it1) {
                (*it1)->accept(*this);
            }
            _last_statement = true;
            it->second.back()->accept(*this);
            _last_statement = false;
            if (std::next(it) == else_if_statements.end() && !statement.hasElseStatements()) {
                // target the same label as the last instruction of the compiled else if statements
                auto *goto_of_last_instruction =
                        dynamic_cast<GotoInstruction *>(std::prev(_label_to_instruction.end())->second.get());
                assert(goto_of_last_instruction != nullptr);
                auto goto_else = std::make_unique<GotoInstruction>(goto_of_last_instruction->getLabel());
                _label_to_instruction.emplace(
                        else_if_label, std::make_unique<IfInstruction>(else_if_expression->clone(),
                                                                       goto_else_if_then->clone(), goto_else->clone()));
            } else {
                auto goto_else_if_else = std::make_unique<GotoInstruction>(_label);
                _label_to_instruction.emplace(else_if_label,
                                              std::make_unique<IfInstruction>(else_if_expression->clone(),
                                                                              goto_else_if_then->clone(),
                                                                              goto_else_if_else->clone()));
            }
        }
    }
    if (statement.hasElseStatements()) {
        int if_label = _label;
        if (!statement.hasThenStatements() && non_empty_else_if_statements.empty()) {
            _label++;
        }
        // compile else statements
        auto goto_else = std::make_unique<GotoInstruction>(_label);
        for (auto it = statement.elseStatementsBegin(); it != std::prev(statement.elseStatementsEnd()); ++it) {
            it->accept(*this);
        }
        _last_statement = true;
        std::prev(statement.elseStatementsEnd())->accept(*this);
        _last_statement = false;
        // XXX check if empty then and else-if
        if (!statement.hasThenStatements() && non_empty_else_if_statements.empty()) {
            // target the same label as the last instruction of the compiled else statements
            auto *goto_of_last_instruction =
                    dynamic_cast<GotoInstruction *>(std::prev(_label_to_instruction.end())->second.get());
            assert(goto_of_last_instruction != nullptr);
            // XXX important, take the expression of the last else if statement, as the if statement is empty and all
            // XXX preceding else if statements are also empty, but logically, we go into the else case of the last
            // XXX negated expression
            auto goto_then = std::make_unique<GotoInstruction>(goto_of_last_instruction->getLabel());
            // XXX check whether statement has else if statements, if not, we need to take the if expression
            if (statement.hasElseIfStatements()) {
                _label_to_instruction.emplace(
                        if_label, std::make_unique<IfInstruction>(statement.getElseIfStatements().back().first->clone(),
                                                                  goto_then->clone(), goto_else->clone()));
            } else {
                _label_to_instruction.emplace(if_label,
                                              std::make_unique<IfInstruction>(statement.getExpression().clone(),
                                                                              goto_then->clone(), goto_else->clone()));
            }
        }
    }
    if (!last_statement) {
        int next_label = _label;
        for (const auto &label_to_instruction : _label_to_instruction) {
            auto &instruction = label_to_instruction.second;
            if (auto *if_instruction = dynamic_cast<IfInstruction *>(instruction.get())) {
                auto &goto_then = if_instruction->getGotoThen();
                auto &goto_else = if_instruction->getGotoElse();
                if (goto_then.getLabel() == _next_label_placeholders.top()) {
                    goto_then.setLabel(next_label);
                }
                if (goto_else.getLabel() == _next_label_placeholders.top()) {
                    goto_else.setLabel(next_label);
                }
            }
            if (auto *while_instruction = dynamic_cast<WhileInstruction *>(instruction.get())) {
                auto &goto_while_exit = while_instruction->getGotoWhileExit();
                if (goto_while_exit.getLabel() == _next_label_placeholders.top()) {
                    goto_while_exit.setLabel(next_label);
                }
            }
            if (auto *call_instruction = dynamic_cast<CallInstruction *>(instruction.get())) {
                auto &goto_intraprocedural = call_instruction->getGotoIntraprocedural();
                if (goto_intraprocedural.getLabel() == _next_label_placeholders.top()) {
                    goto_intraprocedural.setLabel(next_label);
                }
            }
            if (auto *goto_instruction = dynamic_cast<GotoInstruction *>(instruction.get())) {
                if (goto_instruction->getLabel() == _next_label_placeholders.top()) {
                    goto_instruction->setLabel(next_label);
                }
            }
        }
        _next_label_placeholders.pop();
    } else {
        _last_statement = last_statement;
    }
}

/*
 * XXX due to call completion, i.e., adding additional pre- and post-assignments such that all variables are always
 * XXX initialized, the handling of nested calls is dealt with when compiling the post assignments.
 * XXX in case the call has no inputs or no outputs, we handle the target appropriately
 */
void Compiler::PouCompiler::visit(InvocationStatement &statement) {
    bool last_statement = _last_statement;
    if (last_statement) {
        // XXX ignore when building the pre-assignments of the call statement
        _last_statement = false;
    }
    // retrieve the callee module by name
    std::shared_ptr<Variable> variable = statement.getVariableAccess().getVariable();
    const Module &module = *_name_to_module.at(variable->getDataType().getFullyQualifiedName());
    // input assignment handling and call completion
    std::set<std::string> input_variable_names;
    for (auto it = module.getInterface().inputVariablesBegin(); it != module.getInterface().inputVariablesEnd(); ++it) {
        input_variable_names.insert(it->getName());
    }
    /*
     * XXX determine which inputs are set and augment by assignments to unchanged inputs
     * XXX we use a map to enforce the same order as in the interface (using built-in string ordering)
     * callee.x := caller.y
     */
    instruction_ref_map_t input_variable_name_to_pre_assignment_instruction;
    for (auto it = statement.preAssignmentsBegin(); it != statement.preAssignmentsEnd(); ++it) {
        const auto *field_access = dynamic_cast<const FieldAccess *>(&it->getVariableReference());
        assert(field_access != nullptr);
        const auto *variable_access = dynamic_cast<const VariableAccess *>(&field_access->getVariableReference());
        assert(variable_access != nullptr);
        std::string input_variable_name = variable_access->getVariable()->getName();
        assert(input_variable_names.count(input_variable_name) != 0);
        input_variable_names.erase(input_variable_name);
        // compile statement
        // XXX pre-fetch label, as "Goto" instructions are added for explicit control-flow
        int label = _label;
        it->accept(*this);
        auto *compiled_pre_assignment = dynamic_cast<AssignmentInstruction *>(_label_to_instruction.at(label).get());
        compiled_pre_assignment->setType(AssignmentInstruction::Type::PARAMETER_IN);
        input_variable_name_to_pre_assignment_instruction.emplace(input_variable_name, *compiled_pre_assignment);
    }
    for (const auto &input_variable_name : input_variable_names) {
        auto callee_input_variable = module.getInterface().getVariable(input_variable_name);
        // identity assignment for call interface completion
        auto identity_assignment = std::make_unique<AssignmentStatement>(
                std::make_unique<FieldAccess>(statement.getVariableAccess().clone(),
                                              std::make_unique<VariableAccess>(callee_input_variable)),
                std::make_unique<FieldAccess>(statement.getVariableAccess().clone(),
                                              std::make_unique<VariableAccess>(callee_input_variable)));
        int label = _label;
        identity_assignment->accept(*this);
        auto *compiled_pre_assignment = dynamic_cast<AssignmentInstruction *>(_label_to_instruction.at(label).get());
        compiled_pre_assignment->setType(AssignmentInstruction::Type::PARAMETER_IN);
        input_variable_name_to_pre_assignment_instruction.emplace(input_variable_name, *compiled_pre_assignment);
    }
    // debug
    /*
    std::cout << "Complete pre-assignment:" << std::endl;
    for (const auto &p : input_variable_name_to_pre_assignment_instruction) {
        std::cout << "Input variable " << p.first << " of callee"
                  << ": " << p.second << std::endl;
    }
    */
    int call_label = _label++;
    // check if call has outputs/post assignments
    if (module.getInterface().outputVariablesBegin() == module.getInterface().outputVariablesEnd()) {
        _last_statement = last_statement;
        int next_label = _last_statement ? _next_label_placeholders.top() : _label;
        auto goto_intraprocedural = std::make_unique<GotoInstruction>(next_label);
        auto goto_interprocedural = std::make_unique<GotoInstruction>(module.getEntryLabel());
        _label_to_instruction.emplace(call_label,
                                      std::make_unique<CallInstruction>(statement.getVariableAccess().clone(),
                                                                        std::move(goto_intraprocedural),
                                                                        std::move(goto_interprocedural)));
    } else {
        auto goto_intraprocedural = std::make_unique<GotoInstruction>(_label);
        // output assignment handling and call completion
        std::set<std::string> output_variable_names;
        for (auto it = module.getInterface().outputVariablesBegin(); it != module.getInterface().outputVariablesEnd();
             ++it) {
            output_variable_names.insert(it->getName());
        }
        // XXX we use a map to enforce the same order as in the interface (using built-in string ordering)
        // callee.x => caller.y, which was translated to caller.y := callee.x during parsing
        instruction_ref_map_t output_variable_name_to_post_assignment_instruction;
        for (auto it = statement.postAssignmentsBegin(); it != statement.postAssignmentsEnd(); ++it) {
            // retrieve the written variable of the caller, i.e., the variable occuring on the lhs of the call y = call(x)
            const auto &variable_reference = it->getVariableReference();
            const auto *variable_access = dynamic_cast<const VariableAccess *>(&variable_reference);
            assert(variable_access != nullptr);
            std::string variable_name = variable_access->getVariable()->getName();
            // XXX values are always returned by appropriately assigning the output variable of the callee
            const auto *field_access = dynamic_cast<const FieldAccess *>(&it->getExpression());
            assert(field_access != nullptr);
            assert(field_access->getRecordAccess().getVariable()->getName() == variable->getName());
            const auto *callee_variable_access =
                    dynamic_cast<const VariableAccess *>(&field_access->getVariableReference());
            assert(callee_variable_access != nullptr);
            std::string output_variable_name = callee_variable_access->getVariable()->getName();
            assert(output_variable_names.count(output_variable_name) != 0);
            output_variable_names.erase(output_variable_name);
            // compile statement
            // XXX pre-fetch label, as "Goto" instructions are added for explicit control-flow
            int label = _label;
            if (output_variable_names.empty() && std::next(it) == statement.postAssignmentsEnd()) {
                // XXX enable goto from nested call to target of parent in case this call is the last statement
                _last_statement = last_statement;
            }
            it->accept(*this);
            auto *compiled_post_assignment =
                    dynamic_cast<AssignmentInstruction *>(_label_to_instruction.at(label).get());
            compiled_post_assignment->setType(AssignmentInstruction::Type::PARAMETER_OUT);
            output_variable_name_to_post_assignment_instruction.emplace(variable_name, *compiled_post_assignment);
        }
        for (auto it = output_variable_names.begin(); it != output_variable_names.end(); ++it) {
            auto callee_output_variable = module.getInterface().getVariable(*it);
            // identity assignment for call interface completion
            auto identity_assignment = std::make_unique<AssignmentStatement>(
                    std::make_unique<FieldAccess>(statement.getVariableAccess().clone(),
                                                  std::make_unique<VariableAccess>(callee_output_variable)),
                    std::make_unique<FieldAccess>(statement.getVariableAccess().clone(),
                                                  std::make_unique<VariableAccess>(callee_output_variable)));
            int label = _label;
            if (std::next(it) == output_variable_names.end()) {
                // XXX enable goto from nested call to target of parent in case this call is the last statement
                _last_statement = last_statement;
            }
            identity_assignment->accept(*this);
            auto *compiled_post_assignment =
                    dynamic_cast<AssignmentInstruction *>(_label_to_instruction.at(label).get());
            compiled_post_assignment->setType(AssignmentInstruction::Type::PARAMETER_OUT);
            output_variable_name_to_post_assignment_instruction.emplace(variable->getName() + "." + *it,
                                                                        *compiled_post_assignment);
        }
        // debug
        /*
        std::cout << "Complete post-assignment:" << std::endl;
        for (const auto &p : output_variable_name_to_post_assignment_instruction) {
            std::cout << "Variable " << p.first << " of caller"
                      << ": " << p.second << std::endl;
        }
        */
        auto goto_interprocedural = std::make_unique<GotoInstruction>(module.getEntryLabel());
        _label_to_instruction.emplace(call_label,
                                      std::make_unique<CallInstruction>(statement.getVariableAccess().clone(),
                                                                        std::move(goto_intraprocedural),
                                                                        std::move(goto_interprocedural)));
    }
}

/*
 * We distinguish between while and if instructions.
 * | while header | <--------------------|
 *        |         \                    |
 *        |           \                  |
 *        |             | body entry |   |
 *        |                    |         |
 *        |                    |         |
 *        |                    v         |
 *        |             | body exit | ---|
 * | while exit |
 */
void Compiler::PouCompiler::visit(WhileStatement &statement) {
    bool last_statement = _last_statement;
    if (last_statement) {
        // XXX ignore when building the body of the while statement
        _last_statement = false;
    }
    if (statement.hasStatements()) {
        // always push a next label placeholder for the last statement of the body
        _next_label_placeholders.push(--_next_label_placeholder);
        int while_header = _label++;
        auto goto_body_entry = std::make_unique<GotoInstruction>(_label);
        for (auto it = statement.statementsBegin(); std::next(it) != statement.statementsEnd(); ++it) {
            it->accept(*this);
        }
        _last_statement = true;
        std::prev(statement.statementsEnd())->accept(*this);
        _last_statement = false;
        // XXX set all placeholders from within the body of the while to the while header
        int next_label = while_header;
        for (const auto &label_to_instruction : _label_to_instruction) {
            auto &instruction = label_to_instruction.second;
            if (auto *if_instruction = dynamic_cast<IfInstruction *>(instruction.get())) {
                auto &goto_then = if_instruction->getGotoThen();
                auto &goto_else = if_instruction->getGotoElse();
                if (goto_then.getLabel() == _next_label_placeholders.top()) {
                    goto_then.setLabel(next_label);
                }
                if (goto_else.getLabel() == _next_label_placeholders.top()) {
                    goto_else.setLabel(next_label);
                }
            }
            if (auto *while_instruction = dynamic_cast<WhileInstruction *>(instruction.get())) {
                auto &goto_while_exit = while_instruction->getGotoWhileExit();
                if (goto_while_exit.getLabel() == _next_label_placeholders.top()) {
                    goto_while_exit.setLabel(next_label);
                }
            }
            if (auto *call_instruction = dynamic_cast<CallInstruction *>(instruction.get())) {
                auto &goto_intraprocedural = call_instruction->getGotoIntraprocedural();
                if (goto_intraprocedural.getLabel() == _next_label_placeholders.top()) {
                    goto_intraprocedural.setLabel(next_label);
                }
            }
            if (auto *goto_instruction = dynamic_cast<GotoInstruction *>(instruction.get())) {
                if (goto_instruction->getLabel() == _next_label_placeholders.top()) {
                    goto_instruction->setLabel(next_label);
                }
            }
        }
        // pop the placeholder to correctly assign the target for the while header
        _next_label_placeholders.pop();
        // XXX if the while header is nested, we need to adjust the goto while exit to the target of the parent
        _last_statement = last_statement;
        next_label = _last_statement ? _next_label_placeholders.top() : _label;
        auto goto_while_exit = std::make_unique<GotoInstruction>(next_label);
        _label_to_instruction.emplace(while_header, std::make_unique<WhileInstruction>(
                                                            statement.getExpression().clone(),
                                                            std::move(goto_body_entry), std::move(goto_while_exit)));
    } else {
        // XXX drop completely empty while statement
        return;
    }
}

void Compiler::PouCompiler::visit(CaseStatement &statement) {
    const auto &cases = statement.getCases();
    std::unique_ptr<Expression> expression = cases.at(0).first->clone();
    std::vector<std::unique_ptr<Statement>> if_statements;
    for (const auto &it : cases.at(0).second) {
        if_statements.push_back(it->clone());
    }
    std::vector<ElseIfStatement> else_if_statements;
    for (std::vector<std::unique_ptr<IfStatement>>::size_type i = 1; i < cases.size(); ++i) {
        std::unique_ptr<Expression> else_if_expression = cases.at(i).first->clone();
        std::vector<std::unique_ptr<Statement>> statements;
        for (const auto &it : cases.at(i).second) {
            statements.push_back(it->clone());
        }
        else_if_statements.emplace_back(std::move(else_if_expression), std::move(statements));
    }
    std::vector<std::unique_ptr<Statement>> else_statements;
    for (auto it = statement.elseStatementsBegin(); it != statement.elseStatementsEnd(); ++it) {
        else_statements.push_back(it->clone());
    }
    auto lowered_if_statement = std::make_unique<IfStatement>(
            std::move(expression), std::move(if_statements), std::move(else_if_statements), std::move(else_statements));
    // compile lowered if statement
    lowered_if_statement->accept(*this);
}
