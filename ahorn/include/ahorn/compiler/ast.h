#ifndef AHORN_AST_H
#define AHORN_AST_H

#include "IEC61131Visitor.h"

#include "compiler/pou/pou.h"
#include "ir/interface.h"
#include "ir/type/data_type.h"

class Ast : public IEC61131Visitor {
public:
    Ast() = default;

    antlrcpp::Any visitSourceCode(IEC61131Parser::SourceCodeContext *context) override;

    antlrcpp::Any visitProgramDeclaration(IEC61131Parser::ProgramDeclarationContext *context) override;

    antlrcpp::Any visitProgramName(IEC61131Parser::ProgramNameContext *context) override;

    antlrcpp::Any visitFunctionBlockDeclaration(IEC61131Parser::FunctionBlockDeclarationContext *context) override;

    antlrcpp::Any visitFunctionBlockName(IEC61131Parser::FunctionBlockNameContext *context) override;

    antlrcpp::Any visitFunctionBlockBody(IEC61131Parser::FunctionBlockBodyContext *context) override;

    antlrcpp::Any visitFunctionDeclaration(IEC61131Parser::FunctionDeclarationContext *context) override;

    antlrcpp::Any visitFunctionName(IEC61131Parser::FunctionNameContext *context) override;

    antlrcpp::Any visitFunctionBody(IEC61131Parser::FunctionBodyContext *context) override;

    antlrcpp::Any visitInterface(IEC61131Parser::InterfaceContext *context) override;

    antlrcpp::Any visitInputVariableDeclarations(IEC61131Parser::InputVariableDeclarationsContext *context) override;

    antlrcpp::Any visitLocalVariableDeclarations(IEC61131Parser::LocalVariableDeclarationsContext *context) override;

    antlrcpp::Any visitOutputVariableDeclarations(IEC61131Parser::OutputVariableDeclarationsContext *context) override;

    antlrcpp::Any
    visitTemporaryVariableDeclarations(IEC61131Parser::TemporaryVariableDeclarationsContext *context) override;

    antlrcpp::Any
    visitVariableDeclarationInitialization(IEC61131Parser::VariableDeclarationInitializationContext *context) override;

    antlrcpp::Any visitVariableList(IEC61131Parser::VariableListContext *context) override;

    antlrcpp::Any visitStructureVariableDeclarationInitialization(
            IEC61131Parser::StructureVariableDeclarationInitializationContext *context) override;

    antlrcpp::Any visitStructureSpecificationInitialization(
            IEC61131Parser::StructureSpecificationInitializationContext *context) override;

    antlrcpp::Any visitStructureTypeName(IEC61131Parser::StructureTypeNameContext *context) override;

    antlrcpp::Any
    visitSimpleSpecificationInitialization(IEC61131Parser::SimpleSpecificationInitializationContext *context) override;

    antlrcpp::Any visitSimpleSpecification(IEC61131Parser::SimpleSpecificationContext *context) override;

    antlrcpp::Any visitSimpleTypeName(IEC61131Parser::SimpleTypeNameContext *context) override;

    antlrcpp::Any visitFunctionBlockDeclarationInitialization(
            IEC61131Parser::FunctionBlockDeclarationInitializationContext *context) override;

    antlrcpp::Any visitFunctionBlockNameList(IEC61131Parser::FunctionBlockNameListContext *context) override;

    antlrcpp::Any visitFunctionBlockTypeName(IEC61131Parser::FunctionBlockTypeNameContext *context) override;

    antlrcpp::Any visitStandardFunctionBlockName(IEC61131Parser::StandardFunctionBlockNameContext *context) override;

    antlrcpp::Any visitDerivedFunctionBlockName(IEC61131Parser::DerivedFunctionBlockNameContext *context) override;

    antlrcpp::Any visitStructureInitialization(IEC61131Parser::StructureInitializationContext *context) override;

    antlrcpp::Any
    visitStructureElementInitialization(IEC61131Parser::StructureElementInitializationContext *context) override;

    antlrcpp::Any visitStructureElementName(IEC61131Parser::StructureElementNameContext *context) override;

    antlrcpp::Any visitVariableName(IEC61131Parser::VariableNameContext *context) override;

    antlrcpp::Any visitDataTypeDeclaration(IEC61131Parser::DataTypeDeclarationContext *context) override;

    antlrcpp::Any visitTypeDeclaration(IEC61131Parser::TypeDeclarationContext *context) override;

    antlrcpp::Any visitEnumeratedTypeDeclaration(IEC61131Parser::EnumeratedTypeDeclarationContext *context) override;

