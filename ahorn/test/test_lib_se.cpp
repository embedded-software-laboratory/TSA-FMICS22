#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <gtest/gtest.h>

#include "cfg/builder.h"
#include "compiler/compiler.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/goto_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "ir/type/elementary_type.h"
#include "pass/basic_block_pass.h"
#include "se/baseline/engine.h"
#include "se/experimental/context/context.h"
#include "se/experimental/context/frame.h"
#include "se/experimental/context/state.h"
#include "se/experimental/encoder.h"
#include "se/experimental/engine.h"
#include "se/experimental/evaluator.h"
#include "se/experimental/executor.h"
#include "se/experimental/expression/concrete_expression.h"
#include "se/experimental/expression/symbolic_expression.h"
#include "se/experimental/z3/manager.h"

#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "utilities/ostream_operators.h"

#include <chrono>
#include <fstream>
#include <sstream>

class TestLibSe : public ::testing::Test {
public:
    TestLibSe() : ::testing::Test() {}

protected:
    void SetUp() override {
        createLogger("Symbolic Execution");
        createLogger("SMART");
        createLogger("Baseline");
        createLogger("CBMC");
    }

    void createLogger(const std::string &name) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/" + name + ".txt", true);
        file_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);

        auto logger = std::make_shared<spdlog::logger>(name, std::begin(sinks), std::end(sinks));
        logger->set_level(spdlog::level::trace);

        spdlog::register_logger(logger);
    }

    void TearDown() override {
        spdlog::drop("Symbolic Execution");
        spdlog::drop("SMART");
        spdlog::drop("Baseline");
        spdlog::drop("CBMC");
    }

    static std::shared_ptr<Cfg> getCfg(std::string const &path) {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        const std::string &source_code = buffer.str();
        auto project = Compiler::compile(source_code);
        auto builder = std::make_unique<Builder>(*project);
        return builder->build();
    }
};

TEST_F(TestLibSe, Expression) {
    using namespace se;

    z3::context z3_context;
    z3::expr z3_expression_a = z3_context.int_const("a");
    std::shared_ptr<Expression> symbolic_expression_a = std::make_shared<SymbolicExpression>(z3_expression_a);

    std::cout << "use count: " << symbolic_expression_a.use_count() << std::endl;
    { std::shared_ptr<Expression> symbolic_expression_a_copy = symbolic_expression_a; }
    std::cout << "use count: " << symbolic_expression_a.use_count() << std::endl;

    const z3::expr &z3_expression_a_no_copy = symbolic_expression_a->getZ3Expression();
    z3::expr z3_expression_a_copy = symbolic_expression_a->getZ3Expression();

    z3::expr z3_expression_b = z3_context.int_const("b");
    z3::expr c = z3_expression_a + z3_expression_b;
    std::unique_ptr<Expression> unique_symbolic_expression_b = std::make_unique<SymbolicExpression>(z3_expression_b);
    unique_symbolic_expression_b = nullptr;

    std::vector<z3::expr> expressions;
    expressions.push_back(z3_expression_a);
    expressions.push_back(z3_expression_b);

    std::vector<z3::expr> expressions_copy;
    expressions_copy.reserve(expressions.size());
    for (const z3::expr &expression : expressions) {
        expressions_copy.push_back(expression);
    }

    throw std::logic_error("Not implemented yet.");
}

TEST_F(TestLibSe, Manager) {
    using namespace se;
    auto manager = std::make_unique<Manager>();
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, State) {
    using namespace se;
    auto vertex = std::make_shared<Vertex>(0);
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto state = std::make_unique<State>(
            *configuration, *vertex, std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>(),
            std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>(),
            std::map<std::string, std::shared_ptr<ShadowExpression>>(), std::vector<std::shared_ptr<Expression>>(),
            nullptr,
            std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                     VerificationConditionComparator>(),
            std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>(),
            std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator>(),
            std::map<std::string, std::string, VerificationConditionComparator>());
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Frame) {
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Context) {
    using namespace se;
    auto vertex = std::make_shared<Vertex>(0);
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto state = std::make_unique<State>(
            *configuration, *vertex, std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>(),
            std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>(),
            std::map<std::string, std::shared_ptr<ShadowExpression>>(), std::vector<std::shared_ptr<Expression>>(),
            nullptr,
            std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                     VerificationConditionComparator>(),
            std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>(),
            std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator>(),
            std::map<std::string, std::string, VerificationConditionComparator>());
    auto context = std::make_unique<Context>(0, std::move(state), std::deque<std::shared_ptr<Frame>>());
    ASSERT_EQ(context->_cycle, 0);
}

