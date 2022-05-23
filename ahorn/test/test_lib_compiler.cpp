#include <gtest/gtest.h>

#include "compiler/compiler.h"
#include "compiler/statement/assignment_statement.h"
#include "compiler/statement/if_statement.h"
#include "compiler/statement/invocation_statement.h"
#include "ir/type/derived_type.h"
#include "ir/type/elementary_type.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>

using namespace ir;

class TestLibCompiler : public ::testing::Test {
public:
    TestLibCompiler() : ::testing::Test() {}

protected:
    void SetUp() override {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/compiler.txt", true);
        file_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);

        auto logger = std::make_shared<spdlog::logger>("Compiler", std::begin(sinks), std::end(sinks));
        logger->set_level(spdlog::level::trace);

        spdlog::register_logger(logger);
    }

    void TearDown() override {
        spdlog::drop("Compiler");
    }

    static std::unique_ptr<compiler::Project> getProject(std::string const &path) {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        const std::string &source_code = buffer.str();
        auto compiler = std::make_unique<Compiler>();
        auto project = compiler->parse(source_code);
        return project;
    }
};

TEST_F(TestLibCompiler, Compiler) {
    {
        auto project = getProject("../../test/file/ir_empty_if.st");
        const auto &program = project->getProgram();
        auto x = program.getInterface().getVariable("x");
        auto y = program.getInterface().getVariable("y");
        ASSERT_EQ(dynamic_cast<const ElementaryType &>(x->getDataType()).getType(), ElementaryType::Type::BOOL);
        ASSERT_EQ(dynamic_cast<const ElementaryType &>(y->getDataType()).getType(), ElementaryType::Type::INT);
        {
            const Statement &statement = *(program.getBody()._statement_list.at(0));
            const auto &if_statement = dynamic_cast<const IfStatement &>(statement);
            ASSERT_EQ(if_statement.hasThenStatements(), false);
            ASSERT_EQ(if_statement.hasElseIfStatements(), false);
            ASSERT_EQ(if_statement.hasElseStatements(), false);
        }
    }
    {
        auto project = getProject("../../test/file/parse_in_order.st");
        const auto &program = project->getProgram();
        for (auto it = project->pousBegin(); it != project->pousEnd(); ++it) {
            std::cout << *it << std::endl;
        }
        for (auto it = program.flattenedInterfaceBegin(); it != program.flattenedInterfaceEnd(); ++it) {
            std::cout << "FQN: " << it->getFullyQualifiedName() << ", N: " << it->getName() << std::endl;
        }
        {
            const auto &pou = project->getPou("Fb1");
            ASSERT_EQ(pou.getKind(), Pou::Kind::FUNCTION_BLOCK);
            {// input variables
                const auto &x = *(pou.getInterface()._name_to_variable.at("x"));
                ASSERT_EQ(x.getStorageType(), Variable::StorageType::INPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(x.getDataType()).getType(), ElementaryType::Type::BOOL);
                const auto &y = *(pou.getInterface()._name_to_variable.at("y"));
                ASSERT_EQ(y.getStorageType(), Variable::StorageType::INPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(y.getDataType()).getType(), ElementaryType::Type::INT);
                const auto &z = *(pou.getInterface()._name_to_variable.at("z"));
                ASSERT_EQ(z.getStorageType(), Variable::StorageType::INPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(z.getDataType()).getType(), ElementaryType::Type::INT);
            }
            {// local variables
                const auto &g = *(pou.getInterface()._name_to_variable.at("g"));
                ASSERT_EQ(g.getStorageType(), Variable::StorageType::LOCAL);
                ASSERT_EQ(dynamic_cast<const DerivedType &>(g.getDataType()).getName(), "Fb2");
            }
            {// output variables
                const auto &a = *(pou.getInterface()._name_to_variable.at("a"));
                ASSERT_EQ(a.getStorageType(), Variable::StorageType::OUTPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(a.getDataType()).getType(), ElementaryType::Type::INT);
                const auto &b = *(pou.getInterface()._name_to_variable.at("b"));
                ASSERT_EQ(b.getStorageType(), Variable::StorageType::OUTPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(b.getDataType()).getType(), ElementaryType::Type::INT);
                const auto &c = *(pou.getInterface()._name_to_variable.at("c"));
                ASSERT_EQ(c.getStorageType(), Variable::StorageType::OUTPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(c.getDataType()).getType(), ElementaryType::Type::INT);
            }
            {
                const auto &statement = *(pou.getBody()._statement_list.at(0));
                const auto &assignment_statement = dynamic_cast<const AssignmentStatement &>(statement);
                std::stringstream str;
                assignment_statement.print(str);
                ASSERT_EQ(str.str(), "a := x + 1");
            }
            {
                const auto &statement = *(pou.getBody()._statement_list.at(1));
                const auto &invocation_statement = dynamic_cast<const InvocationStatement &>(statement);
                const auto &g = *(invocation_statement.getVariableAccess().getVariable());
                ASSERT_EQ(g.getName(), "g");
                ASSERT_EQ(g.getStorageType(), Variable::StorageType::LOCAL);
                ASSERT_EQ(dynamic_cast<const DerivedType &>(g.getDataType()).getName(), "Fb2");
                ASSERT_EQ(invocation_statement._pre_assignments.size(), 2);
                {
                    const auto &pre_assignment = *(invocation_statement._pre_assignments.at(0));
                    std::stringstream str;
                    pre_assignment.print(str);
                    ASSERT_EQ(str.str(), "g.x := y");
                }
                {
                    const auto &pre_assignment = *(invocation_statement._pre_assignments.at(1));
                    std::stringstream str;
                    pre_assignment.print(str);
                    ASSERT_EQ(str.str(), "g.y := z");
                }
                ASSERT_EQ(invocation_statement._post_assignments.size(), 1);
                {
                    const auto &post_assignment = *(invocation_statement._post_assignments.at(0));
                    std::stringstream str;
                    post_assignment.print(str);
                    ASSERT_EQ(str.str(), "b := g.a");
                }
            }
            {
                const auto &statement = *(pou.getBody()._statement_list.at(2));
                const auto &if_statement = dynamic_cast<const IfStatement &>(statement);
            }
        }
        {
            const auto &pou = project->getPou("Fb2");
            ASSERT_EQ(pou.getKind(), Pou::Kind::FUNCTION_BLOCK);
            {// input variables
                const auto &x = *(pou.getInterface()._name_to_variable.at("x"));
                ASSERT_EQ(x.getStorageType(), Variable::StorageType::INPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(x.getDataType()).getType(), ElementaryType::Type::INT);
                const auto &y = *(pou.getInterface()._name_to_variable.at("y"));
                ASSERT_EQ(y.getStorageType(), Variable::StorageType::INPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(y.getDataType()).getType(), ElementaryType::Type::INT);
            }
            {// output variables
                const auto &a = *(pou.getInterface()._name_to_variable.at("a"));
                ASSERT_EQ(a.getStorageType(), Variable::StorageType::OUTPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(a.getDataType()).getType(), ElementaryType::Type::INT);
                const auto &b = *(pou.getInterface()._name_to_variable.at("b"));
                ASSERT_EQ(b.getStorageType(), Variable::StorageType::OUTPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(b.getDataType()).getType(), ElementaryType::Type::INT);
            }
        }
        {
            const auto &pou = project->getPou("Fb3");
            ASSERT_EQ(pou.getKind(), Pou::Kind::FUNCTION_BLOCK);
            {// input variables
                const auto &x = *(pou.getInterface()._name_to_variable.at("x"));
                ASSERT_EQ(x.getStorageType(), Variable::StorageType::INPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(x.getDataType()).getType(), ElementaryType::Type::INT);
                const auto &y = *(pou.getInterface()._name_to_variable.at("y"));
                ASSERT_EQ(y.getStorageType(), Variable::StorageType::INPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(y.getDataType()).getType(), ElementaryType::Type::INT);
                const auto &z = *(pou.getInterface()._name_to_variable.at("z"));
                ASSERT_EQ(z.getStorageType(), Variable::StorageType::INPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(z.getDataType()).getType(), ElementaryType::Type::INT);
            }
            {// local variables
                const auto &f = *(pou.getInterface()._name_to_variable.at("f"));
                ASSERT_EQ(f.getStorageType(), Variable::StorageType::LOCAL);
                ASSERT_EQ(dynamic_cast<const DerivedType &>(f.getDataType()).getName(), "Fb1");
            }
            {// output variables
                const auto &a = *(pou.getInterface()._name_to_variable.at("a"));
                ASSERT_EQ(a.getStorageType(), Variable::StorageType::OUTPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(a.getDataType()).getType(), ElementaryType::Type::INT);
            }
        }
        {
            const auto &pou = project->getPou("P");
            ASSERT_EQ(pou.getKind(), Pou::Kind::PROGRAM);
            {// input variables
                const auto &x = *(pou.getInterface()._name_to_variable.at("x"));
                ASSERT_EQ(x.getStorageType(), Variable::StorageType::INPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(x.getDataType()).getType(), ElementaryType::Type::INT);
            }
            {// local variables
                const auto &f = *(pou.getInterface()._name_to_variable.at("f"));
                ASSERT_EQ(f.getStorageType(), Variable::StorageType::LOCAL);
                ASSERT_EQ(dynamic_cast<const DerivedType &>(f.getDataType()).getName(), "Fb1");
                const auto &g = *(pou.getInterface()._name_to_variable.at("g"));
                ASSERT_EQ(g.getStorageType(), Variable::StorageType::LOCAL);
                ASSERT_EQ(dynamic_cast<const DerivedType &>(g.getDataType()).getName(), "Fb2");
                const auto &h = *(pou.getInterface()._name_to_variable.at("h"));
                ASSERT_EQ(h.getStorageType(), Variable::StorageType::LOCAL);
                ASSERT_EQ(dynamic_cast<const DerivedType &>(h.getDataType()).getName(), "Fb3");
                const auto &z = *(pou.getInterface()._name_to_variable.at("z"));
                ASSERT_EQ(z.getStorageType(), Variable::StorageType::LOCAL);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(z.getDataType()).getType(), ElementaryType::Type::INT);
            }
            {// output variables
                const auto &y = *(pou.getInterface()._name_to_variable.at("y"));
                ASSERT_EQ(y.getStorageType(), Variable::StorageType::OUTPUT);
                ASSERT_EQ(dynamic_cast<const ElementaryType &>(y.getDataType()).getType(), ElementaryType::Type::INT);
            }
        }
    }
}

TEST_F(TestLibCompiler, NoRecursion) {
    ASSERT_ANY_THROW(getProject("../../test/file/call_recursion.st"));
}