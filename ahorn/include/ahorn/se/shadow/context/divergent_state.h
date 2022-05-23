#ifndef AHORN_DIVERGENT_STATE_H
#define AHORN_DIVERGENT_STATE_H

#include "se/shadow/context/state.h"

#include "z3++.h"

#include "spdlog/fmt/bundled/color.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

#include <map>
#include <memory>

namespace se::shadow {
    class DivergentState : public State {
    public:
        DivergentState(const Vertex &vertex, std::map<std::string, z3::expr> concrete_valuations,
                       std::map<std::string, std::pair<z3::expr, z3::expr>> concrete_shadow_valuations,
                       std::map<std::string, z3::expr> symbolic_valuations,
                       std::map<std::string, std::pair<z3::expr, z3::expr>> symbolic_shadow_valuations,
                       std::vector<z3::expr> path_constraint, std::vector<z3::expr> old_path_constraint,
                       std::vector<z3::expr> new_path_constraint,
                       std::map<std::string, unsigned int> flattened_name_to_version);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const DivergentState &divergent_state) {
            return divergent_state.print(os);
        }

        const std::map<std::string, std::pair<z3::expr, z3::expr>> &getConcreteShadowValuations() const;

        void setConcreteShadowValuation(const std::string &contextualized_name, const z3::expr &old_valuation,
                                        const z3::expr &new_valuation);

        const std::map<std::string, std::pair<z3::expr, z3::expr>> &getSymbolicShadowValuations() const;

        void setSymbolicShadowValuation(const std::string &contextualized_name, const z3::expr &old_valuation,
                                        const z3::expr &new_valuation);

        const std::vector<z3::expr> &getOldPathConstraint() const;

        void pushOldPathConstraint(const z3::expr &expression);

        const std::vector<z3::expr> &getNewPathConstraint() const;

        void pushNewPathConstraint(const z3::expr &expression);

        std::unique_ptr<DivergentState> divergentFork(Solver &solver, const Vertex &vertex, z3::model &model,
                                                      const z3::expr &old_encoded_expression,
                                                      const z3::expr &new_encoded_expression) const;

    private:
        std::map<std::string, std::pair<z3::expr, z3::expr>> _concrete_shadow_valuations;
        std::map<std::string, std::pair<z3::expr, z3::expr>> _symbolic_shadow_valuations;
        std::vector<z3::expr> _old_path_constraint;
        std::vector<z3::expr> _new_path_constraint;
    };
}// namespace se::shadow

#endif//AHORN_DIVERGENT_STATE_H
