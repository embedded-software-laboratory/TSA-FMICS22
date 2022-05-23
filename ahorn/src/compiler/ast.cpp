#include "compiler/ast.h"
#include "compiler/project.h"
#include "compiler/statement/assert_statement.h"
#include "compiler/statement/assignment_statement.h"
#include "compiler/statement/assume_statement.h"
#include "compiler/statement/case_statement.h"
#include "compiler/statement/if_statement.h"
#include "compiler/statement/invocation_statement.h"
#include "compiler/statement/statement.h"
#include "compiler/statement/while_statement.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/change_expression.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/enumerated_value.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/expression/constant/nondeterministic_constant.h"
#include "ir/expression/constant/time_constant.h"
#include "ir/expression/expression.h"
#include "ir/expression/field_access.h"
#include "ir/expression/unary_expression.h"
#include "ir/expression/variable_access.h"
#include "ir/expression/variable_reference.h"
#include "ir/type/data_type.h"
#include "ir/type/derived_type.h"
#include "ir/type/elementary_type.h"
#include "ir/type/enumerated_type.h"
#include "ir/type/inconclusive_type.h"
#include "ir/type/safety_type.h"
#include "ir/type/simple_type.h"
#include "ir/variable.h"

#include "boost/tuple/tuple.hpp"

#include "spdlog/spdlog.h"

#include <regex>

using namespace ir;

antlrcpp::Any Ast::visitSourceCode(IEC61131Parser::SourceCodeContext *context) {
    // First pass, pre-compute all interfaces, begin with parsing all data types
    for (IEC61131Parser::DataTypeDeclarationContext *data_type_declaration_context : context->dataTypeDeclaration()) {
        std::vector<DataType *> data_types = visit(data_type_declaration_context).as<std::vector<DataType *>>();
        for (auto &data_type : data_types) {
            _name_to_data_type.emplace(data_type->getFullyQualifiedName(), std::unique_ptr<DataType>(data_type));
        }
    }
    if (context->programDeclaration()) {
        std::string program_name = visit(context->programDeclaration()->programName()).as<std::string>();
        _name_to_interface.emplace(
                std::move(program_name),
                std::unique_ptr<Interface>(visit(context->programDeclaration()->interface()).as<Interface *>()));
    }
    for (auto function_block_declaration_context : context->functionBlockDeclaration()) {
        std::string derived_function_block_name =
                visit(function_block_declaration_context->derivedFunctionBlockName()).as<std::string>();
        _name_to_interface.emplace(
                derived_function_block_name,
                std::unique_ptr<Interface>(visit(function_block_declaration_context->interface()).as<Interface *>()));
        _name_to_data_type.emplace(derived_function_block_name,
                                   std::make_unique<DerivedType>(derived_function_block_name));
    }
    // resolve inconclusive types
    for (auto &name_to_interface : _name_to_interface) {
        resolveInconclusiveTypes(*name_to_interface.second);
    }
    // Second pass: parse program and all function blocks
    if (context->programDeclaration()) {
        visit(context->programDeclaration());
    } else {
        SPDLOG_TRACE("Input does not contain a program.");
    }
    for (IEC61131Parser::FunctionBlockDeclarationContext *function_block_declaration_context :
         context->functionBlockDeclaration()) {
        visit(function_block_declaration_context);
    }
    for (IEC61131Parser::FunctionDeclarationContext *function_declaration_context : context->functionDeclaration()) {
        throw std::logic_error("Not implemented yet.");
    }
    // Third pass: populate usages to retrieve flattened interface and build pous in order
    noRecursionCheck();
    if (context->programDeclaration()) {
        std::string program_name = visit(context->programDeclaration()->programName()).as<std::string>();
        buildPou(program_name);
    }
    for (IEC61131Parser::FunctionBlockDeclarationContext *function_block_declaration_context :
         context->functionBlockDeclaration()) {
        std::string derived_function_block_name =
                visit(function_block_declaration_context->derivedFunctionBlockName()).as<std::string>();
        buildPou(derived_function_block_name);
    }
    // add pous to project
    for (auto &p : _name_to_pou) {
        resolveInconclusiveTypes(p.second->getInterface());
    }
    return new compiler::Project(std::move(_name_to_pou), std::move(_name_to_data_type));
}

void Ast::buildPou(const std::string &pou_name) {
    if (_name_to_pou.count(pou_name)) {
        return;
    }
    const auto &interface = _name_to_interface.at(pou_name);
    std::map<std::string, std::reference_wrapper<const Pou>> name_to_pou;
    for (auto variable = interface->variablesBegin(); variable != interface->variablesEnd(); ++variable) {
        if (variable->getDataType().getKind() == DataType::Kind::DERIVED_TYPE) {
            // Variable : DataType, e.g., f : Fb1
            std::string variable_name = variable->getName();
            std::string type_representative_name = variable->getDataType().getFullyQualifiedName();
            // compile type representative pou
            if (_name_to_pou.count(type_representative_name) == 0) {
                buildPou(type_representative_name);
            }
            const Pou &type_representative_pou = *_name_to_pou.at(type_representative_name);
            name_to_pou.emplace(type_representative_pou.getFullyQualifiedName(), type_representative_pou);
        }
    }
    _name_to_pou.emplace(pou_name,
                         std::make_unique<Pou>(pou_name, _name_to_kind.at(pou_name),
                                               std::move(_name_to_interface.at(pou_name)), std::move(name_to_pou),
                                               std::move(_name_to_body.at(pou_name))));
}


