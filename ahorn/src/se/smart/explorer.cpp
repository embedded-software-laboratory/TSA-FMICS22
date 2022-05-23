#include "se/smart/explorer.h"

using namespace se::smart;

Explorer::Explorer() : _contexts(std::vector<std::unique_ptr<Context>>()) {}

bool Explorer::isEmpty() const {
    return _contexts.empty();
}

std::unique_ptr<Context> Explorer::popContext() {
    std::unique_ptr<Context> context = std::move(_contexts.back());
    _contexts.pop_back();
    return context;
}

void Explorer::pushContext(std::unique_ptr<Context> context) {
    _contexts.push_back(std::move(context));
}