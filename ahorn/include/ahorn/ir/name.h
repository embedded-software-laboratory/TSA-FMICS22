#ifndef AHORN_NAME_H
#define AHORN_NAME_H

#include <string>

#include <memory>

namespace ir {
    class Name {
    public:
        virtual ~Name() = default;

        // disable copy and copy assignment
        Name(const Name &other) = delete;
        Name &operator=(const Name &) = delete;

        std::string getName() const;
        void setParent(const Name &parent);
        const Name *getParent() const;
        std::string getFullyQualifiedName() const;

    protected:
        explicit Name(std::string name);

    private:
        const std::string _name;
        const Name *_parent;
    };
}// namespace ir

#endif//AHORN_NAME_H