    antlrcpp::Any visitEnumeratedTypeName(IEC61131Parser::EnumeratedTypeNameContext *context) override;

    antlrcpp::Any visitEnumeratedSpecificationInitialization(
            IEC61131Parser::EnumeratedSpecificationInitializationContext *context) override;

    antlrcpp::Any visitEnumeratedSpecification(IEC61131Parser::EnumeratedSpecificationContext *context) override;

    antlrcpp::Any
    visitEnumeratedValueSpecification(IEC61131Parser::EnumeratedValueSpecificationContext *context) override;

    antlrcpp::Any visitEnumeratedValue(IEC61131Parser::EnumeratedValueContext *context) override;

    antlrcpp::Any visitElementaryTypeName(IEC61131Parser::ElementaryTypeNameContext *context) override;

    antlrcpp::Any visitNumericTypeName(IEC61131Parser::NumericTypeNameContext *context) override;

    antlrcpp::Any visitIntegerTypeName(IEC61131Parser::IntegerTypeNameContext *context) override;

    antlrcpp::Any visitUnsignedIntegerTypeName(IEC61131Parser::UnsignedIntegerTypeNameContext *context) override;

    antlrcpp::Any visitSignedIntegerTypeName(IEC61131Parser::SignedIntegerTypeNameContext *context) override;

    antlrcpp::Any visitRealTypeName(IEC61131Parser::RealTypeNameContext *context) override;

    antlrcpp::Any visitBitStringTypeName(IEC61131Parser::BitStringTypeNameContext *context) override;

    antlrcpp::Any visitBooleanTypeName(IEC61131Parser::BooleanTypeNameContext *context) override;

    antlrcpp::Any visitTimeTypeName(IEC61131Parser::TimeTypeNameContext *context) override;

    antlrcpp::Any visitDerivedTypeName(IEC61131Parser::DerivedTypeNameContext *context) override;

    antlrcpp::Any visitSafetyTypeName(IEC61131Parser::SafetyTypeNameContext *context) override;

    antlrcpp::Any visitSafetyBooleanTypeName(IEC61131Parser::SafetyBooleanTypeNameContext *context) override;

    antlrcpp::Any visitStatement(IEC61131Parser::StatementContext *context) override;

    antlrcpp::Any visitAssignmentStatement(IEC61131Parser::AssignmentStatementContext *context) override;

    antlrcpp::Any visitNondeterministicConstant(IEC61131Parser::NondeterministicConstantContext *context) override;

    antlrcpp::Any visitAssumeStatement(IEC61131Parser::AssumeStatementContext *context) override;

    antlrcpp::Any visitAssertStatement(IEC61131Parser::AssertStatementContext *context) override;

    antlrcpp::Any visitSubprogramControlStatement(IEC61131Parser::SubprogramControlStatementContext *context) override;

    antlrcpp::Any visitInvocationStatement(IEC61131Parser::InvocationStatementContext *context) override;

    antlrcpp::Any visitFunctionBlockInstanceName(IEC61131Parser::FunctionBlockInstanceNameContext *context) override;

    antlrcpp::Any visitParameterAssignment(IEC61131Parser::ParameterAssignmentContext *context) override;

    antlrcpp::Any visitSelectionStatement(IEC61131Parser::SelectionStatementContext *context) override;

    antlrcpp::Any visitIfStatement(IEC61131Parser::IfStatementContext *context) override;

    antlrcpp::Any visitStatementList(IEC61131Parser::StatementListContext *context) override;

    antlrcpp::Any visitCaseStatement(IEC61131Parser::CaseStatementContext *context) override;

    antlrcpp::Any visitCaseSelection(IEC61131Parser::CaseSelectionContext *context) override;

    antlrcpp::Any visitCaseList(IEC61131Parser::CaseListContext *context) override;

    antlrcpp::Any visitCaseListElement(IEC61131Parser::CaseListElementContext *context) override;

    antlrcpp::Any visitIterationStatement(IEC61131Parser::IterationStatementContext *context) override;

    antlrcpp::Any visitWhileStatement(IEC61131Parser::WhileStatementContext *context) override;

    antlrcpp::Any visitBinaryExpression(IEC61131Parser::BinaryExpressionContext *context) override;

    antlrcpp::Any visitChangeExpression(IEC61131Parser::ChangeExpressionContext *context) override;

    antlrcpp::Any visitUnaryExpression(IEC61131Parser::UnaryExpressionContext *context) override;

