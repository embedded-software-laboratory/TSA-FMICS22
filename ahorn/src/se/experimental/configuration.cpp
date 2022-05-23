#include "se/experimental/configuration.h"

#include <sstream>

using namespace se;

Configuration::Configuration(EngineMode engine_mode, StepSize step_size, ExplorationHeuristic exploration_heuristic,
                             ExecutionMode execution_mode, EncodingMode encoding_mode,
                             SummarizationMode summarization_mode, BlockEncoding block_encoding,
                             MergeStrategy merge_strategy, ShadowProcessingMode shadow_processing_mode)
    : _engine_mode(engine_mode), _step_size(step_size), _exploration_heuristic(exploration_heuristic),
      _execution_mode(execution_mode), _encoding_mode(encoding_mode), _summarization_mode(summarization_mode),
      _block_encoding(block_encoding), _merge_strategy(merge_strategy),
      _shadow_processing_mode(shadow_processing_mode) {}

std::ostream &Configuration::print(std::ostream &os) const {
    std::stringstream str;
    str << "{\n";
    str << "\tEngine:\n";
    str << "\t\tEngine mode: ";
    switch (_engine_mode) {
        case EngineMode::COMPOSITIONAL: {
            str << "compositional";
            break;
        }
        case EngineMode::SHADOW: {
            str << "shadow";
            break;
        }
        default:
            throw std::runtime_error("Invalid engine mode configured.");
    }
    str << ",\n";
    str << "\t\tStep size: ";
    switch (_step_size) {
        case StepSize::INSTRUCTION: {
            str << "instruction";
            break;
        }
        case StepSize::CYCLE: {
            str << "cycle";
            break;
        }
        default:
            throw std::runtime_error("Invalid step size configured.");
    }
    str << ",\n";
    str << "\tExplorer:\n";
    str << "\t\tHeuristic: ";
    switch (_exploration_heuristic) {
        case ExplorationHeuristic::DEPTH_FIRST: {
            str << "depth-first";
            break;
        }
        case ExplorationHeuristic::BREADTH_FIRST: {
            str << "breadth-first";
            break;
        }
        default:
            throw std::runtime_error("Invalid heuristic configured.");
    }
    str << ",\n";
    str << "\tExecutor:\n";
    str << "\t\tExecution mode: ";
    switch (_execution_mode) {
        case ExecutionMode::COMPOSITIONAL: {
            str << "compositional";
            break;
        }
        case ExecutionMode::SHADOW: {
            str << "shadow";
            break;
        }
        default:
            throw std::runtime_error("Invalid execution mode configured.");
    }
    str << ",\n";
    str << "\t\tEncoding mode: ";
    switch (_encoding_mode) {
        case EncodingMode::NONE: {
            str << "none";
            break;
        }
        case EncodingMode::VERIFICATION_CONDITION_GENERATION: {
            str << "verification condition generation";
            break;
        }
        default:
            throw std::runtime_error("Invalid encoding mode configured.");
    }
    str << ",\n";
    str << "\t\tSummarization mode: ";
    switch (_summarization_mode) {
        case SummarizationMode::NONE: {
            str << "none";
            break;
        }
        case SummarizationMode::FUNCTION_BLOCK: {
            str << "function block";
            break;
        }
        default:
            throw std::runtime_error("Invalid summarization mode configured.");
    }
    str << ",\n";
    str << "\t\tBlock encoding: ";
    switch (_block_encoding) {
        case BlockEncoding::SINGLE: {
            str << "single";
            break;
        }
        case BlockEncoding::BASIC: {
            str << "basic";
            break;
        }
        default:
            throw std::runtime_error("Invalid block encoding configured.");
    }
    str << ",\n";
    str << "\tMerger:\n";
    str << "\t\tMerge strategy: ";
    switch (_merge_strategy) {
        case MergeStrategy::AT_ALL_JOIN_POINTS: {
            str << "at all join points";
            break;
        }
        case MergeStrategy::ONLY_AT_CYCLE_END: {
            str << "only at cycle end";
            break;
        }
        default:
            throw std::runtime_error("Invalid merge strategy configured.");
    }
    str << "\n";
    str << "}";
    return os << str.str();
}

Configuration::EngineMode Configuration::getEngineMode() const {
    return _engine_mode;
}

Configuration::StepSize Configuration::getStepSize() const {
    return _step_size;
}

Configuration::ExplorationHeuristic Configuration::getExplorationHeuristic() const {
    return _exploration_heuristic;
}

Configuration::ExecutionMode Configuration::getExecutionMode() const {
    return _execution_mode;
}

Configuration::EncodingMode Configuration::getEncodingMode() const {
    return _encoding_mode;
}

Configuration::SummarizationMode Configuration::getSummarizationMode() const {
    return _summarization_mode;
}

Configuration::BlockEncoding Configuration::getBlockEncoding() const {
    return _block_encoding;
}

Configuration::MergeStrategy Configuration::getMergeStrategy() const {
    return _merge_strategy;
}

void Configuration::setShadowProcessingMode(ShadowProcessingMode shadow_processing_mode) {
    _shadow_processing_mode = shadow_processing_mode;
}

Configuration::ShadowProcessingMode Configuration::getShadowProcessingMode() const {
    return _shadow_processing_mode;
}