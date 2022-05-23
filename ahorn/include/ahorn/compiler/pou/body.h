#ifndef AHORN_BODY_H
#define AHORN_BODY_H

#include <gtest/gtest_prod.h>

#include "compiler/statement/statement.h"
#include <boost/iterator/indirect_iterator.hpp>

#include <memory>
#include <vector>

class TestLibCompiler_Compiler_Test;

class Body {
private:
    FRIEND_TEST(::TestLibCompiler, Compiler);

private:
    using statements_t = std::vector<std::unique_ptr<Statement>>;
    using const_statements_it = boost::indirect_iterator<statements_t::const_iterator>;

public:
    explicit Body(statements_t statement_list);
    const_statements_it statementListBegin() const;
    const_statements_it statementListEnd() const;

    friend std::ostream &operator<<(std::ostream &os, Body const &body) {
        return body.print(os);
    }
    std::ostream &print(std::ostream &os) const;

private:
    statements_t _statement_list;
};

#endif//AHORN_BODY_H