    antlrcpp::Any visitPrimaryExpression(IEC61131Parser::PrimaryExpressionContext *context) override;

    antlrcpp::Any visitFunctionCall(IEC61131Parser::FunctionCallContext *context) override;

    antlrcpp::Any visitConstant(IEC61131Parser::ConstantContext *context) override;

    antlrcpp::Any visitVariableReference(IEC61131Parser::VariableReferenceContext *context) override;

    antlrcpp::Any visitVariableAccess(IEC61131Parser::VariableAccessContext *context) override;

    antlrcpp::Any visitFieldAccess(IEC61131Parser::FieldAccessContext *context) override;

    antlrcpp::Any visitRecordAccess(IEC61131Parser::RecordAccessContext *context) override;

    antlrcpp::Any visitNumericLiteral(IEC61131Parser::NumericLiteralContext *context) override;

    antlrcpp::Any visitIntegerLiteral(IEC61131Parser::IntegerLiteralContext *context) override;

    antlrcpp::Any visitSignedInteger(IEC61131Parser::SignedIntegerContext *context) override;

    antlrcpp::Any visitUnsignedInteger(IEC61131Parser::UnsignedIntegerContext *context) override;

    antlrcpp::Any visitTimeLiteral(IEC61131Parser::TimeLiteralContext *context) override;

    antlrcpp::Any visitBooleanLiteral(IEC61131Parser::BooleanLiteralContext *context) override;

    antlrcpp::Any visitSequentialFunctionChart(IEC61131Parser::SequentialFunctionChartContext *context) override;

    antlrcpp::Any
    visitSequentialFunctionChartNetwork(IEC61131Parser::SequentialFunctionChartNetworkContext *context) override;

    antlrcpp::Any visitInitialStep(IEC61131Parser::InitialStepContext *context) override;

    antlrcpp::Any visitStep(IEC61131Parser::StepContext *context) override;

    antlrcpp::Any visitStepName(IEC61131Parser::StepNameContext *context) override;

    antlrcpp::Any visitActionAssociation(IEC61131Parser::ActionAssociationContext *context) override;

    antlrcpp::Any visitActionName(IEC61131Parser::ActionNameContext *context) override;

    antlrcpp::Any visitActionQualifier(IEC61131Parser::ActionQualifierContext *context) override;

    antlrcpp::Any visitTimedQualifier(IEC61131Parser::TimedQualifierContext *context) override;

    antlrcpp::Any visitActionTime(IEC61131Parser::ActionTimeContext *context) override;

    antlrcpp::Any visitIndicatorName(IEC61131Parser::IndicatorNameContext *context) override;

    antlrcpp::Any visitTransition(IEC61131Parser::TransitionContext *context) override;

    antlrcpp::Any visitTransitionName(IEC61131Parser::TransitionNameContext *context) override;

    antlrcpp::Any visitSteps(IEC61131Parser::StepsContext *context) override;

    antlrcpp::Any visitTransitionCondition(IEC61131Parser::TransitionConditionContext *context) override;

    antlrcpp::Any visitAction(IEC61131Parser::ActionContext *context) override;

private:
    void resolveInconclusiveTypes(ir::Interface &interface);

    // checks if there exists a recursive definition
    void noRecursionCheck();

    static std::unique_ptr<ir::Expression>
    compileCaseExpression(const ir::Expression &lhs_expression,
                          const std::vector<std::unique_ptr<ir::Expression>> &case_expressions);

    // XXX recursive function, should only be called in the third pass, i.e., when all members are populated such
    // that all the information needed to build the pous is given
    void buildPou(const std::string &pou_name);

private:
    // the name of the current parsed pou
    std::string _pou_name;
    // type representative name of pou to kind
    std::map<std::string, Pou::Kind> _name_to_kind;
    // type representative name of pou to interface
    // the interface of the current pou, used to distinguish between enumerated values and variable references, can be
    // accessed via _pou_name
    std::map<std::string, std::unique_ptr<ir::Interface>> _name_to_interface;
    // type representative name of pou to body
    std::map<std::string, std::unique_ptr<Body>> _name_to_body;
    // the name of the callee, e.g., f in function block instance name f : Fb1, or the function name
    std::string _callee_variable_name;
    // maps a (user-defined) data type name to the respective data type
    std::map<std::string, std::unique_ptr<ir::DataType>> _name_to_data_type;
    // maps a pou type name to the respective pou
    std::map<std::string, std::unique_ptr<Pou>> _name_to_pou;
};
#endif//AHORN_AST_H