antlrcpp::Any Ast::visitProgramDeclaration(IEC61131Parser::ProgramDeclarationContext *context) {
    _pou_name = visit(context->programName()).as<std::string>();
    // XXX reuse pre-computed interface for parsing the body
    _name_to_kind.emplace(_pou_name, Pou::Kind::PROGRAM);
    _name_to_body.emplace(_pou_name, std::unique_ptr<Body>(visit(context->functionBlockBody()).as<Body *>()));
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitProgramName(IEC61131Parser::ProgramNameContext *context) {
    return context->IDENTIFIER()->getText();
}

antlrcpp::Any Ast::visitFunctionBlockDeclaration(IEC61131Parser::FunctionBlockDeclarationContext *context) {
    _pou_name = visit(context->derivedFunctionBlockName()).as<std::string>();
    // XXX reuse pre-computed interface for parsing the body
    _name_to_kind.emplace(_pou_name, Pou::Kind::FUNCTION_BLOCK);
    _name_to_body.emplace(_pou_name, std::unique_ptr<Body>(visit(context->functionBlockBody()).as<Body *>()));
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitFunctionBlockName(IEC61131Parser::FunctionBlockNameContext *context) {
    return context->IDENTIFIER()->getText();
}

antlrcpp::Any Ast::visitInterface(IEC61131Parser::InterfaceContext *context) {
    std::vector<std::shared_ptr<Variable>> variables;
    for (auto input_variable_declarations_context : context->inputVariableDeclarations()) {
        std::vector<Variable *> input_variables =
                visit(input_variable_declarations_context).as<std::vector<Variable *>>();
        for (Variable *variable : input_variables) {
            variables.push_back(std::shared_ptr<Variable>(variable));
        }
    }
    for (auto local_variable_declarations_context : context->localVariableDeclarations()) {
        std::vector<Variable *> local_variables =
                visit(local_variable_declarations_context).as<std::vector<Variable *>>();
        for (Variable *variable : local_variables) {
            variables.push_back(std::shared_ptr<Variable>(variable));
        }
    }
    for (auto output_variable_declarations_context : context->outputVariableDeclarations()) {
        std::vector<Variable *> output_variables =
                visit(output_variable_declarations_context).as<std::vector<Variable *>>();
        for (Variable *variable : output_variables) {
            variables.push_back(std::shared_ptr<Variable>(variable));
        }
    }
    for (auto temporary_variable_declarations_context : context->temporaryVariableDeclarations()) {
        std::vector<Variable *> temporary_variables =
                visit(temporary_variable_declarations_context).as<std::vector<Variable *>>();
        for (Variable *variable : temporary_variables) {
            variables.push_back(std::shared_ptr<Variable>(variable));
        }
    }
    // TODO: create type-dependent initial values
    return new Interface(std::move(variables));
}

antlrcpp::Any Ast::visitInputVariableDeclarations(IEC61131Parser::InputVariableDeclarationsContext *context) {
    std::vector<Variable *> input_variables;
    for (IEC61131Parser::VariableDeclarationInitializationContext *variable_declaration_initialization_context :
         context->variableDeclarationInitialization()) {
        std::vector<Variable *> variables =
                visit(variable_declaration_initialization_context).as<std::vector<Variable *>>();
        for (auto variable : variables) {
            input_variables.push_back(variable);
        }
    }
    return input_variables;
}

antlrcpp::Any Ast::visitLocalVariableDeclarations(IEC61131Parser::LocalVariableDeclarationsContext *context) {
    std::vector<Variable *> local_variables;
    for (IEC61131Parser::VariableDeclarationInitializationContext *variable_declaration_initialization_context :
         context->variableDeclarationInitialization()) {
        std::vector<Variable *> variables =
                std::move(visit(variable_declaration_initialization_context)).as<std::vector<Variable *>>();
        for (auto variable : variables) {
            local_variables.push_back(variable);
        }
    }
    return local_variables;
}

antlrcpp::Any Ast::visitOutputVariableDeclarations(IEC61131Parser::OutputVariableDeclarationsContext *context) {
    std::vector<Variable *> output_variables;
    for (IEC61131Parser::VariableDeclarationInitializationContext *variable_declaration_initialization_context :
         context->variableDeclarationInitialization()) {
        std::vector<Variable *> variables =
                visit(variable_declaration_initialization_context).as<std::vector<Variable *>>();
        for (auto variable : variables) {
            output_variables.push_back(variable);
        }
    }
    return output_variables;
}

antlrcpp::Any Ast::visitTemporaryVariableDeclarations(IEC61131Parser::TemporaryVariableDeclarationsContext *context) {
    std::vector<Variable *> temporary_variables;
    for (IEC61131Parser::VariableDeclarationInitializationContext *variable_declaration_initialization_context :
         context->variableDeclarationInitialization()) {
        std::vector<Variable *> variables =
                visit(variable_declaration_initialization_context).as<std::vector<Variable *>>();
        for (auto variable : variables) {
            temporary_variables.push_back(variable);
        }
    }
    return temporary_variables;
}

antlrcpp::Any Ast::visitVariableName(IEC61131Parser::VariableNameContext *context) {
    return context->IDENTIFIER()->getText();
}

antlrcpp::Any Ast::visitElementaryTypeName(IEC61131Parser::ElementaryTypeNameContext *context) {
    if (context->numericTypeName()) {
        std::string numeric_type_name = visit(context->numericTypeName()).as<std::string>();
        return new ElementaryType(std::move(numeric_type_name));
    } else if (context->bitStringTypeName()) {
        std::string bit_string_type_name = visit(context->bitStringTypeName()).as<std::string>();
        return new ElementaryType(std::move(bit_string_type_name));
    } else if (context->booleanTypeName()) {
        std::string boolean_type_name = visit(context->booleanTypeName()).as<std::string>();
        return new ElementaryType(std::move(boolean_type_name));
    } else if (context->timeTypeName()) {
        std::string time_type_name = visit(context->timeTypeName()).as<std::string>();
        return new ElementaryType(std::move(time_type_name));
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitNumericTypeName(IEC61131Parser::NumericTypeNameContext *context) {
    if (context->integerTypeName()) {
        return visit(context->integerTypeName());
    } else if (context->realTypeName()) {
        return visit(context->realTypeName());
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitIntegerTypeName(IEC61131Parser::IntegerTypeNameContext *context) {
    if (context->unsignedIntegerTypeName()) {
        return visit(context->unsignedIntegerTypeName());
    } else if (context->signedIntegerTypeName()) {
        return visit(context->signedIntegerTypeName());
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitUnsignedIntegerTypeName(IEC61131Parser::UnsignedIntegerTypeNameContext *context) {
    if (context->UINT()) {
        return context->UINT()->getText();
    } else if (context->USINT()) {
        return context->USINT()->getText();
    } else if (context->UDINT()) {
        return context->UDINT()->getText();
    } else if (context->ULINT()) {
        return context->ULINT()->getText();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitSignedIntegerTypeName(IEC61131Parser::SignedIntegerTypeNameContext *context) {
    if (context->INT()) {
        return context->INT()->getText();
    } else if (context->LINT()) {
        return context->LINT()->getText();
    } else if (context->DINT()) {
        return context->DINT()->getText();
    } else if (context->SINT()) {
        return context->SINT()->getText();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitRealTypeName(IEC61131Parser::RealTypeNameContext *context) {
    if (context->REAL()) {
        return context->REAL()->getText();
    } else if (context->LREAL()) {
        return context->LREAL()->getText();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitBitStringTypeName(IEC61131Parser::BitStringTypeNameContext *context) {
    if (context->BYTE()) {
        return context->BYTE()->getText();
    } else if (context->WORD()) {
        return context->WORD()->getText();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitBooleanTypeName(IEC61131Parser::BooleanTypeNameContext *context) {
    if (context->BOOL()) {
        return context->BOOL()->getText();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitTimeTypeName(IEC61131Parser::TimeTypeNameContext *context) {
    if (context->TIME()) {
        return context->TIME()->getText();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitDerivedTypeName(IEC61131Parser::DerivedTypeNameContext *context) {
    return context->IDENTIFIER()->getText();
}

antlrcpp::Any Ast::visitSafetyTypeName(IEC61131Parser::SafetyTypeNameContext *context) {
    if (context->safetyBooleanTypeName()) {
        std::string safety_boolean_type_name = visit(context->safetyBooleanTypeName()).as<std::string>();
        return new SafetyType(std::move(safety_boolean_type_name));
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitSafetyBooleanTypeName(IEC61131Parser::SafetyBooleanTypeNameContext *context) {
    if (context->SAFEBOOL()) {
        return context->SAFEBOOL()->getText();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitStatement(IEC61131Parser::StatementContext *context) {
    if (context->assignmentStatement()) {
        return (Statement *) visit(context->assignmentStatement()).as<AssignmentStatement *>();
    } else if (context->assertStatement()) {
        return (Statement *) visit(context->assertStatement()).as<AssertStatement *>();
    } else if (context->assumeStatement()) {
        return (Statement *) visit(context->assumeStatement()).as<AssumeStatement *>();
    } else if (context->iterationStatement()) {
        return (Statement *) visit(context->iterationStatement()).as<IterationStatement *>();
    } else if (context->selectionStatement()) {
        return (Statement *) visit(context->selectionStatement()).as<SelectionStatement *>();
    } else if (context->subprogramControlStatement()) {
        return (Statement *) visit(context->subprogramControlStatement()).as<InvocationStatement *>();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitAssignmentStatement(IEC61131Parser::AssignmentStatementContext *context) {
    std::unique_ptr<VariableReference> variable_reference =
            std::unique_ptr<VariableReference>(visit(context->variableReference()).as<VariableReference *>());
    if (context->expression()) {
        Expression *expression = visit(context->expression()).as<Expression *>();
        return new AssignmentStatement(std::move(variable_reference), std::unique_ptr<Expression>(expression));
    } else if (context->nondeterministicConstant()) {
        Expression *expression = visit(context->nondeterministicConstant()).as<Expression *>();
        return new AssignmentStatement(std::move(variable_reference), std::unique_ptr<Expression>(expression));
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitNondeterministicConstant(IEC61131Parser::NondeterministicConstantContext *context) {
    if (context->NONDETERMINISTIC_BOOL()) {
        return (Expression *) new NondeterministicConstant(Expression::Type::BOOLEAN);
    } else if (context->NONDETERMINISTIC_INT()) {
        return (Expression *) new NondeterministicConstant(Expression::Type::ARITHMETIC);
    } else if (context->NONDETERMINISTIC_TIME()) {
        return (Expression *) new NondeterministicConstant(Expression::Type::ARITHMETIC);
    } else {
        throw std::logic_error("Invalid nondeterministic constant.");
    }
}

antlrcpp::Any Ast::visitAssumeStatement(IEC61131Parser::AssumeStatementContext *context) {
    Expression *expression = visit(context->expression()).as<Expression *>();
    return new AssumeStatement(std::unique_ptr<Expression>(expression));
}

antlrcpp::Any Ast::visitAssertStatement(IEC61131Parser::AssertStatementContext *context) {
    Expression *expression = visit(context->expression()).as<Expression *>();
    return new AssertStatement(std::unique_ptr<Expression>(expression));
}

antlrcpp::Any Ast::visitSubprogramControlStatement(IEC61131Parser::SubprogramControlStatementContext *context) {
    if (context->invocationStatement()) {
        return visit(context->invocationStatement());
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitInvocationStatement(IEC61131Parser::InvocationStatementContext *context) {
    // TODO: if we want to support functions in the future, they are indistinguishable during parsing from
    //  invocations. As invoked function blocks always require the caller to have the appropriate variable
    //  declaration, we can determine whether it is a function or a function block by checking for containment of a
    //  function block in the callers interface.
    std::string function_block_instance_name = visit(context->functionBlockInstanceName()).as<std::string>();
    _callee_variable_name = function_block_instance_name;
    std::vector<std::unique_ptr<AssignmentStatement>> pre_assignments;
    std::vector<std::unique_ptr<AssignmentStatement>> post_assignments;
    for (IEC61131Parser::ParameterAssignmentContext *parameter_assignment_context : context->parameterAssignment()) {
        if (parameter_assignment_context->pre_assignment) {
            AssignmentStatement *assignment_statement = visit(parameter_assignment_context).as<AssignmentStatement *>();
            pre_assignments.push_back(std::unique_ptr<AssignmentStatement>(assignment_statement));
        } else if (parameter_assignment_context->post_assignment) {
            AssignmentStatement *assignment_statement = visit(parameter_assignment_context).as<AssignmentStatement *>();
            post_assignments.push_back(std::unique_ptr<AssignmentStatement>(assignment_statement));
        } else {
            throw std::logic_error("Not implemented yet.");
        }
    }
    if (context->DOT()) {
        auto variable_reference =
                std::unique_ptr<VariableReference>(visit(context->variableReference()).as<VariableReference *>());
        throw std::logic_error("Not implemented yet.");
    }
    auto function_block_variable = _name_to_interface.at(_pou_name)->getVariable(function_block_instance_name);
    return new InvocationStatement(std::make_unique<VariableAccess>(function_block_variable),
                                   std::move(pre_assignments), std::move(post_assignments));
}

antlrcpp::Any Ast::visitFunctionBlockInstanceName(IEC61131Parser::FunctionBlockInstanceNameContext *context) {
    return context->IDENTIFIER()->getText();
}

antlrcpp::Any Ast::visitParameterAssignment(IEC61131Parser::ParameterAssignmentContext *context) {
    // retrieve the callee variable in the parent's scope
    auto callee_variable = _name_to_interface.at(_pou_name)->getVariable(_callee_variable_name);
    if (context->ASSIGNMENT()) {
        std::string callee_input_variable_name = visit(context->calleeInputVariable).as<std::string>();
        // retrieve callee input variable from the interface of the callee
        std::string type_representative_name = callee_variable->getDataType().getName();
        auto callee_input_variable =
                _name_to_interface.at(type_representative_name)->getVariable(callee_input_variable_name);
        auto field_access = std::make_unique<FieldAccess>(std::make_unique<VariableAccess>(callee_variable),
                                                          std::make_unique<VariableAccess>(callee_input_variable));
        Expression *expression = visit(context->expression()).as<Expression *>();
        return new AssignmentStatement(std::move(field_access), std::unique_ptr<Expression>(expression));
    } else if (context->ARROW_RIGHT()) {
        // z => x
        // - - - > x := Fb.z;
        std::string callee_output_variable_name = visit(context->calleeOutputVariable).as<std::string>();
        // retrieve callee output variable from the interface of the callee
        std::string type_representative_name = callee_variable->getDataType().getName();
        auto callee_output_variable =
                _name_to_interface.at(type_representative_name)->getVariable(callee_output_variable_name);
        auto field_access = std::make_unique<FieldAccess>(std::make_unique<VariableAccess>(callee_variable),
                                                          std::make_unique<VariableAccess>(callee_output_variable));
        std::string caller_variable_name = visit(context->callerVariable).as<std::string>();
        auto caller_variable = _name_to_interface.at(_pou_name)->getVariable(caller_variable_name);
        return new AssignmentStatement(std::make_unique<VariableAccess>(caller_variable), std::move(field_access));
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitSelectionStatement(IEC61131Parser::SelectionStatementContext *context) {
    if (context->ifStatement()) {
        return (SelectionStatement *) visit(context->ifStatement()).as<IfStatement *>();
    } else if (context->caseStatement()) {
        return (SelectionStatement *) visit(context->caseStatement()).as<CaseStatement *>();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

/* ifStatement:
    IF ifCondition=expression THEN
        (ifStatementList=statementList)
    (ELSIF elseIfConditions+=expression THEN
        elseIfStatementLists+=statementList)*
    (ELSE
        (elseStatementList=statementList)
    )?
    END_IF;
*/
antlrcpp::Any Ast::visitIfStatement(IEC61131Parser::IfStatementContext *context) {
    /*
    int start_line = context->getStart()->getLine();
    int start_column = context->getStart()->getCharPositionInLine();
    int stop_line = context->getStop()->getLine();
    int stop_column = context->getStop()->getCharPositionInLine();
    */
    std::unique_ptr<Expression> expression =
            std::unique_ptr<Expression>(visit(context->ifCondition).as<Expression *>());
    std::vector<std::unique_ptr<Statement>> if_statements;
    std::vector<Statement *> if_statement_list = visit(context->ifStatementList).as<std::vector<Statement *>>();
    if_statements.reserve(if_statement_list.size());
    for (auto *statement : if_statement_list) {
        if_statements.push_back(std::unique_ptr<Statement>(statement));
    }
    std::vector<ElseIfStatement> else_if_statements;
    for (std::vector<IEC61131Parser::StatementListContext *>::size_type i = 0; i < context->elseIfStatementLists.size();
         ++i) {
        auto else_if_expression =
                std::unique_ptr<Expression>(visit(context->elseIfConditions.at(i)).as<Expression *>());
        std::vector<Statement *> statements = visit(context->elseIfStatementLists.at(i)).as<std::vector<Statement *>>();
        std::vector<std::unique_ptr<Statement>> unique_statements;
        unique_statements.reserve(statements.size());
        for (Statement *statement : statements) {
            unique_statements.push_back(std::unique_ptr<Statement>(statement));
        }
        else_if_statements.emplace_back(std::move(else_if_expression), std::move(unique_statements));
    }
    // check if non-empty else statement list
    std::vector<std::unique_ptr<Statement>> else_statements;
    if (context->elseStatementList) {
        std::vector<Statement *> else_statement_list = visit(context->elseStatementList).as<std::vector<Statement *>>();
        for (auto *statement : else_statement_list) {
            else_statements.push_back(std::unique_ptr<Statement>(statement));
        }
    } else {
        // do nothing, we allow empty else statement list
    }
    return new IfStatement(std::move(expression), std::move(if_statements), std::move(else_if_statements),
                           std::move(else_statements));
}

antlrcpp::Any Ast::visitStatementList(IEC61131Parser::StatementListContext *context) {
    std::vector<Statement *> statements;
    for (IEC61131Parser::StatementContext *statement_context : context->statement()) {
        statements.push_back(visit(statement_context).as<Statement *>());
    }
    return statements;
}

antlrcpp::Any Ast::visitIterationStatement(IEC61131Parser::IterationStatementContext *context) {
    if (context->whileStatement()) {
        return (IterationStatement *) visit(context->whileStatement()).as<WhileStatement *>();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitWhileStatement(IEC61131Parser::WhileStatementContext *context) {
    std::unique_ptr<Expression> expression =
            std::unique_ptr<Expression>(visit(context->expression()).as<Expression *>());
    std::vector<std::unique_ptr<Statement>> while_statements;
    std::vector<Statement *> statements = visit(context->statementList()).as<std::vector<Statement *>>();
    while_statements.reserve(statements.size());
    for (auto *statement : statements) {
        while_statements.push_back(std::unique_ptr<Statement>(statement));
    }
    return new WhileStatement(std::move(expression), std::move(while_statements));
}

antlrcpp::Any Ast::visitBinaryExpression(IEC61131Parser::BinaryExpressionContext *context) {
    std::unique_ptr<Expression> left_operand =
            std::unique_ptr<Expression>(visit(context->leftOperand).as<Expression *>());
    std::unique_ptr<Expression> right_operand =
            std::unique_ptr<Expression>(visit(context->rightOperand).as<Expression *>());
    if (context->LESS_THAN()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::LESS_THAN, std::move(left_operand),
                                                   std::move(right_operand));
    } else if (context->GREATER_THAN()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::GREATER_THAN,
                                                   std::move(left_operand), std::move(right_operand));
    } else if (context->LESS_THAN_OR_EQUAL_TO()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::LESS_THAN_OR_EQUAL_TO,
                                                   std::move(left_operand), std::move(right_operand));
    } else if (context->GREATER_THAN_OR_EQUAL_TO()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::GREATER_THAN_OR_EQUAL_TO,
                                                   std::move(left_operand), std::move(right_operand));
    } else if (context->PLUS()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::ADD, std::move(left_operand),
                                                   std::move(right_operand));
    } else if (context->MINUS()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::SUBTRACT, std::move(left_operand),
                                                   std::move(right_operand));
    } else if (context->MULTIPLY()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::MULTIPLY, std::move(left_operand),
                                                   std::move(right_operand));
    } else if (context->DIVIDE()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::DIVIDE, std::move(left_operand),
                                                   std::move(right_operand));
    } else if (context->EQUALITY()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::EQUALITY, std::move(left_operand),
                                                   std::move(right_operand));
    } else if (context->INEQUALITY()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::INEQUALITY,
                                                   std::move(left_operand), std::move(right_operand));
    } else if (context->BOOLEAN_AND()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::BOOLEAN_AND,
                                                   std::move(left_operand), std::move(right_operand));
    } else if (context->BOOLEAN_EXCLUSIVE_OR()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::BOOLEAN_EXCLUSIVE_OR,
                                                   std::move(left_operand), std::move(right_operand));
    } else if (context->BOOLEAN_OR()) {
        return (Expression *) new BinaryExpression(BinaryExpression::BinaryOperator::BOOLEAN_OR,
                                                   std::move(left_operand), std::move(right_operand));
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitUnaryExpression(IEC61131Parser::UnaryExpressionContext *context) {
    Expression *expression = visit(context->primaryExpression()).as<Expression *>();
    if (context->PLUS()) {
        throw std::logic_error("Not implemented yet.");
    } else if (context->MINUS()) {
        return (Expression *) new UnaryExpression(UnaryExpression::UnaryOperator::NEGATION,
                                                  std::unique_ptr<Expression>(expression));
    } else if (context->COMPLEMENT()) {
        return (Expression *) new UnaryExpression(UnaryExpression::UnaryOperator::COMPLEMENT,
                                                  std::unique_ptr<Expression>(expression));
    } else {
        return expression;
    }
}

antlrcpp::Any Ast::visitPrimaryExpression(IEC61131Parser::PrimaryExpressionContext *context) {
    if (context->expression()) {
        return visit(context->expression());
    } else if (context->variableReference()) {
        return (Expression *) visit(context->variableReference()).as<VariableReference *>();
    } else if (context->constant()) {
        return (Expression *) visit(context->constant()).as<Constant *>();
    } else if (context->enumeratedValue()) {
        // enumeratedValue matches IDENTIFIER, i.e., it can also match to a variableReference. In order to resolve
        // the ambiguity, we can check whether a variable with the name exists or not. the rule has to be kept
        // though, because a variableReference does not match IDENTIFIER#IDENTIFIER, which is an explicitly
        // typed enumeratedValue
        std::string identifier = context->enumeratedValue()->IDENTIFIER()->getText();
        const auto &interface = _name_to_interface.at(_pou_name);
        auto it = std::find_if(
                interface->variablesBegin(), interface->variablesEnd(),
                [identifier](const Variable &variable) { return variable.getFullyQualifiedName() == identifier; });
        if (it != interface->variablesEnd()) {
            // enumeratedValue is ambiguous and actually should represent a variable
            // spdlog::trace("Identifier {} in primary expression for context->enumeratedValue() is ambiguous and should "
            //              "actually represent a variable",
            //              identifier);
            // the variable can only be a variable access, because a field access is correctly distinguished from an
            // enumerated value due to the "DOT"
            return (Expression *) new VariableAccess(interface->getVariable(identifier));
        } else {
            // the identifier indeed represents an enumeratedValue
            // XXX we can not set the index value of this enumerated_value at this point, as there can be multiple
            // XXX enums with the same value names, we need to distinguish it in the context of the expression, i.e.,
            // XXX usually in comparison with the variable, else we can only guess if there are multiple defined enum
            // XXX values
            auto *enumerated_value = visit(context->enumeratedValue()).as<EnumeratedValue *>();
            return (Expression *) enumerated_value;
        }
    } else if (context->functionCall()) {
        throw std::logic_error("Not implemented yet.");
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitConstant(IEC61131Parser::ConstantContext *context) {
    if (context->numericLiteral()) {
        return visit(context->numericLiteral());
    } else if (context->timeLiteral()) {
        return (Constant *) visit(context->timeLiteral()).as<TimeConstant *>();
    } else if (context->booleanLiteral()) {
        return (Constant *) visit(context->booleanLiteral()).as<BooleanConstant *>();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitNumericLiteral(IEC61131Parser::NumericLiteralContext *context) {
    if (context->integerLiteral()) {
        return (Constant *) visit(context->integerLiteral()).as<IntegerConstant *>();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitIntegerLiteral(IEC61131Parser::IntegerLiteralContext *context) {
    if (context->integerTypeName()) {
        spdlog::warn("context->integerTypeName() is currently ignored...");
    }
    if (context->signedInteger()) {
        int value = visit(context->signedInteger()).as<int>();
        return new IntegerConstant(value);
    } else if (context->HEXADECIMAL_INTEGER()) {
        std::string string_hex_value = (context->HEXADECIMAL_INTEGER()->toString()).replace(0, 3, "0x");
        int value = std::stoi(string_hex_value, nullptr, 16);
        return new IntegerConstant(value);
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitSignedInteger(IEC61131Parser::SignedIntegerContext *context) {
    int unsigned_integer = visit(context->unsignedInteger()).as<int>();
    if (context->MINUS() || context->PLUS()) {
        throw std::logic_error("Not implemented yet.");
    } else {
        return unsigned_integer;
    }
}

antlrcpp::Any Ast::visitUnsignedInteger(IEC61131Parser::UnsignedIntegerContext *context) {
    return std::stoi(context->UNSIGNED_INTEGER()->getText());
}

antlrcpp::Any Ast::visitBooleanLiteral(IEC61131Parser::BooleanLiteralContext *context) {
    if (context->booleanTypeName()) {
        throw std::logic_error("Not implemented yet.");
    }
    if (context->FALSE()) {
        return new BooleanConstant(false);
    } else if (context->TRUE()) {
        return new BooleanConstant(true);
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitFunctionDeclaration(IEC61131Parser::FunctionDeclarationContext *context) {
    throw std::logic_error("Not implemented yet.");
}

antlrcpp::Any Ast::visitFunctionName(IEC61131Parser::FunctionNameContext *context) {
    throw std::logic_error("Not implemented yet.");
}

antlrcpp::Any Ast::visitChangeExpression(IEC61131Parser::ChangeExpressionContext *context) {
    std::unique_ptr<Expression> old_operand =
            std::unique_ptr<Expression>(visit(context->oldOperand).as<Expression *>());
    std::unique_ptr<Expression> new_operand =
            std::unique_ptr<Expression>(visit(context->newOperand).as<Expression *>());
    return (Expression *) new ChangeExpression(std::move(old_operand), std::move(new_operand));
}

antlrcpp::Any Ast::visitVariableReference(IEC61131Parser::VariableReferenceContext *context) {
    if (context->variableAccess()) {
        return (VariableReference *) visit(context->variableAccess()).as<VariableAccess *>();
    } else if (context->fieldAccess()) {
        return (VariableReference *) visit(context->fieldAccess()).as<FieldAccess *>();
    } else {
        throw std::logic_error("Invalid context, this case should be unreachable.");
    }
}

antlrcpp::Any Ast::visitVariableAccess(IEC61131Parser::VariableAccessContext *context) {
    std::string variable_name = visit(context->variableName()).as<std::string>();
    const auto &interface = _name_to_interface.at(_pou_name);
    auto it = std::find_if(
            interface->variablesBegin(), interface->variablesEnd(),
            [variable_name](const Variable &variable) { return variable.getFullyQualifiedName() == variable_name; });
    if (it != interface->variablesEnd()) {
        return new VariableAccess(interface->getVariable(variable_name));
    } else {
        throw std::logic_error("Variable with variable name " + variable_name + " has not been defined in this POU.");
    }
}

antlrcpp::Any Ast::visitFieldAccess(IEC61131Parser::FieldAccessContext *context) {
    // PPU.Stack.x.y.z
    // TODO: make sure, that the variable reference is taken from the interface of the record access, and not from
    //  the current interface.
    VariableAccess *record_access = visit(context->recordAccess()).as<VariableAccess *>();
    // temporarily save current pou name
    std::string saved_pou_name = _pou_name;
    // overwrite pou name to the name of the pou where the record variable is residing in
    _pou_name = record_access->getVariable()->getDataType().getName();
    VariableReference *variable_reference = visit(context->variableReference()).as<VariableReference *>();
    // reset pou name to save pou name
    _pou_name = saved_pou_name;
    return new FieldAccess(std::unique_ptr<VariableAccess>(record_access),
                           std::unique_ptr<VariableReference>(variable_reference));
}

antlrcpp::Any Ast::visitRecordAccess(IEC61131Parser::RecordAccessContext *context) {
    return visit(context->variableAccess()).as<VariableAccess *>();
}

antlrcpp::Any Ast::visitCaseStatement(IEC61131Parser::CaseStatementContext *context) {
    std::unique_ptr<Expression> expression =
            std::unique_ptr<Expression>(visit(context->expression()).as<Expression *>());
    std::vector<ElseIfStatement> cases;
    for (IEC61131Parser::CaseSelectionContext *case_selection_context : context->caseSelection()) {
        std::pair<std::vector<Expression *>, std::vector<Statement *>> raw_case =
                visit(case_selection_context).as<std::pair<std::vector<Expression *>, std::vector<Statement *>>>();
        std::vector<std::unique_ptr<Expression>> unique_expressions;
        unique_expressions.reserve(raw_case.first.size());
        for (Expression *expr : raw_case.first) {
            unique_expressions.push_back(std::unique_ptr<Expression>(expr));
        }
        std::vector<std::unique_ptr<Statement>> unique_statements;
        unique_statements.reserve(raw_case.second.size());
        for (Statement *statement : raw_case.second) {
            unique_statements.push_back(std::unique_ptr<Statement>(statement));
        }
        auto compiled_case_expression = compileCaseExpression(*expression, unique_expressions);
        cases.emplace_back(std::move(compiled_case_expression), std::move(unique_statements));
    }
    std::vector<std::unique_ptr<Statement>> else_statements;
    if (context->statementList()) {
        std::vector<Statement *> statements = visit(context->statementList()).as<std::vector<Statement *>>();
        else_statements.reserve(statements.size());
        for (auto *statement : statements) {
            else_statements.push_back(std::unique_ptr<Statement>(statement));
        }
    } else {
        // do nothing, we allow empty else cases
    }
    return new CaseStatement(std::move(expression), std::move(cases), std::move(else_statements));
}

antlrcpp::Any Ast::visitCaseSelection(IEC61131Parser::CaseSelectionContext *context) {
    std::vector<Expression *> expressions = visit(context->caseList()).as<std::vector<Expression *>>();
    std::vector<Statement *> statements = visit(context->statementList()).as<std::vector<Statement *>>();
    return std::make_pair(expressions, statements);
}

antlrcpp::Any Ast::visitCaseList(IEC61131Parser::CaseListContext *context) {
    std::vector<Expression *> expressions;
    for (IEC61131Parser::CaseListElementContext *caseListElementContext : context->caseListElement()) {
        expressions.push_back(visit(caseListElementContext).as<Expression *>());
    }
    return expressions;
}

antlrcpp::Any Ast::visitCaseListElement(IEC61131Parser::CaseListElementContext *context) {
    if (context->expression()) {
        return (Expression *) visit(context->expression()).as<Expression *>();
    } else if (context->enumeratedValue()) {
        throw std::logic_error("Not implemented yet.");
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitDataTypeDeclaration(IEC61131Parser::DataTypeDeclarationContext *context) {
    std::vector<DataType *> data_types;
    for (IEC61131Parser::TypeDeclarationContext *type_declaration_context : context->typeDeclaration()) {
        data_types.push_back(visit(type_declaration_context).as<DataType *>());
    }
    return std::move(data_types);
}

antlrcpp::Any Ast::visitTypeDeclaration(IEC61131Parser::TypeDeclarationContext *context) {
    if (context->enumeratedTypeDeclaration()) {
        return (DataType *) visit(context->enumeratedTypeDeclaration()).as<EnumeratedType *>();
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any Ast::visitEnumeratedTypeDeclaration(IEC61131Parser::EnumeratedTypeDeclarationContext *context) {
    std::string enumerated_type_name = visit(context->enumeratedTypeName()).as<std::string>();
    auto *enumerated_type = new EnumeratedType(std::move(enumerated_type_name));
    boost::tuple<std::string, std::vector<std::string>, EnumeratedValue *> result =
            visit(context->enumeratedSpecificationInitialization())
                    .as<boost::tuple<std::string, std::vector<std::string>, EnumeratedValue *>>();
    if (result.get<0>().empty() && !result.get<1>().empty()) {
        // fill the enumerated type with the allowed values
        std::vector<std::string> values = result.get<1>();
        for (auto &value : values) {
            enumerated_type->addValue(std::move(value));
        }
        if (result.get<2>() == nullptr) {
            // do nothing, enumerated type declaration does not define an initial value
        } else {
            // TODO (MG): handle initial value
            throw std::logic_error("Not implemented yet.");
        }
    } else {
        // enumerated type declaration can only happen within a TYPE ... END_TYPE
        throw std::logic_error("This case should not occur.");
    }
    return enumerated_type;
}

antlrcpp::Any Ast::visitEnumeratedTypeName(IEC61131Parser::EnumeratedTypeNameContext *context) {
    return context->IDENTIFIER()->getText();
}

antlrcpp::Any
Ast::visitEnumeratedSpecificationInitialization(IEC61131Parser::EnumeratedSpecificationInitializationContext *context) {
    EnumeratedValue *enumerated_value = nullptr;
    if (context->enumeratedValue()) {
        // enum is initialized with a default value
        enumerated_value = visit(context->enumeratedValue()).as<EnumeratedValue *>();
    }
    // an enumerated specification initialization is either called in a TYPE ... END_TYPE declaration
    // or within a VAR_XXX ... END_VAR context. Regarding the TYPE ... END_TYPE context, the enumeratedSpecification
    // returns a vector of multiple values, in case of the VAR_XXX ... END_VAR context, the enumeratedSpecification
    // returns just a single value, i.e., the name of a enumerated data type.
    auto pair = visit(context->enumeratedSpecification()).as<std::pair<std::string, std::vector<std::string>>>();
    // check under which context the enumerated specification is initialized
    if (!pair.first.empty() && pair.second.empty()) {
        // VAR_XXX ... END_VAR
        std::string enumerated_type_name = pair.first;
        //  check if enumerated_value != nullptr, if it is, this variable has a default initialization. because
        //  we know the enumerated_type_name, we can check if it was parsed already (should be trivially parsed,
        //  because we first parse user-defined data-types) and then find the corresponding value of enumerated_value
        //  in the data_type and write the index value to this enumerated_value
        if (enumerated_value != nullptr) {
            auto *enum_type = dynamic_cast<EnumeratedType *>(_name_to_data_type.at(enumerated_type_name).get());
            assert(enum_type != nullptr);
            enumerated_value->setIndex(enum_type->getIndex(enumerated_value->getValue()));
        }
        boost::tuple<std::string, std::vector<std::string>, EnumeratedValue *> result =
                boost::make_tuple(enumerated_type_name, std::vector<std::string>(), enumerated_value);
        return result;
    } else if (pair.first.empty() && !pair.second.empty()) {
        // TYPE ... END_TYPE
        std::vector<std::string> values = pair.second;
        boost::tuple<std::string, std::vector<std::string>, EnumeratedValue *> result =
                boost::make_tuple("", values, enumerated_value);
        return result;
    } else {
        throw std::logic_error("This case should not occur.");
    }
}

antlrcpp::Any Ast::visitEnumeratedSpecification(IEC61131Parser::EnumeratedSpecificationContext *context) {
    // an enumerated specification either specifies a new user-defined data type or assigns a variable an already
    // defined enum
    if (context->enumeratedTypeName()) {
        std::string enumerated_type_name = visit(context->enumeratedTypeName()).as<std::string>();
        return std::pair<std::string, std::vector<std::string>>(std::move(enumerated_type_name),
                                                                std::vector<std::string>());
    } else {
        std::vector<std::string> values;
        for (auto *enumerated_value_specification_context : context->values) {
            values.push_back(visit(enumerated_value_specification_context).as<std::string>());
        }
        return std::pair<std::string, std::vector<std::string>>("", std::move(values));
    }
}

antlrcpp::Any Ast::visitEnumeratedValueSpecification(IEC61131Parser::EnumeratedValueSpecificationContext *context) {
    return context->IDENTIFIER()->getText();
}

antlrcpp::Any Ast::visitEnumeratedValue(IEC61131Parser::EnumeratedValueContext *context) {
    if (context->enumeratedTypeName()) {
        std::string enumerated_type_name = visit(context->enumeratedTypeName()).as<std::string>();
        auto it = _name_to_data_type.find(enumerated_type_name);
        if (it == _name_to_data_type.end()) {
            throw std::runtime_error("No enumerated data type with name " + enumerated_type_name + " does exist.");
        } else {
            std::string value = context->IDENTIFIER()->getText();
            auto *enumerated_type = dynamic_cast<const EnumeratedType *>(&*it->second);
            assert(enumerated_type != nullptr);
            int index = enumerated_type->getIndex(value);
            auto *enumerated_value = new EnumeratedValue(std::move(value));
            enumerated_value->setIndex(index);
            return enumerated_value;
        }
    } else {
        throw std::logic_error("Not explicitly typed enumerated value is currently not supported.");
    }
}

antlrcpp::Any Ast::visitTimeLiteral(IEC61131Parser::TimeLiteralContext *context) {
    if (context->timeTypeName()) {
        spdlog::warn("context->timeTypeName() is currently ignored...");
    }
    std::string interval = context->INTERVAL()->getText();
    // convert time into milliseconds
    size_t pos = 0;
    std::string token;
    int value = 0;
    if ((pos = interval.find('d')) != std::string::npos) {
        token = interval.substr(0, pos);
        spdlog::trace("Converting {}d into ms...", token);
        value += std::stoi(token) * 86400000;
        interval.erase(0, pos + 1);
    }
    if ((pos = interval.find('h')) != std::string::npos) {
        token = interval.substr(0, pos);
        spdlog::trace("Converting {}h into ms...", token);
        value += std::stoi(token) * 3600000;
        interval.erase(0, pos + 1);
    }
    if ((pos = interval.find('m')) != std::string::npos) {
        token = interval.substr(0, pos);
        // check if m matched to ms or to m
        auto is_ms = interval.substr(pos, pos + 1);
        bool is_number = std::regex_match(is_ms, std::regex("(\\+|-)?[0-9]*(\\.?([0-9]+))$"));
        if (!is_number) {
            // matched to ms...
        } else {
            spdlog::trace("Converting {}m into ms...", token);
            value += std::stoi(token) * 60000;
            interval.erase(0, pos + 1);
        }
    }
    if ((pos = interval.find('s')) != std::string::npos) {
        token = interval.substr(0, pos);
        // check if s matched to ms or to s
        auto is_ms = interval.substr(pos - 1, pos);
        bool is_number = std::regex_match(is_ms, std::regex("(\\+|-)?[0-9]*(\\.?([0-9]+))$"));
        if (!is_number) {
            // matched to ms...
        } else {
            spdlog::trace("Converting {}s into ms...", token);
            value += std::stoi(token) * 1000;
            interval.erase(0, pos + 1);
        }
    }
    if ((pos = interval.find("ms")) != std::string::npos) {
        token = interval.substr(0, pos);
        spdlog::trace("Adding {}ms to resulting ms...", token);
        value += std::stoi(token);
        interval.erase(0, pos + 2);
    }
    spdlog::trace("Resulting time in ms: {}", value);
    return new TimeConstant(value);
}

antlrcpp::Any Ast::visitSequentialFunctionChart(IEC61131Parser::SequentialFunctionChartContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitSequentialFunctionChartNetwork(IEC61131Parser::SequentialFunctionChartNetworkContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitInitialStep(IEC61131Parser::InitialStepContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitStep(IEC61131Parser::StepContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitStepName(IEC61131Parser::StepNameContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitActionAssociation(IEC61131Parser::ActionAssociationContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitActionName(IEC61131Parser::ActionNameContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitActionQualifier(IEC61131Parser::ActionQualifierContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitTimedQualifier(IEC61131Parser::TimedQualifierContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitActionTime(IEC61131Parser::ActionTimeContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitIndicatorName(IEC61131Parser::IndicatorNameContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitTransition(IEC61131Parser::TransitionContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitTransitionName(IEC61131Parser::TransitionNameContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitSteps(IEC61131Parser::StepsContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitTransitionCondition(IEC61131Parser::TransitionConditionContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitAction(IEC61131Parser::ActionContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitFunctionBody(IEC61131Parser::FunctionBodyContext *context) {
    return antlrcpp::Any();
}

antlrcpp::Any Ast::visitFunctionBlockBody(IEC61131Parser::FunctionBlockBodyContext *context) {
    if (context->sequentialFunctionChart()) {
        throw std::logic_error("Not implemented yet.");
    } else if (context->statementList()) {
        std::vector<std::unique_ptr<Statement>> statement_list;
        std::vector<Statement *> statements = visit(context->statementList()).as<std::vector<Statement *>>();
        statement_list.reserve(statements.size());
        for (Statement *statement : statements) {
            statement_list.push_back(std::unique_ptr<Statement>(statement));
        }
        return new Body(std::move(statement_list));
    } else {
        throw std::logic_error("Not implemented yet.");
    }
}

antlrcpp::Any
Ast::visitVariableDeclarationInitialization(IEC61131Parser::VariableDeclarationInitializationContext *context) {
    std::vector<Variable *> variables;
    Variable::StorageType storage_type;
    if (context->input) {
        storage_type = Variable::StorageType::INPUT;
    } else if (context->local) {
        storage_type = Variable::StorageType::LOCAL;
    } else if (context->output) {
        storage_type = Variable::StorageType::OUTPUT;
    } else if (context->temporary) {
        storage_type = Variable::StorageType::TEMPORARY;
    } else {
        throw std::logic_error("Not implemented yet.");
    }
    if (context->simpleSpecificationInitialization()) {
        std::vector<std::string> variable_names = visit(context->variableList()).as<std::vector<std::string>>();
        std::pair<DataType *, Constant *> pair =
                visit(context->simpleSpecificationInitialization()).as<std::pair<DataType *, Constant *>>();
        if (pair.first == nullptr) {
            throw std::logic_error("Variable has no type.");
        }
        for (auto variable_name : variable_names) {
            std::unique_ptr<DataType> type;
            if (auto simple_type = dynamic_cast<SimpleType *>(pair.first)) {
                auto name = simple_type->getName();
                if (_name_to_data_type.count(name) == 0) {
                    // type has not been parsed yet or variable has a different context, delay the decision
                    type = std::unique_ptr<DataType>(new InconclusiveType(std::move(name)));
                } else {
                    // type has already been parsed and can be used to determine the correct type for this variable
                    type = _name_to_data_type.at(name)->clone();
                }
            } else {
                type = pair.first->clone();
            }
            if (pair.second == nullptr) {
                // Variable has no initial value, do nothing
                variables.push_back(new Variable(std::move(variable_name), std::move(type), storage_type));
            } else {
                // Variable has an initial value
                if (auto *integer_constant = dynamic_cast<IntegerConstant *>(pair.second)) {
                    variables.push_back(new Variable(std::move(variable_name), std::move(type), storage_type,
                                                     std::make_unique<IntegerConstant>(integer_constant->getValue())));
                } else if (auto *boolean_constant = dynamic_cast<BooleanConstant *>(pair.second)) {
                    variables.push_back(new Variable(std::move(variable_name), std::move(type), storage_type,
                                                     std::make_unique<BooleanConstant>(boolean_constant->getValue())));
                } else if (auto *time_constant = dynamic_cast<TimeConstant *>(pair.second)) {
                    variables.push_back(new Variable(std::move(variable_name), std::move(type), storage_type,
                                                     std::make_unique<TimeConstant>(time_constant->getValue())));
                } else {
                    throw std::logic_error("Not implemented yet.");
                }
            }
        }
        delete pair.first;
        delete pair.second;
    } else if (context->enumeratedSpecificationInitialization()) {
        // check if enum was already parsed, else delay decision, i.e., check whether _name_to_data_type already has
        // an entry for this enum, then add the data_type to this variable else add an inconclusive type.
        std::vector<std::string> variable_names = visit(context->variableList()).as<std::vector<std::string>>();
        for (auto variable_name : variable_names) {
            // XXX result.get<0>() : name of the type, e.g., State : Magazin_States_t denotes "Magazin_States_t"
            // XXX result.get<1>() : the values of the enum, e.g., values of "Magazin_States_t"
            // XXX result.get<2>() : the initial value of this variable, e.g., a value from "Magazin_States_t"
            boost::tuple<std::string, std::vector<std::string>, EnumeratedValue *> result =
                    visit(context->enumeratedSpecificationInitialization())
                            .as<boost::tuple<std::string, std::vector<std::string>, EnumeratedValue *>>();
            assert(result.get<1>().empty());
            std::string data_type_name = result.get<0>();
            std::unique_ptr<DataType> data_type;
            if (_name_to_data_type.count(data_type_name)) {
                // type has already been parsed and can be used to determine the correct type for this variable
                data_type = _name_to_data_type.at(data_type_name)->clone();
            } else {
                // type has not been parsed yet or variable has a different context, delay the decision
                data_type = std::unique_ptr<DataType>(new InconclusiveType(std::move(data_type_name)));
            }
            EnumeratedValue *initial_value = result.get<2>();
            if (initial_value == nullptr) {
                // Variable has no initial value, do nothing
                variables.push_back(new Variable(std::move(variable_name), std::move(data_type), storage_type));
            } else {
                // Variable has an initial value
                // TODO (MG): how to handle the initial value? For enums we use their index value for now, but only
                //  because our value type system is too unflexible -_-"
                // XXX I do not want to use strings in the value type handle, to much hassle...
                variables.push_back(new Variable(std::move(variable_name), std::move(data_type), storage_type,
                                                 std::make_unique<IntegerConstant>(initial_value->getIndex())));
            }
        }
    } else if (context->structureVariableDeclarationInitialization()) {
        // check if struct was already parsed, else delay decision
        throw std::logic_error("Not implemented yet.");
    } else if (context->functionBlockDeclarationInitialization()) {
        // check if function block was already parsed, else delay decision
        throw std::logic_error("Not implemented yet.");
    }
    return variables;
}

antlrcpp::Any Ast::visitVariableList(IEC61131Parser::VariableListContext *context) {
    std::vector<std::string> variable_names;
    for (auto variable_name_context : context->variableName()) {
        variable_names.push_back(visit(variable_name_context).as<std::string>());
    }
    return variable_names;
}

antlrcpp::Any Ast::visitStructureVariableDeclarationInitialization(
        IEC61131Parser::StructureVariableDeclarationInitializationContext *context) {
    throw std::logic_error("Not implemented yet.");
}

antlrcpp::Any
Ast::visitStructureSpecificationInitialization(IEC61131Parser::StructureSpecificationInitializationContext *context) {
    throw std::logic_error("Not implemented yet.");
}

antlrcpp::Any Ast::visitStructureTypeName(IEC61131Parser::StructureTypeNameContext *context) {
    throw std::logic_error("Not implemented yet.");
}

antlrcpp::Any
Ast::visitSimpleSpecificationInitialization(IEC61131Parser::SimpleSpecificationInitializationContext *context) {
    DataType *data_type = visit(context->simpleSpecification()).as<DataType *>();
    Constant *constant = nullptr;
    if (context->ASSIGNMENT()) {
        // variable has an initial value
        constant = visit(context->constant()).as<Constant *>();
    }
    return std::make_pair(data_type, constant);
}

antlrcpp::Any Ast::visitSimpleSpecification(IEC61131Parser::SimpleSpecificationContext *context) {
    DataType *data_type = nullptr;
    if (context->elementaryTypeName()) {
        auto elementary_type = visit(context->elementaryTypeName()).as<ElementaryType *>();
        data_type = (DataType *) elementary_type;
    } else if (context->safetyTypeName()) {
        auto safety_type = visit(context->safetyTypeName()).as<SafetyType *>();
        data_type = (DataType *) safety_type;
    } else if (context->simpleTypeName()) {
        data_type = (DataType *) new SimpleType(visit(context->simpleTypeName()).as<std::string>());
    } else {
        throw std::logic_error("Not implemented yet.");
    }
    assert(data_type != nullptr);
    return data_type;
}

antlrcpp::Any Ast::visitSimpleTypeName(IEC61131Parser::SimpleTypeNameContext *context) {
    return context->IDENTIFIER()->getText();
}

antlrcpp::Any Ast::visitFunctionBlockDeclarationInitialization(
        IEC61131Parser::FunctionBlockDeclarationInitializationContext *context) {
    throw std::logic_error("Not implemented yet.");
}

antlrcpp::Any Ast::visitFunctionBlockNameList(IEC61131Parser::FunctionBlockNameListContext *context) {
    throw std::logic_error("Not implemented yet.");
}

antlrcpp::Any Ast::visitFunctionBlockTypeName(IEC61131Parser::FunctionBlockTypeNameContext *context) {
    throw std::logic_error("Not implemented yet.");
}

antlrcpp::Any Ast::visitStandardFunctionBlockName(IEC61131Parser::StandardFunctionBlockNameContext *context) {
    throw std::logic_error("Not implemented yet.");
}

antlrcpp::Any Ast::visitDerivedFunctionBlockName(IEC61131Parser::DerivedFunctionBlockNameContext *context) {
    return context->IDENTIFIER()->getText();
}

antlrcpp::Any Ast::visitStructureInitialization(IEC61131Parser::StructureInitializationContext *context) {
    throw std::logic_error("Not implemented yet.");
}

antlrcpp::Any Ast::visitStructureElementInitialization(IEC61131Parser::StructureElementInitializationContext *context) {
    throw std::logic_error("Not implemented yet.");
}

antlrcpp::Any Ast::visitStructureElementName(IEC61131Parser::StructureElementNameContext *context) {
    throw std::logic_error("Not implemented yet.");
}

void Ast::resolveInconclusiveTypes(Interface &interface) {
    for (auto it = interface.variablesBegin(); it != interface.variablesEnd(); ++it) {
        const auto &type = it->getDataType();
        if (const auto *inconclusive_type = dynamic_cast<const InconclusiveType *>(&type)) {
            auto name = inconclusive_type->getName();
            bool found = false;
            for (auto &p : _name_to_data_type) {
                if (name == p.first) {
                    // check if inconclusive type is a derived type
                    if (p.second->getKind() == DataType::Kind::DERIVED_TYPE) {
                        found = true;
                        it->setType(std::make_unique<DerivedType>(name));
                        break;
                    } else {
                        throw std::logic_error("Not implemented yet.");
                    }
                }
            }
            if (!found) {
                throw std::logic_error("Inconclusive type " + name + " could not be resolved!");
            }
        }
    }
}

antlrcpp::Any Ast::visitFunctionCall(IEC61131Parser::FunctionCallContext *context) {
    throw std::logic_error("Not implemented yet.");
}

std::unique_ptr<Expression>
Ast::compileCaseExpression(const Expression &lhs_expression,
                           const std::vector<std::unique_ptr<Expression>> &case_expressions) {
    std::unique_ptr<BinaryExpression> expression = nullptr;
    if (case_expressions.size() > 1) {
        expression = std::make_unique<BinaryExpression>(
                BinaryExpression::BinaryOperator::BOOLEAN_OR,
                std::make_unique<BinaryExpression>(BinaryExpression::BinaryOperator::EQUALITY,
                                                   std::move(lhs_expression.clone()),
                                                   std::move(case_expressions.at(0)->clone())),
                nullptr);
        for (std::vector<std::unique_ptr<Expression>>::size_type i = 1; i < case_expressions.size(); ++i) {
            auto e = std::make_unique<BinaryExpression>(BinaryExpression::BinaryOperator::EQUALITY,
                                                        std::move(lhs_expression.clone()),
                                                        std::move(case_expressions.at(i)->clone()));
            expression->setRightOperand(std::move(e));
            if (i + 1 != case_expressions.size()) {
                expression = std::make_unique<BinaryExpression>(BinaryExpression::BinaryOperator::BOOLEAN_OR,
                                                                std::move(expression), nullptr);
            }
        }
    } else {
        expression = std::make_unique<BinaryExpression>(BinaryExpression::BinaryOperator::EQUALITY,
                                                        std::move(lhs_expression.clone()),
                                                        std::move(case_expressions.at(0)->clone()));
    }
    return std::move(expression);
}

void Ast::noRecursionCheck() {
    for (auto &name_to_interface : _name_to_interface) {
        std::string name = name_to_interface.first;
        const auto &interface = name_to_interface.second;
        for (auto variable = interface->variablesBegin(); variable != interface->variablesEnd(); ++variable) {
            if (variable->getDataType().getKind() == DataType::Kind::DERIVED_TYPE) {
                // Variable : DataType, e.g., f : Fb1
                std::string variable_name = variable->getName();
                std::string type_representative_name = variable->getDataType().getFullyQualifiedName();
                const auto &used_pou_interface = _name_to_interface.at(type_representative_name);
                auto it = std::find_if(used_pou_interface->variablesBegin(), used_pou_interface->variablesEnd(),
                                       [name](const auto &v) { return v.getDataType().getName() == name; });
                if (it != used_pou_interface->variablesEnd()) {
                    throw std::logic_error("Not allowed recursion between " + name + " and " +
                                           type_representative_name + " detected. ");
                }
            }
        }
    }
}
