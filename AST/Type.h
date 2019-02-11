#pragma once
#include "INode.h"
#include "Identifier.h"
#include <memory>
#include <variant>

struct NoneType
{
   bool operator ==(const NoneType&) const {
        return true;
    }
    bool operator !=(const NoneType&) const {
        return false;
    }
};

struct IntType
{
    bool operator ==(const IntType&) const {
        return true;
    }
    bool operator !=(const IntType&) const {
        return false;
    }
};
struct BoolType
{
    bool operator ==(const BoolType&) const {
        return true;
    }
    bool operator !=(const BoolType&) const {
        return false;
    }
};
struct IntArrType
{
    bool operator ==(const IntArrType&) const {
        return true;
    }
    bool operator !=(const IntArrType&) const {
        return false;
    }
};
struct ClassType
{
    bool operator ==(const ClassType& t) const {
        return dynamic_cast<Identifier*>(name)->name == dynamic_cast<Identifier*>(t.name)->name;
    }
    bool operator !=(const ClassType& t) const {
        return dynamic_cast<Identifier*>(name)->name != dynamic_cast<Identifier*>(t.name)->name;
    }
    IIdentifier* name;
};


using TYPE = std::variant<NoneType, IntType, BoolType, IntArrType, ClassType>;

struct Type: public IType
{
    TYPE type;
    template <typename T>
    Type(T&& t) : type(t) {}
    void Accept(Visitor* v) const;
    bool operator ==(const Type& t) const {
        return type == t.type;
    }
    bool operator !=(const Type& t) const {
        return type != t.type;
    }
};