TEST_F(TestLibSe, Evaluator) {
    using namespace se;
    auto manager = std::make_unique<Manager>();
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto evaluator = std::make_unique<Evaluator>(*configuration, *manager);

    auto a = std::make_unique<ir::VariableAccess>(std::make_unique<ir::Variable>(
            "a", std::make_unique<ir::ElementaryType>("INT"), ir::Variable::StorageType::LOCAL));
    auto b = std::make_unique<ir::VariableAccess>(std::make_unique<ir::Variable>(
            "b", std::make_unique<ir::ElementaryType>("INT"), ir::Variable::StorageType::LOCAL));
    auto expression = std::make_unique<ir::BinaryExpression>(ir::BinaryExpression::BinaryOperator::ADD, std::move(a),
                                                             std::move(b));

    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
            contextualized_name_to_concrete_expression;
    contextualized_name_to_concrete_expression.emplace(
            "a_0__0", std::make_unique<ConcreteExpression>(manager->makeIntegerValue(1)));
    contextualized_name_to_concrete_expression.emplace(
            "b_0__0", std::make_unique<ConcreteExpression>(manager->makeIntegerValue(2)));
    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
            contextualized_name_to_symbolic_expression;
    auto vertex = std::make_shared<Vertex>(0);
    auto state = std::make_unique<State>(
            *configuration, *vertex, std::move(contextualized_name_to_concrete_expression),
            std::move(contextualized_name_to_symbolic_expression),
            std::map<std::string, std::shared_ptr<ShadowExpression>>(), std::vector<std::shared_ptr<Expression>>(),
            nullptr,
            std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                     VerificationConditionComparator>(),
            std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>(),
            std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator>(),
            std::map<std::string, std::string, VerificationConditionComparator>());
    auto context = std::make_unique<Context>(0, std::move(state), std::deque<std::shared_ptr<Frame>>());

    std::shared_ptr<Expression> evaluated_expression = evaluator->evaluate(*expression, *context);
    std::cout << *evaluated_expression << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Encoder) {
    using namespace se;
    auto manager = std::make_unique<Manager>();
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto encoder = std::make_unique<Encoder>(*configuration, *manager);

    auto a = std::make_unique<ir::VariableAccess>(std::make_unique<ir::Variable>(
            "a", std::make_unique<ir::ElementaryType>("INT"), ir::Variable::StorageType::LOCAL));
    auto b = std::make_unique<ir::VariableAccess>(std::make_unique<ir::Variable>(
            "b", std::make_unique<ir::ElementaryType>("INT"), ir::Variable::StorageType::LOCAL));
    auto expression = std::make_unique<ir::BinaryExpression>(ir::BinaryExpression::BinaryOperator::ADD, std::move(a),
                                                             std::move(b));

    std::map<std::string, unsigned int> name_to_version;
    name_to_version.emplace("a", 0);
    name_to_version.emplace("b", 0);
    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
            contextualized_name_to_concrete_expression;
    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
            contextualized_name_to_symbolic_expression;
    contextualized_name_to_symbolic_expression.emplace(
            "a_0__0", std::make_unique<SymbolicExpression>(manager->makeIntegerConstant("a_0__0")));
    contextualized_name_to_symbolic_expression.emplace(
            "b_0__0", std::make_unique<SymbolicExpression>(manager->makeIntegerConstant("b_0__0")));
    auto vertex = std::make_shared<Vertex>(0);
    auto state = std::make_unique<State>(
            *configuration, *vertex, std::move(contextualized_name_to_concrete_expression),
            std::move(contextualized_name_to_symbolic_expression),
            std::map<std::string, std::shared_ptr<ShadowExpression>>(), std::vector<std::shared_ptr<Expression>>(),
            nullptr,
            std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                     VerificationConditionComparator>(),
            std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>(),
            std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator>(),
            std::map<std::string, std::string, VerificationConditionComparator>());

    auto context = std::make_unique<Context>(0, std::move(state), std::deque<std::shared_ptr<Frame>>());

    std::shared_ptr<Expression> encoded_expression = encoder->encode(*expression, *context);
    std::cout << *encoded_expression << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Executor_Assignment) {
    using namespace se;
    auto manager = std::make_unique<Manager>();
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto executor = std::make_unique<Executor>(*configuration, *manager);

    auto a = std::make_unique<ir::VariableAccess>(std::make_unique<ir::Variable>(
            "a", std::make_unique<ir::ElementaryType>("INT"), ir::Variable::StorageType::LOCAL));
    auto b = std::make_unique<ir::VariableAccess>(std::make_unique<ir::Variable>(
            "b", std::make_unique<ir::ElementaryType>("INT"), ir::Variable::StorageType::LOCAL));
    auto expression = std::make_unique<ir::BinaryExpression>(ir::BinaryExpression::BinaryOperator::ADD, std::move(b),
                                                             std::make_unique<ir::IntegerConstant>(1));
    auto assignment_instruction = std::make_unique<ir::AssignmentInstruction>(std::move(a), std::move(expression));

    std::map<unsigned int, std::shared_ptr<Vertex>> label_to_vertex;
    auto vertex = std::make_shared<Vertex>(0);
    vertex->setInstruction(std::move(assignment_instruction));
    label_to_vertex.emplace(0, std::move(vertex));

    std::vector<std::shared_ptr<Edge>> edges;
    edges.push_back(std::make_shared<Edge>(0, 1));

    auto cfg = std::make_shared<Cfg>(
            Cfg::Type::PROGRAM, "P", std::make_unique<ir::Interface>(std::vector<std::shared_ptr<ir::Variable>>()),
            std::map<std::string, std::shared_ptr<Cfg>>(), std::move(label_to_vertex), std::move(edges), 0, 0);

    std::map<std::string, unsigned int> name_to_version;
    name_to_version.emplace("a", 0);
    name_to_version.emplace("b", 0);
    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
            contextualized_name_to_concrete_expression;
    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
            contextualized_name_to_symbolic_expression;
    contextualized_name_to_symbolic_expression.emplace(
            "a_0__0", std::make_unique<SymbolicExpression>(manager->makeIntegerConstant("a_0__0")));
    contextualized_name_to_symbolic_expression.emplace(
            "b_0__0", std::make_unique<SymbolicExpression>(manager->makeIntegerConstant("b_0__0")));
    vertex = std::make_shared<Vertex>(0);
    auto state = std::make_unique<State>(
            *configuration, *vertex, std::move(contextualized_name_to_concrete_expression),
            std::move(contextualized_name_to_symbolic_expression),
            std::map<std::string, std::shared_ptr<ShadowExpression>>(), std::vector<std::shared_ptr<Expression>>(),
            nullptr,
            std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                     VerificationConditionComparator>(),
            std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>(),
            std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator>(),
            std::map<std::string, std::string, VerificationConditionComparator>());

    std::deque<std::shared_ptr<Frame>> frame_stack;
    frame_stack.push_back(std::make_shared<Frame>(*cfg, "P", 0));

    auto context = std::make_unique<Context>(0, std::move(state), std::move(frame_stack));

    executor->execute(std::move(context));

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Executor_If) {
    using namespace se;
    auto manager = std::make_unique<Manager>();
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto executor = std::make_unique<Executor>(*configuration, *manager);

    auto a_variable = std::make_shared<ir::Variable>("a", std::make_unique<ir::ElementaryType>("INT"),
                                                     ir::Variable::StorageType::INPUT);
    auto b_variable = std::make_shared<ir::Variable>("b", std::make_unique<ir::ElementaryType>("INT"),
                                                     ir::Variable::StorageType::LOCAL);
    auto a_expression = std::make_unique<ir::VariableAccess>(a_variable);
    auto expression =
            std::make_unique<ir::BinaryExpression>(ir::BinaryExpression::BinaryOperator::GREATER_THAN,
                                                   std::move(a_expression), std::make_unique<ir::IntegerConstant>(1));
    auto if_instruction = std::make_unique<ir::IfInstruction>(
            std::move(expression), std::make_unique<ir::GotoInstruction>(1), std::make_unique<ir::GotoInstruction>(2));

    std::vector<std::shared_ptr<ir::Variable>> variables;
    variables.push_back(a_variable);
    variables.push_back(b_variable);

    std::map<unsigned int, std::shared_ptr<Vertex>> label_to_vertex;
    auto vertex = std::make_shared<Vertex>(0);
    vertex->setInstruction(std::move(if_instruction));
    label_to_vertex.emplace(0, std::move(vertex));
    label_to_vertex.emplace(1, std::make_shared<Vertex>(1));
    label_to_vertex.emplace(2, std::make_shared<Vertex>(2));

    std::vector<std::shared_ptr<Edge>> edges;
    edges.push_back(std::make_shared<Edge>(0, 1, Edge::Type::TRUE_BRANCH));
    edges.push_back(std::make_shared<Edge>(0, 2, Edge::Type::FALSE_BRANCH));

    auto cfg = std::make_shared<Cfg>(Cfg::Type::PROGRAM, "P", std::make_unique<ir::Interface>(variables),
                                     std::map<std::string, std::shared_ptr<Cfg>>(), std::move(label_to_vertex),
                                     std::move(edges), 0, 0);

    std::map<std::string, unsigned int> decontextualized_name_to_version;
    decontextualized_name_to_version.emplace("P.a", 0);
    decontextualized_name_to_version.emplace("P.b", 0);
    decontextualized_name_to_version.emplace("P.c", 0);
    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
            contextualized_name_to_concrete_expression;
    contextualized_name_to_concrete_expression.emplace(
            "P.a_0__0", std::make_unique<ConcreteExpression>(manager->makeIntegerValue(0)));
    contextualized_name_to_concrete_expression.emplace(
            "P.b_0__0", std::make_unique<ConcreteExpression>(manager->makeIntegerValue(1)));
    contextualized_name_to_concrete_expression.emplace(
            "P.b_1__0", std::make_unique<ConcreteExpression>(manager->makeIntegerValue(3)));
    contextualized_name_to_concrete_expression.emplace(
            "P.c_0__0", std::make_unique<ConcreteExpression>(manager->makeIntegerValue(2)));
    std::map<std::string, std::shared_ptr<Expression>, ContextualizedNameComparator>
            contextualized_name_to_symbolic_expression;
    contextualized_name_to_symbolic_expression.emplace(
            "P.a_0__0", std::make_unique<SymbolicExpression>(manager->makeIntegerConstant("P.a_0__0")));
    contextualized_name_to_symbolic_expression.emplace(
            "P.b_0__0", std::make_unique<SymbolicExpression>(manager->makeIntegerConstant("P.b_0__0")));
    contextualized_name_to_symbolic_expression.emplace(
            "P.b_1__0", std::make_unique<SymbolicExpression>(manager->makeIntegerConstant("P.b_0__0") +
                                                             manager->makeIntegerConstant("P.c_0__0")));
    contextualized_name_to_symbolic_expression.emplace(
            "P.c_0__0", std::make_unique<SymbolicExpression>(manager->makeIntegerConstant("P.a_0__0") +
                                                             manager->makeIntegerValue(1)));
    vertex = std::make_shared<Vertex>(0);
    auto state = std::make_unique<State>(
            *configuration, *vertex, std::move(contextualized_name_to_concrete_expression),
            std::move(contextualized_name_to_symbolic_expression),
            std::map<std::string, std::shared_ptr<ShadowExpression>>(), std::vector<std::shared_ptr<Expression>>(),
            nullptr,
            std::map<std::string, std::vector<std::shared_ptr<AssumptionLiteralExpression>>,
                     VerificationConditionComparator>(),
            std::map<std::string, std::vector<std::shared_ptr<SymbolicExpression>>, VerificationConditionComparator>(),
            std::map<std::string, std::map<std::string, z3::expr>, VerificationConditionComparator>(),
            std::map<std::string, std::string, VerificationConditionComparator>());

    std::deque<std::shared_ptr<Frame>> frame_stack;
    frame_stack.push_back(std::make_shared<Frame>(*cfg, "P", 0));
    auto context = std::make_unique<Context>(0, std::move(state), std::move(frame_stack));

    std::cout << *context << std::endl;

    auto execution_result = executor->execute(std::move(context));

    std::cout << *execution_result.second.at(0) << std::endl;
    std::cout << *execution_result.second.at(1) << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Constant_Propagation_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/constant_propagation.st");
    // initialization
    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        engine->_explorer->pushContext(std::move(succeeding_context));
    }

    step_result = engine->step();
    succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Constant_Propagation_With_Input_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/constant_propagation_with_input.st");
    // initialization
    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        engine->_explorer->pushContext(std::move(succeeding_context));
    }

    step_result = engine->step();
    succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Constant_Propagation_With_Shadows_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/constant_propagation_with_shadows.st");
    // initialization
    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        engine->_explorer->pushContext(std::move(succeeding_context));
    }

    step_result = engine->step();
    succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_If_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/if.st");
    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_While_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/while.st");
    std::cout << cfg->toDot() << std::endl;

    {
        std::cout << "basic block pass:" << std::endl;
        auto basic_block_pass = std::make_unique<pass::BasicBlockPass>();
        std::shared_ptr<Cfg> basic_block_cfg = basic_block_pass->apply(*cfg);
        std::cout << basic_block_cfg->toDot() << std::endl;
    }

    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;
}

