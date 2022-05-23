#ifndef AHORN_SMART_FRAME_H
#define AHORN_SMART_FRAME_H

#include "cfg/cfg.h"

namespace se::smart {
    class Frame {
    public:
        // XXX default constructor disabled
        Frame() = delete;
        // XXX copy constructor disabled
        Frame(const Frame &other) = delete;
        // XXX copy assignment disabled
        Frame &operator=(const Frame &) = delete;

        Frame(const Cfg &cfg, std::string scope, unsigned int return_label, int executed_conditionals);

        std::ostream &print(std::ostream &os) const;

        friend std::ostream &operator<<(std::ostream &os, const Frame &frame) {
            return frame.print(os);
        }

        const Cfg &getCfg() const;

        const std::string &getScope() const;

        unsigned int getReturnLabel() const;

        int getExecutedConditionals() const;

    private:
        const Cfg &_cfg;
        std::string _scope;
        unsigned int _return_label;
        int _executed_conditionals;
    };
}// namespace se::smart

#endif//AHORN_SMART_FRAME_H
