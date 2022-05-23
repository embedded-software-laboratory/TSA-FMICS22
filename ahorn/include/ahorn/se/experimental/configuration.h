#ifndef AHORN_CONFIGURATION_H
#define AHORN_CONFIGURATION_H

#include <ostream>

namespace se {
    class Configuration {
    public:
        // Engine
        enum class EngineMode { COMPOSITIONAL, SHADOW };

        enum class StepSize { INSTRUCTION, CYCLE };

        enum class TerminationCriteria { TIME_OUT, CYCLE_BOUND, COVERAGE };

        // Explorer
        enum class ExplorationHeuristic { DEPTH_FIRST, BREADTH_FIRST };

        // Executor
        enum class ExecutionMode { COMPOSITIONAL, SHADOW };

        enum class EncodingMode { NONE, VERIFICATION_CONDITION_GENERATION };

        enum class SummarizationMode { NONE, FUNCTION_BLOCK };

        enum class BlockEncoding { SINGLE, BASIC };

        // Merger
        enum class MergeStrategy { AT_ALL_JOIN_POINTS, ONLY_AT_CYCLE_END };

        // Evaluator and Encoder
        enum class ShadowProcessingMode { NONE, OLD, NEW, BOTH };

        Configuration(EngineMode engine_mode, StepSize step_size, ExplorationHeuristic exploration_heuristic,
                      ExecutionMode execution_mode, EncodingMode encoding_mode, SummarizationMode summarization_mode,
                      BlockEncoding block_encoding, MergeStrategy merge_strategy,
                      ShadowProcessingMode shadow_processing_mode = ShadowProcessingMode::NONE);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Configuration &configuration) {
            return configuration.print(os);
        }

        // Engine
        EngineMode getEngineMode() const;

        StepSize getStepSize() const;

        // Explorer
        ExplorationHeuristic getExplorationHeuristic() const;

        // Executor
        ExecutionMode getExecutionMode() const;

        EncodingMode getEncodingMode() const;

        SummarizationMode getSummarizationMode() const;

        BlockEncoding getBlockEncoding() const;

        // Merger
        MergeStrategy getMergeStrategy() const;

        // Encoder and Evaluator
        void setShadowProcessingMode(ShadowProcessingMode shadow_processing_mode);

        ShadowProcessingMode getShadowProcessingMode() const;

    private:
        // Engine
        EngineMode _engine_mode;

        StepSize _step_size;

        // Explorer
        ExplorationHeuristic _exploration_heuristic;

        // Executor
        ExecutionMode _execution_mode;

        EncodingMode _encoding_mode;

        SummarizationMode _summarization_mode;

        BlockEncoding _block_encoding;

        // Merger
        MergeStrategy _merge_strategy;

        // Encoder and Evaluator
        ShadowProcessingMode _shadow_processing_mode;
    };
}// namespace se

#endif//AHORN_CONFIGURATION_H
