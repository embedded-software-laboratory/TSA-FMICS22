#ifndef AHORN_SMART_CONTEXT_H
#define AHORN_SMART_CONTEXT_H

#include "se/smart/context/frame.h"
#include "se/smart/context/state.h"

#include <deque>
#include <memory>

namespace se::smart {
    class Context {
    public:
        // XXX default constructor disabled
        Context() = delete;
        // XXX copy constructor disabled
        Context(const Context &other) = delete;
        // XXX copy assignment disabled
        Context &operator=(const Context &) = delete;

        Context(std::map<std::string, unsigned int> flattened_name_to_version,
                std::set<std::string> whole_program_inputs, unsigned int cycle, std::unique_ptr<State> state,
                std::deque<std::shared_ptr<Frame>> frame_stack);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Context &context) {
            return context.print(os);
        }

        unsigned int getVersion(const std::string &flattened_name) const;

        void setVersion(const std::string &flattened_name, unsigned int version);

        bool isWholeProgramInput(const std::string &flattened_name) const;

        unsigned int getCycle() const;

        void setCycle(unsigned int cycle);

        State &getState() const;

        const Frame &getFrame() const;

        void pushFrame(std::shared_ptr<Frame> frame);

        void popFrame();

        int getExecutedConditionals() const;

        void setExecutedConditionals(int executed_conditionals, unsigned int label);

        void resetExecutedConditionals();

        std::vector<unsigned int> &getStack();

        void backtrack(int j, z3::model &model, std::vector<z3::expr> path_constraint);

    private:
        // "locally" managed variable versioning for implicit SSA-form
        std::map<std::string, unsigned int> _flattened_name_to_version;
        // set of whole-program inputs, i.e., "truly symbolic" input variables
        std::set<std::string> _whole_program_inputs;
        unsigned int _cycle;
        std::unique_ptr<State> _state;
        std::deque<std::shared_ptr<Frame>> _frame_stack;
        int _executed_conditionals;
        std::vector<unsigned int> _stack;
        std::map<int, unsigned int> _executed_conditional_to_label;
    };
}// namespace se::smart

#endif//AHORN_SMART_CONTEXT_H
