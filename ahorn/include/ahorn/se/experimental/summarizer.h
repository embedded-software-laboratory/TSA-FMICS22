#ifndef AHORN_SUMMARIZER_H
#define AHORN_SUMMARIZER_H

#include "se/experimental/context/context.h"
#include "se/experimental/summary.h"
#include "se/experimental/z3/manager.h"

#include <map>
#include <memory>
#include <vector>

namespace se {
    class Summarizer {
    public:
        // XXX default constructor disabled
        Summarizer() = delete;
        // XXX copy constructor disabled
        Summarizer(const Summarizer &other) = delete;
        // XXX copy assignment disabled
        Summarizer &operator=(const Summarizer &) = delete;

        explicit Summarizer(Manager &manager);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Summarizer &summarizer) {
            return summarizer.print(os);
        }

        void findApplicableSummary(const Context &context);

        void summarize(const Context &context);

    private:
        bool isSummaryApplicable(const Summary &summary,
                                 const std::map<std::string, std::shared_ptr<ConcreteExpression>>
                                         &reversioned_names_to_concrete_expression);

        static inline std::vector<std::vector<std::shared_ptr<AssumptionLiteralExpression>>>
        determineAssumptionLiteralPaths(const Context &context);

        std::unique_ptr<Summary> summarizePath(const Context &context,
                                               const std::vector<std::shared_ptr<AssumptionLiteralExpression>> &path) const;

        static inline std::string extractFlattenedName(const std::string &contextualized_name);

        static inline std::string extractUncycledName(const std::string &name);

    private:
        Manager *const _manager;

        std::map<std::string, std::vector<std::unique_ptr<Summary>>> _type_representative_name_to_summary;
    };
}// namespace se

#endif//AHORN_SUMMARIZER_H