TEST_F(TestLibSe, Engine_While_With_If_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/while_with_if.st");
    std::cout << cfg->toDot() << std::endl;

    {
        std::cout << "basic block pass:" << std::endl;
        auto basic_block_pass = std::make_unique<pass::BasicBlockPass>();
        std::shared_ptr<Cfg> basic_block_cfg = basic_block_pass->apply(*cfg);
        std::cout << basic_block_cfg->toDot() << std::endl;
    }

    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;
}

TEST_F(TestLibSe, Engine_Non_deterministic_If_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/non_deterministic_if.st");
    std::cout << cfg->toDot() << std::endl;

    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Call_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/call.st");
    std::cout << cfg->toDot() << std::endl;

    {
        std::cout << "basic block pass:" << std::endl;
        auto basic_block_pass = std::make_unique<pass::BasicBlockPass>();
        std::shared_ptr<Cfg> basic_block_cfg = basic_block_pass->apply(*cfg);
        std::cout << basic_block_cfg->toDot() << std::endl;
    }

    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;

    z3::solver solver(engine->_manager->getZ3Context());
    const Context &succeeding_context = *succeeding_contexts.at(0);
    const State &state = succeeding_context.getState();
    throw std::logic_error("Not implemented yet.");
    /*
    for (const auto &verification_condition : state.getVerificationConditions()) {
        z3::expr assumption_literal = engine->_manager->makeBooleanConstant(verification_condition.first);
        z3::expr_vector e(engine->_manager->getZ3Context());
        if (verification_condition.first == "b_P_11__0") {
            e.push_back(engine->_manager->makeBooleanConstant("b_P_7__0"));
        }
        for (const auto &expression : verification_condition.second) {
            e.push_back(expression->getZ3Expression());
        }
        solver.add(z3::implies(assumption_literal, z3::mk_and(e)));
    }
     */
    std::cout << solver.assertions() << std::endl;
    z3::expr_vector vec(engine->_manager->getZ3Context());
    /*
    vec.push_back(engine->_manager->makeBooleanConstant("b_P_13__0"));
    vec.push_back(engine->_manager->makeIntegerConstant("P.x_0__0") == engine->_manager->makeIntegerValue(32));
    vec.push_back(engine->_manager->makeIntegerConstant("P.y_2__0") == engine->_manager->makeIntegerValue(64));
    */
    z3::check_result result = solver.check(vec);
    switch (result) {
        case z3::unsat: {
            std::cout << "unsat" << std::endl;
            break;
        }
        case z3::sat: {
            std::cout << "sat" << std::endl;
            z3::model z3_model = solver.get_model();
            for (unsigned int i = 0; i < z3_model.size(); ++i) {
                z3::func_decl constant_declaration = z3_model.get_const_decl(i);
                assert(constant_declaration.is_const() && constant_declaration.arity() == 0);
                std::string contextualized_name = constant_declaration.name().str();
                z3::expr interpretation = z3_model.get_const_interp(constant_declaration);
                std::cout << contextualized_name << " -> " << interpretation << std::endl;
            }
            break;
        }
        case z3::unknown: {
            std::cout << "unknown" << std::endl;
            break;
        }
    }

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Call_Two_Times_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/call_two_times.st");
    std::cout << cfg->toDot() << std::endl;

    {
        std::cout << "basic block pass:" << std::endl;
        auto basic_block_pass = std::make_unique<pass::BasicBlockPass>();
        std::shared_ptr<Cfg> basic_block_cfg = basic_block_pass->apply(*cfg);
        std::cout << basic_block_cfg->toDot() << std::endl;
    }

    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Call_With_Tripple_Diamond_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/call_with_tripple_diamond.st");
    std::cout << cfg->toDot() << std::endl;

    {
        std::cout << "basic block pass:" << std::endl;
        auto basic_block_pass = std::make_unique<pass::BasicBlockPass>();
        std::shared_ptr<Cfg> basic_block_cfg = basic_block_pass->apply(*cfg);
        std::cout << basic_block_cfg->toDot() << std::endl;
    }

    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Branch_Coverage_In_3_Cycles_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/branch_coverage_in_3_cycles.st");
    std::cout << cfg->toDot() << std::endl;
    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
        engine->_explorer->pushContext(std::move(succeeding_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;
    step_result = engine->step();
    succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
        engine->_explorer->pushContext(std::move(succeeding_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;
    step_result = engine->step();
    succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
        engine->_explorer->pushContext(std::move(succeeding_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Call_Coverage_In_3_Cycles_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::VERIFICATION_CONDITION_GENERATION,
            Configuration::SummarizationMode::FUNCTION_BLOCK, Configuration::BlockEncoding::SINGLE,
            Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/call_coverage_in_3_cycles.st");
    std::cout << cfg->toDot() << std::endl;
    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
        engine->_explorer->pushContext(std::move(succeeding_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;
    step_result = engine->step();
    succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
        engine->_explorer->pushContext(std::move(succeeding_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;
    step_result = engine->step();
    succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
        engine->_explorer->pushContext(std::move(succeeding_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Call_Coverage_In_1_Cycle_With_Change_Shadow_Cfg) {
    using namespace se;
    auto shadow_configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::SHADOW, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::SHADOW,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS,
            Configuration::ShadowProcessingMode::BOTH);
    auto shadow_engine = std::make_unique<Engine>(std::move(shadow_configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/call_coverage_in_1_cycle_with_change_shadow.st");
    std::cout << cfg->toDot() << std::endl;

    shadow_engine->initialize(*cfg);
    auto context = shadow_engine->createInitialContext(*cfg);

    // Phase 1
    shadow_engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = shadow_engine->step();
    assert(step_result.first == Engine::EngineStatus::DIVERGENT_BEHAVIOR);
    std::vector<std::unique_ptr<Context>> divergent_contexts = std::move(step_result.second);

    // Phase 2
    // TODO (25.01.2022): new engine means new z3 context, transfer the context/constraints to the new context?
    for (std::unique_ptr<Context> &divergent_context : divergent_contexts) {
        auto configuration = std::make_unique<Configuration>(
                Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
                Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
                Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
                Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS,
                Configuration::ShadowProcessingMode::NEW);
        auto compositional_engine = std::make_unique<Engine>(std::move(configuration));
        compositional_engine->initialize(*cfg);
        std::unique_ptr<Context> seeded_context = compositional_engine->translateDivergentContext(
                shadow_engine->_manager->getZ3Context(), std::move(divergent_context));
        compositional_engine->_explorer->pushContext(std::move(seeded_context));
        step_result = compositional_engine->step();
    }

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Breadth_First_and_Merging_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/breadth-first_and_merging.st");
    std::cout << cfg->toDot() << std::endl;
    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
        engine->_merger->pushContext(std::move(succeeding_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;

    std::unique_ptr<Context> merged_context = engine->_merger->merge();
    std::cout << "\n\n######## Merged context for succeeding cycle: ########" << std::endl;
    std::cout << *merged_context << std::endl;
    std::cout << "\n###################################################\n\n" << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Antivalent_Test_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    std::cout << *configuration << std::endl;
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/antivalent_test.st");
    std::cout << cfg->toDot() << std::endl;

    engine->initialize(*cfg);
    std::cout << *engine->_merger << std::endl;
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));

    while (engine->_cycle < 20) {
        std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
        std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
        std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
        for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
            engine->_explorer->pushContext(std::move(succeeding_context));
        }
        std::cout << "\n###################################################\n\n" << std::endl;
    }

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Emergency_Stop_Test_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::ONLY_AT_CYCLE_END);
    std::cout << *configuration << std::endl;
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/emergency_stop_test.st");
    std::cout << cfg->toDot() << std::endl;

    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        // std::cout << "\n" << *succeeding_context << std::endl;
        engine->_explorer->pushContext(std::move(succeeding_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;
    step_result = engine->step();
    succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        // std::cout << "\n" << *succeeding_context << std::endl;
        engine->_explorer->pushContext(std::move(succeeding_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;
    step_result = engine->step();
    succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        // std::cout << "\n" << *succeeding_context << std::endl;
        engine->_explorer->pushContext(std::move(succeeding_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;
    step_result = engine->step();
    succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        // std::cout << "\n" << *succeeding_context << std::endl;
        engine->_explorer->pushContext(std::move(succeeding_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;
    step_result = engine->step();
    succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        // std::cout << "\n" << *succeeding_context << std::endl;
        engine->_explorer->pushContext(std::move(succeeding_context));
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    std::cout << "Engine: " << *engine << std::endl;

    /*
    for (const auto &test_case : engine->_test_suite->getTestCases()) {
        std::cout << *test_case << std::endl;
    }
     */

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_If_Change_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/if_change.st");
    std::cout << cfg->toDot() << std::endl;

    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_If_Reaching_Change_Cfg) {
    using namespace se;
    auto shadow_configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::SHADOW, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::SHADOW,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS,
            Configuration::ShadowProcessingMode::BOTH);
    auto shadow_engine = std::make_unique<Engine>(std::move(shadow_configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/if_reaching_change.st");
    std::cout << cfg->toDot() << std::endl;

    shadow_engine->initialize(*cfg);
    auto context = shadow_engine->createInitialContext(*cfg);

    // Phase 1
    shadow_engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = shadow_engine->step();
    assert(step_result.first == Engine::EngineStatus::DIVERGENT_BEHAVIOR);
    std::vector<std::unique_ptr<Context>> divergent_contexts = std::move(step_result.second);

    // Phase 2
    // TODO (25.01.2022): new engine means new z3 context, transfer the context/constraints to the new context?
    for (std::unique_ptr<Context> &divergent_context : divergent_contexts) {
        auto configuration = std::make_unique<Configuration>(
                Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
                Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
                Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
                Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS,
                Configuration::ShadowProcessingMode::NEW);
        auto compositional_engine = std::make_unique<Engine>(std::move(configuration));
        compositional_engine->initialize(*cfg);
        std::unique_ptr<Context> seeded_context = compositional_engine->translateDivergentContext(
                shadow_engine->_manager->getZ3Context(), std::move(divergent_context));
        compositional_engine->_explorer->pushContext(std::move(seeded_context));
        step_result = compositional_engine->step();
    }

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Merge_If_Call_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/merge_if_call.st");
    std::cout << cfg->toDot() << std::endl;

    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Different_Frame_Stacks_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::AT_ALL_JOIN_POINTS);
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/different_frame_stacks.st");
    std::cout << cfg->toDot() << std::endl;

    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);

    engine->_explorer->pushContext(std::move(context));
    std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
    std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
    std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
    for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
        std::cout << "\n" << *succeeding_context << std::endl;
    }
    std::cout << "\n###################################################\n\n" << std::endl;

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;

    ASSERT_EQ(0, 1);
}

TEST_F(TestLibSe, Engine_Mode_Selector_Test_Cfg) {
    using namespace se;
    auto configuration = std::make_unique<Configuration>(
            Configuration::EngineMode::COMPOSITIONAL, Configuration::StepSize::CYCLE,
            Configuration::ExplorationHeuristic::DEPTH_FIRST, Configuration::ExecutionMode::COMPOSITIONAL,
            Configuration::EncodingMode::NONE, Configuration::SummarizationMode::NONE,
            Configuration::BlockEncoding::SINGLE, Configuration::MergeStrategy::ONLY_AT_CYCLE_END);
    std::cout << *configuration << std::endl;
    auto engine = std::make_unique<Engine>(std::move(configuration));
    std::shared_ptr<Cfg> cfg = getCfg("../../test/file/se/mode_selector_test.st");
    std::cout << cfg->toDot() << std::endl;

    engine->initialize(*cfg);
    auto context = engine->createInitialContext(*cfg);
    engine->_explorer->pushContext(std::move(context));

    while (engine->_cycle < 4) {
        std::pair<Engine::EngineStatus, std::vector<std::unique_ptr<Context>>> step_result = engine->step();
        std::vector<std::unique_ptr<Context>> succeeding_contexts = std::move(step_result.second);
        std::cout << "\n\n######## Contexts reaching end of cycle " << engine->_cycle << ": ########" << std::endl;
        for (std::unique_ptr<Context> &succeeding_context : succeeding_contexts) {
            engine->_explorer->pushContext(std::move(succeeding_context));
        }
        std::cout << "\n###################################################\n\n" << std::endl;
    }

    std::cout << "Engine: " << *engine << std::endl;
    std::cout << "Explorer: " << *engine->_explorer << std::endl;

    ASSERT_EQ(0, 1);
}