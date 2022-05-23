#include <gtest/gtest.h>

#include "cfg/cfg.h"
#include "compiler/compiler.h"
#include "ir/expression/binary_expression.h"
#include "ir/expression/constant/boolean_constant.h"
#include "ir/expression/constant/integer_constant.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/goto_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "ir/instruction/while_instruction.h"
#include "ir/project.h"
#include "ir/type/derived_type.h"
#include "ir/type/elementary_type.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>

using namespace ir;

class TestLibIr : public ::testing::Test {
public:
    TestLibIr() : ::testing::Test() {}

protected:
    void SetUp() override {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/ir.txt", true);
        file_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);

        auto logger =
                std::make_shared<spdlog::logger>("Intermediate Representation", std::begin(sinks), std::end(sinks));
        logger->set_level(spdlog::level::trace);

        spdlog::register_logger(logger);
    }

    void TearDown() override {
        spdlog::drop("Intermediate Representation");
    }

    static std::unique_ptr<Project> getProject(std::string const &path) {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        const std::string &source_code = buffer.str();
        auto project = Compiler::compile(source_code);
        return project;
    }
};

TEST_F(TestLibIr, Module) {
    {
        auto project = getProject("../../test/file/ir_empty_if.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        auto x = module.getInterface().getVariable("x");
        auto y = module.getInterface().getVariable("y");
        ASSERT_EQ(x->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + x->getName());
        ASSERT_EQ(y->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + y->getName());
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 1);
        ASSERT_TRUE(module._name_to_module.empty());
    }
    {
        auto project = getProject("../../test/file/ir_empty_if_else_if_else.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        auto x = module.getInterface().getVariable("x");
        auto y = module.getInterface().getVariable("y");
        ASSERT_EQ(x->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + x->getName());
        ASSERT_EQ(y->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + y->getName());
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 6);
        {
            ASSERT_EQ(module._label_to_instruction.at(1)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(1));
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 2);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 4);
        }
        ASSERT_EQ(module._label_to_instruction.at(2)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(3)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(3));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        ASSERT_EQ(module._label_to_instruction.at(4)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(5)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(5));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        ASSERT_TRUE(module._name_to_module.empty());
    }
    {
        auto project = getProject("../../test/file/ir_empty_if_empty_else_if_else.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        auto x = module.getInterface().getVariable("x");
        auto y = module.getInterface().getVariable("y");
        ASSERT_EQ(x->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + x->getName());
        ASSERT_EQ(y->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + y->getName());
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 4);
        {
            ASSERT_EQ(module._label_to_instruction.at(1)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(1));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "x > 1");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 4);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 2);
        }
        ASSERT_EQ(module._label_to_instruction.at(2)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(3)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(3));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        ASSERT_TRUE(module._name_to_module.empty());
    }
    {
        auto project = getProject("../../test/file/ir_if.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        auto x = module.getInterface().getVariable("x");
        auto y = module.getInterface().getVariable("y");
        ASSERT_EQ(x->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + x->getName());
        ASSERT_EQ(y->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + y->getName());
        ASSERT_EQ(x->hasInitialization(), true);
        ASSERT_EQ((dynamic_cast<const BooleanConstant &>(x->getInitialization())).getValue(), true);
        ASSERT_EQ(y->hasInitialization(), true);
        ASSERT_EQ((dynamic_cast<const IntegerConstant &>(y->getInitialization())).getValue(), 0);
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 6);
        {
            ASSERT_EQ(module._label_to_instruction.at(1)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(1));
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 2);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 4);
        }
        ASSERT_EQ(module._label_to_instruction.at(2)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(3)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(3));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        ASSERT_EQ(module._label_to_instruction.at(4)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(5)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(5));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        ASSERT_TRUE(module._name_to_module.empty());
    }
    {
        auto project = getProject("../../test/file/ir_if_numeral.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        auto x = module.getInterface().getVariable("x");
        auto y = module.getInterface().getVariable("y");
        ASSERT_EQ(x->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + x->getName());
        ASSERT_EQ(y->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + y->getName());
        ASSERT_EQ(x->hasInitialization(), true);
        ASSERT_EQ((dynamic_cast<const BooleanConstant &>(x->getInitialization())).getValue(), true);
        ASSERT_EQ(y->hasInitialization(), true);
        ASSERT_EQ((dynamic_cast<const IntegerConstant &>(y->getInitialization())).getValue(), 0);
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 8);
        ASSERT_EQ(module._label_to_instruction.at(1)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(2)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(2));
            ASSERT_EQ(goto_instruction.getLabel(), 3);
        }
        {
            ASSERT_EQ(module._label_to_instruction.at(3)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(3));
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 4);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 6);
        }
        ASSERT_EQ(module._label_to_instruction.at(4)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(5)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(5));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        ASSERT_EQ(module._label_to_instruction.at(6)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(7)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(7));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        ASSERT_TRUE(module._name_to_module.empty());
    }
    {
        auto project = getProject("../../test/file/ir_if_empty_then.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        auto x = module.getInterface().getVariable("x");
        auto y = module.getInterface().getVariable("y");
        ASSERT_EQ(x->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + x->getName());
        ASSERT_EQ(y->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + y->getName());
        ASSERT_EQ(x->hasInitialization(), false);
        ASSERT_EQ(y->hasInitialization(), false);
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 4);
        {
            ASSERT_EQ(module._label_to_instruction.at(1)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(1));
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 4);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 2);
        }
        ASSERT_EQ(module._label_to_instruction.at(2)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(3)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(3));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        ASSERT_TRUE(module._name_to_module.empty());
    }
    {
        auto project = getProject("../../test/file/ir_if_empty_else.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        auto x = module.getInterface().getVariable("x");
        auto y = module.getInterface().getVariable("y");
        ASSERT_EQ(x->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + x->getName());
        ASSERT_EQ(y->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + y->getName());
        ASSERT_EQ(x->hasInitialization(), false);
        ASSERT_EQ(y->hasInitialization(), false);
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 4);
        {
            ASSERT_EQ(module._label_to_instruction.at(1)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(1));
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 2);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 4);
        }
        ASSERT_EQ(module._label_to_instruction.at(2)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(3)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(3));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        ASSERT_TRUE(module._name_to_module.empty());
    }
    {
        auto project = getProject("../../test/file/ir_if_empty_else_if_else.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        auto x = module.getInterface().getVariable("x");
        auto y = module.getInterface().getVariable("y");
        ASSERT_EQ(x->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + x->getName());
        ASSERT_EQ(y->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + y->getName());
        ASSERT_EQ(x->hasInitialization(), false);
        ASSERT_EQ(y->hasInitialization(), false);
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 6);
        {
            ASSERT_EQ(module._label_to_instruction.at(1)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(1));
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 2);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 4);
        }
        ASSERT_EQ(module._label_to_instruction.at(2)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(3)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(3));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        ASSERT_EQ(module._label_to_instruction.at(4)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(5)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(5));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        ASSERT_TRUE(module._name_to_module.empty());
    }
    {
        auto project = getProject("../../test/file/ir_if_else_if.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        auto x = module.getInterface().getVariable("x");
        auto y = module.getInterface().getVariable("y");
        ASSERT_EQ(x->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + x->getName());
        ASSERT_EQ(y->getFullyQualifiedName(), module.getFullyQualifiedName() + "." + y->getName());
        ASSERT_EQ(x->hasInitialization(), false);
        ASSERT_EQ(y->hasInitialization(), false);
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 24);
        {
            ASSERT_EQ(module._label_to_instruction.at(1)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(1));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 2);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 4);
        }
        ASSERT_EQ(module._label_to_instruction.at(2)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(3)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(3));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        {
            ASSERT_EQ(module._label_to_instruction.at(4)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(4));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "x > 1");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 5);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 19);
        }
        ASSERT_EQ(module._label_to_instruction.at(5)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(6)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(6));
            ASSERT_EQ(goto_instruction.getLabel(), 7);
        }
        {
            ASSERT_EQ(module._label_to_instruction.at(7)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(7));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "x > 5");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 8);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 10);
        }
        ASSERT_EQ(module._label_to_instruction.at(8)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(9)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(9));
            ASSERT_EQ(goto_instruction.getLabel(), 24);
        }
        {
            ASSERT_EQ(module._label_to_instruction.at(10)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(10));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "x > 40");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 11);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 13);
        }
        ASSERT_EQ(module._label_to_instruction.at(11)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(12)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(12));
            ASSERT_EQ(goto_instruction.getLabel(), 24);
        }
        {
            ASSERT_EQ(module._label_to_instruction.at(13)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(13));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "x > 10");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 14);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 17);
        }
        {
            ASSERT_EQ(module._label_to_instruction.at(14)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(14));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "x > 1");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 15);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 24);
        }
        ASSERT_EQ(module._label_to_instruction.at(15)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(16)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(16));
            ASSERT_EQ(goto_instruction.getLabel(), 24);
        }
        ASSERT_EQ(module._label_to_instruction.at(17)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(18)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(18));
            ASSERT_EQ(goto_instruction.getLabel(), 24);
        }
        {
            ASSERT_EQ(module._label_to_instruction.at(19)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(19));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "x > 2");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 20);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 22);
        }
        ASSERT_EQ(module._label_to_instruction.at(20)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(21)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(21));
            ASSERT_EQ(goto_instruction.getLabel(), 24);
        }
        ASSERT_EQ(module._label_to_instruction.at(22)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(23)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(23));
            ASSERT_EQ(goto_instruction.getLabel(), 24);
        }
        ASSERT_TRUE(module._name_to_module.empty());
    }
    {
        auto project = getProject("../../test/file/ir_case.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 18);
        {
            ASSERT_EQ(module._label_to_instruction.at(1)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(1));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "code = 0");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 2);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 4);
        }
        ASSERT_EQ(module._label_to_instruction.at(2)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(3)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(3));
            ASSERT_EQ(goto_instruction.getLabel(), 16);
        }
        {
            ASSERT_EQ(module._label_to_instruction.at(4)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(4));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "code = 1");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 5);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 7);
        }
        ASSERT_EQ(module._label_to_instruction.at(5)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(6)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(6));
            ASSERT_EQ(goto_instruction.getLabel(), 16);
        }
        {
            ASSERT_EQ(module._label_to_instruction.at(7)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(7));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "code = 2");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 8);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 10);
        }
        ASSERT_EQ(module._label_to_instruction.at(8)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(9)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(9));
            ASSERT_EQ(goto_instruction.getLabel(), 16);
        }
        {
            ASSERT_EQ(module._label_to_instruction.at(10)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(10));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "code = 3");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 11);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 13);
        }
        ASSERT_EQ(module._label_to_instruction.at(11)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(12)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(12));
            ASSERT_EQ(goto_instruction.getLabel(), 16);
        }
        {
            ASSERT_EQ(module._label_to_instruction.at(13)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(13));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "code = 4");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 14);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 16);
        }
        ASSERT_EQ(module._label_to_instruction.at(14)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(15)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(15));
            ASSERT_EQ(goto_instruction.getLabel(), 16);
        }
        ASSERT_EQ(module._label_to_instruction.at(16)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(17)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(17));
            ASSERT_EQ(goto_instruction.getLabel(), module._exit_label);
        }
        ASSERT_TRUE(module._name_to_module.empty());
    }
    {
        auto project = getProject("../../test/file/ir_while.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 9);
        {
            ASSERT_EQ(module._label_to_instruction.at(1)->getKind(), Instruction::Kind::WHILE);
            const auto &while_instruction = dynamic_cast<const WhileInstruction &>(*module._label_to_instruction.at(1));
            std::stringstream str;
            while_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "i > 0");
            ASSERT_EQ(while_instruction.getBodyEntryLabel(), 2);
            ASSERT_EQ(while_instruction.getGotoWhileExit().getLabel(), 9);
        }
        {
            ASSERT_EQ(module._label_to_instruction.at(2)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(2));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "x = 1");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 3);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 5);
        }
        ASSERT_EQ(module._label_to_instruction.at(3)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(4)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(4));
            ASSERT_EQ(goto_instruction.getLabel(), 7);
        }
        ASSERT_EQ(module._label_to_instruction.at(5)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(6)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(6));
            ASSERT_EQ(goto_instruction.getLabel(), 7);
        }
        ASSERT_EQ(module._label_to_instruction.at(7)->getKind(), Instruction::Kind::ASSIGNMENT);
        {
            ASSERT_EQ(module._label_to_instruction.at(8)->getKind(), Instruction::Kind::GOTO);
            const auto &goto_instruction = dynamic_cast<const GotoInstruction &>(*module._label_to_instruction.at(8));
            ASSERT_EQ(goto_instruction.getLabel(), 1);
        }
        ASSERT_TRUE(module._name_to_module.empty());
    }
    {
        auto project = getProject("../../test/file/ir_call.st");
        for (auto it = project->modulesBegin(); it != project->modulesEnd(); ++it) {
            std::cout << *it << std::endl;
        }
        const auto &main = project->getMain();
        for (auto it = main.flattenedInterfaceBegin(); it != main.flattenedInterfaceEnd(); ++it) {
            std::cout << it->getFullyQualifiedName() << std::endl;
        }
        {
            const auto &module = project->getModule("Fb2");
            ASSERT_EQ(module.getKind(), Module::Kind::FUNCTION_BLOCK);
            ASSERT_EQ(module._entry_label, 0);
            ASSERT_EQ(module._exit_label, 5);
            ASSERT_TRUE(module._name_to_module.empty());
        }
        {
            const auto &module = project->getModule("Fb1");
            ASSERT_EQ(module.getKind(), Module::Kind::FUNCTION_BLOCK);
            ASSERT_EQ(module._entry_label, 6);
            ASSERT_EQ(module._exit_label, 23);
            ASSERT_TRUE(!module._name_to_module.empty());
        }
        {
            const auto &module = project->getModule("P");
            ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
            ASSERT_EQ(module._entry_label, 24);
            ASSERT_EQ(module._exit_label, 57);
            ASSERT_TRUE(!module._name_to_module.empty());
        }
    }
    {
        auto project = getProject("../../test/file/ir_enum.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 22);
        {
            ASSERT_EQ(module._label_to_instruction.at(1)->getKind(), Instruction::Kind::IF);
            const auto &if_instruction = dynamic_cast<const IfInstruction &>(*module._label_to_instruction.at(1));
            std::stringstream str;
            if_instruction.getExpression().print(str);
            ASSERT_EQ(str.str(), "State = Green_Lamp");
            ASSERT_EQ(if_instruction.getGotoThen().getLabel(), 2);
            ASSERT_EQ(if_instruction.getGotoElse().getLabel(), 4);
        }
    }
    {
        auto project = getProject("../../test/file/regression/ir_hagedorn.st");
        const auto &module = project->getModule("P");
        ASSERT_EQ(module.getKind(), Module::Kind::PROGRAM);
        ASSERT_EQ(module._entry_label, 0);
        ASSERT_EQ(module._exit_label, 12);
    }
}

TEST_F(TestLibIr, CallCompletion) {
    auto project = getProject("../../test/file/ir/call_completion.st");
    const auto &module = project->getModule("P");
    ASSERT_EQ(0, 1);
}

TEST_F(TestLibIr, Type) {
    auto et1 = std::make_unique<ElementaryType>("INT");
    auto et2 = std::make_unique<ElementaryType>("INT");
    ASSERT_TRUE(*et1 == *et2);

    std::unique_ptr<DataType> dt1 = std::make_unique<ElementaryType>("INT");
    ASSERT_TRUE(*dt1 == *et1);

    std::unique_ptr<DataType> dt2 = std::make_unique<DerivedType>("Fb1");
    ASSERT_FALSE(*dt1 == *dt2);

    ASSERT_TRUE(*dt2 == *std::make_unique<DerivedType>("Fb1"));
}