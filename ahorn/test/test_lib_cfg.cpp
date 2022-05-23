#include <gtest/gtest.h>

#include "cfg/builder.h"
#include "compiler/compiler.h"
#include "ir/expression/field_access.h"
#include "ir/instruction/assignment_instruction.h"
#include "ir/instruction/call_instruction.h"
#include "ir/instruction/if_instruction.h"
#include "ir/type/derived_type.h"
#include "ir/type/elementary_type.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <fstream>

class TestLibCfg : public ::testing::Test {
public:
    TestLibCfg() : ::testing::Test() {}

protected:
    void SetUp() override {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/cfg.txt", true);
        file_sink->set_level(spdlog::level::trace);

        std::vector<spdlog::sink_ptr> sinks;
        sinks.push_back(console_sink);
        sinks.push_back(file_sink);

        auto logger = std::make_shared<spdlog::logger>("Control Flow Graph", std::begin(sinks), std::end(sinks));
        logger->set_level(spdlog::level::trace);

        spdlog::register_logger(logger);
    }

    void TearDown() override {
        spdlog::drop("Control Flow Graph");
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

TEST_F(TestLibCfg, Empty_If) {
    auto cfg = getCfg("../../test/file/ir_empty_if.st");
    ASSERT_EQ(cfg->_entry_label, 0);
    ASSERT_EQ(cfg->_exit_label, 1);
    ASSERT_EQ(cfg->_label_to_vertex.at(0)->getType(), Vertex::Type::ENTRY);
}

TEST_F(TestLibCfg, If) {
    auto cfg = getCfg("../../test/file/ir_if.st");
    {
        const auto &elementary_type =
                dynamic_cast<const ir::ElementaryType &>(cfg->getInterface()._name_to_variable.at("x")->getDataType());
        ASSERT_EQ(elementary_type.getType(), ir::ElementaryType::Type::BOOL);
    }
    {
        const auto &elementary_type =
                dynamic_cast<const ir::ElementaryType &>(cfg->getInterface()._name_to_variable.at("y")->getDataType());
        ASSERT_EQ(elementary_type.getType(), ir::ElementaryType::Type::INT);
    }
    ASSERT_EQ(cfg->_entry_label, 0);
    ASSERT_EQ(cfg->_exit_label, 6);
    ASSERT_EQ(cfg->_label_to_vertex.at(cfg->_entry_label)->getType(), Vertex::Type::ENTRY);
    ASSERT_EQ(cfg->_label_to_vertex.at(cfg->_exit_label)->getType(), Vertex::Type::EXIT);
    {
        auto *if_instruction = dynamic_cast<ir::IfInstruction *>(cfg->_label_to_vertex.at(1)->getInstruction());
        ASSERT_EQ(if_instruction->getThenLabel(), 2);
        ASSERT_EQ(if_instruction->getElseLabel(), 4);
        {
            auto outgoing_edges = cfg->getOutgoingEdges(1);
            ASSERT_EQ(outgoing_edges.size(), 2);
            // XXX the order is determined in the builder.cpp during construction of the cfg
            ASSERT_EQ(outgoing_edges.at(0)->getType(), Edge::Type::TRUE_BRANCH);
            ASSERT_EQ(cfg->getTrueEdge(1).getType(), Edge::Type::TRUE_BRANCH);
            ASSERT_EQ(outgoing_edges.at(1)->getType(), Edge::Type::FALSE_BRANCH);
            ASSERT_EQ(cfg->getFalseEdge(1).getType(), Edge::Type::FALSE_BRANCH);
        }
        auto succeeding_labels = cfg->getSucceedingLabels(1);
        ASSERT_EQ(succeeding_labels.at(0), 2);
        ASSERT_EQ(succeeding_labels.at(1), 4);
        {
            int label = if_instruction->getThenLabel();
            auto *assignment_instruction =
                    dynamic_cast<ir::AssignmentInstruction *>(cfg->_label_to_vertex.at(label)->getInstruction());
            ASSERT_TRUE(assignment_instruction != nullptr);
            auto incoming_edges = cfg->getIncomingEdges(label);
            ASSERT_EQ(incoming_edges.size(), 1);
            auto outgoing_edges = cfg->getOutgoingEdges(label);
            ASSERT_EQ(outgoing_edges.size(), 1);
            const auto &lhs = dynamic_cast<const ir::VariableAccess &>(assignment_instruction->getVariableReference());
            ASSERT_EQ(cfg->getName(), lhs.getVariable()->getParent()->getName());
        }
        {
            int label = if_instruction->getElseLabel();
            auto *assignment_instruction =
                    dynamic_cast<ir::AssignmentInstruction *>(cfg->_label_to_vertex.at(label)->getInstruction());
            ASSERT_TRUE(assignment_instruction != nullptr);
            auto incoming_edges = cfg->getIncomingEdges(label);
            ASSERT_EQ(incoming_edges.size(), 1);
            auto outgoing_edges = cfg->getOutgoingEdges(label);
            ASSERT_EQ(outgoing_edges.size(), 1);
            const auto &lhs = dynamic_cast<const ir::VariableAccess &>(assignment_instruction->getVariableReference());
            ASSERT_EQ(cfg->getName(), lhs.getVariable()->getParent()->getName());
        }
    }
}

TEST_F(TestLibCfg, If_Else_If) {
    auto cfg = getCfg("../../test/file/ir_if_else_if.st");
    ASSERT_EQ(cfg->_entry_label, 0);
    ASSERT_EQ(cfg->_exit_label, 24);
    ASSERT_EQ(cfg->_label_to_vertex.at(cfg->_entry_label)->getType(), Vertex::Type::ENTRY);
    ASSERT_EQ(cfg->_label_to_vertex.at(cfg->_exit_label)->getType(), Vertex::Type::EXIT);
}

TEST_F(TestLibCfg, Call) {
    auto cfg = getCfg("../../test/file/ir_call.st");
    std::cout << cfg->toDot() << std::endl;
    {// P
        ASSERT_EQ(cfg->_entry_label, 24);
        ASSERT_EQ(cfg->_exit_label, 57);
        ASSERT_EQ(cfg->_label_to_vertex.at(cfg->_entry_label)->getType(), Vertex::Type::ENTRY);
        ASSERT_EQ(cfg->_label_to_vertex.at(cfg->_exit_label)->getType(), Vertex::Type::EXIT);
        {
            auto *call_instruction =
                    dynamic_cast<ir::CallInstruction *>(cfg->_label_to_vertex.at(31)->getInstruction());
            ASSERT_EQ(call_instruction->getInterproceduralLabel(), 6);
            ASSERT_EQ(call_instruction->getIntraproceduralLabel(), 32);
            auto callee_variable = call_instruction->getVariableAccess().getVariable();
            ASSERT_EQ(callee_variable->getName(), "f");
            const auto &derived_type = dynamic_cast<const ir::DerivedType &>(callee_variable->getDataType());
            std::string type_representative_name = derived_type.getFullyQualifiedName();
            const auto &callee_cfg = cfg->getCfg(type_representative_name);
            ASSERT_EQ(callee_cfg.getName(), "Fb1");
            ASSERT_EQ(callee_cfg._entry_label, 6);
            ASSERT_EQ(callee_cfg._exit_label, 23);
        }
    }
    {// Fb1
        const auto &nested_cfg = cfg->getCfg("Fb1");
        ASSERT_EQ(nested_cfg._entry_label, 6);
        ASSERT_EQ(nested_cfg._exit_label, 23);
        {
            auto *assignment_instruction =
                    dynamic_cast<ir::AssignmentInstruction *>(nested_cfg._label_to_vertex.at(9)->getInstruction());
            const auto &field_access =
                    dynamic_cast<const ir::FieldAccess &>(assignment_instruction->getVariableReference());
            ASSERT_EQ(field_access.getName(), "g.x");
        }
    }
    {// Fb2
        const auto &nested_cfg = cfg->getCfg("Fb2");
        ASSERT_EQ(nested_cfg._entry_label, 0);
        ASSERT_EQ(nested_cfg._exit_label, 5);
    }
}
