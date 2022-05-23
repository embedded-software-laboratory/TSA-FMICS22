#include "ir/instruction/instruction.h"

using namespace ir;

Instruction::Instruction(Kind kind) : _kind(kind) {}

Instruction::Kind Instruction::getKind() const {
    return _kind;
}
