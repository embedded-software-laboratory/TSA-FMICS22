#include "compiler/pou/body.h"
#include <sstream>

Body::Body(statements_t statement_list) : _statement_list(std::move(statement_list)) {}

Body::const_statements_it Body::statementListBegin() const {
    return boost::make_indirect_iterator(_statement_list.begin());
}

Body::const_statements_it Body::statementListEnd() const {
    return boost::make_indirect_iterator(_statement_list.end());
}

std::ostream &Body::print(std::ostream &os) const {
    std::stringstream str;
    for (auto it = statementListBegin(); it != statementListEnd(); ++it) {
        str << *it;
    }
    return os << str.str();
}
