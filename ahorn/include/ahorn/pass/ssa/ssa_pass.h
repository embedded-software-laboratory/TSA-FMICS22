#ifndef AHORN_SSA_PASS_H
#define AHORN_SSA_PASS_H

#include "cfg/cfg.h"
#include "ir/instruction/instruction.h"
#include "ir/instruction/instruction_visitor.h"
#include "ir/variable.h"
#include "pass/ssa/ssa_encoder.h"

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace pass {
    // XXX
    class SsaPass : private ir::InstructionVisitor {
    private:
        friend class SsaEncoder;

    private:
        static const std::string CONSTANT_VARIABLE_NAME_PREFIX;

    private:
        class PhiCandidate {
        public:
            PhiCandidate(std::string name, unsigned int label, int value)
                : _name(std::move(name)), _label(label), _value(value), _values_of_operands(std::vector<int>()),
                  _values_of_users(std::vector<int>()){};

            bool operator==(const PhiCandidate &phi_candidate) const {
                return _name == phi_candidate._name && _label == phi_candidate._label && _value == phi_candidate._value;
            }

            bool operator!=(const PhiCandidate &phi_candidate) const {
                return !(*this == phi_candidate);
            }

            std::string getName() const {
                return _name;
            }

            unsigned int getLabel() const {
                return _label;
            }

            int getValue() const {
                return _value;
            }

            const std::vector<int> getValuesOfOperands() const {
                return _values_of_operands;
            }

            const std::vector<int> getValuesOfUsers() const {
                return _values_of_users;
            }

            void appendValuesOfOperands(int value) {
                _values_of_operands.push_back(value);
            }

            void addValuesOfUsers(int value) {
                _values_of_users.push_back(value);
            }

            void replaceValueOfUsedPhiCandidate(int value, int same) {
                for (int &value_of_operand : _values_of_operands) {
                    if (value_of_operand == value) {
                        value_of_operand = same;
                    }
                }
            }

        private:
            std::string _name;
            unsigned int _label;
            int _value;

            std::vector<int> _values_of_operands;
            std::vector<int> _values_of_users;
        };

    public:
        SsaPass();

        std::shared_ptr<Cfg> apply(const Cfg &cfg);

    private:
        void insertInstructionAtLabel(std::unique_ptr<ir::Instruction> instruction, unsigned int label,
                                      bool in_front = false);

        void writeVariable(const std::string &name, unsigned int label, int value);

        int readVariable(const std::string &name, unsigned int label);

        int readVariableRecursive(const std::string &name, unsigned int label);

        bool isSealable(unsigned int label) const;

        void sealBasicBlock(unsigned int label);

        int placeOperandlessPhi(const std::string &name, unsigned int label);

        int addPhiOperands(const std::string &name, int value);

        int tryRemoveTrivialPhi(PhiCandidate &phi_candidate);

    private:
        void visit(const ir::AssignmentInstruction &instruction) override;
        void visit(const ir::CallInstruction &instruction) override;
        void visit(const ir::IfInstruction &instruction) override;
        void visit(const ir::SequenceInstruction &instruction) override;
        void visit(const ir::WhileInstruction &instruction) override;
        void visit(const ir::GotoInstruction &instruction) override;
        void visit(const ir::HavocInstruction &instruction) override;

    private:
        std::unique_ptr<SsaEncoder> _ssa_encoder;

        const Cfg *_cfg;

        int _value;

        unsigned int _label;

        std::map<int, std::shared_ptr<ir::Variable>> _value_to_variable;

        std::map<unsigned int, std::vector<std::unique_ptr<ir::Instruction>>> _label_to_instructions;

        std::set<unsigned int> _filled_basic_blocks;
        std::set<unsigned int> _sealed_basic_blocks;

        // maps variable name to basic block label to ssa value
        std::map<std::string, std::map<unsigned int, int>> _current_definitions;

        std::map<int, std::shared_ptr<PhiCandidate>> _value_to_phi_candidate;

        // XXX maps type-representative cfg names to the corresponding SSA'ed cfg
        std::map<std::string, std::shared_ptr<Cfg>> _type_representative_name_to_cfg;
    };
}// namespace pass

#endif//AHORN_SSA_PASS_H
