#include "thunder/atom.hpp"
#include "ire/callable.hpp"

namespace jvl::thunder {

// Global
bool Global::operator==(const Global &other) const
{
        return (type == other.type)
                && (binding == other.binding)
                && (qualifier == other.qualifier);
}

Addresses Global::addresses()
{
        return { type, Addresses::null() };
}

std::string Global::to_string() const
{
        return fmt::format("global: %{} = ({}, {})",
                type,
                tbl_global_qualifier[qualifier],
                binding);
}

// TypeField
bool TypeField::operator==(const TypeField &other) const
{
        return (down == other.down)
                && (next == other.next)
                && (item == other.item);
}

Addresses TypeField::addresses()
{
        return { down, next };
}

std::string TypeField::to_string() const
{
        std::string result;

        result += fmt::format("type: ");
        if (item != bad)
                result += fmt::format("{}", tbl_primitive_types[item]);
        else if (down != -1)
                result += fmt::format("%{}", down);
        else
                result += fmt::format("<BAD>");

        result += fmt::format(" -> ");
        if (next >= 0)
                result += fmt::format("%{}", next);
        else
                result += fmt::format("(nil)");

        return result;
}

// Primitive
bool Primitive::operator==(const Primitive &other) const
{
        if (type != other.type)
                return false;

        switch (type) {
        case i32: return idata == other.idata;
        case f32: return fdata == other.fdata;
        default:
                break;
        }

        return false;
}

Addresses Primitive::addresses()
{
        return { Addresses::null(), Addresses::null() };
}

std::string Primitive::to_string() const
{
        std::string result;
        result = fmt::format("primitive: {} = ", tbl_primitive_types[type]);

        switch (type) {
        case i32:
                result += fmt::format("{}", idata);
                break;
        case u32:
                result += fmt::format("{}", udata);
                break;
        case f32:
                result += fmt::format("{:.2f}", fdata);
                break;
        default:
                result += fmt::format("?");
                break;
        }

        return result;
}

// Swizzle
bool Swizzle::operator==(const Swizzle &other) const
{
        return (src == other.src)
                && (code == other.code);
}

Addresses Swizzle::addresses()
{
        return { src, Addresses::null() };
}

std::string Swizzle::to_string() const
{
        return fmt::format("swizzle %{} #{}", src, tbl_swizzle_code[code]);
}

// Operation
bool Operation::operator==(const Operation &other) const
{
        return (args == other.args)
                && (type == other.type)
                && (code == other.code);
}

Addresses Operation::addresses()
{
        return { args, type };
}
        
std::string Operation::to_string() const
{
        return fmt::format("op $({}) %{} -> %{}",
                tbl_operation_code[code],
                args, type);
}

// Intrinsic
bool Intrinsic::operator==(const Intrinsic &other) const
{
        return (args == other.args)
                && (type == other.type)
                && (opn == other.opn);
}

Addresses Intrinsic::addresses()
{
        return { args, type };
}
        
std::string Intrinsic::to_string() const
{
        return fmt::format("intr ${} %{} -> %{}",
                tbl_intrinsic_operation[opn],
                args, type);
}

// List
bool List::operator==(const List &other) const
{
        return (item == other.item)
                && (next == other.next);
}

Addresses List::addresses()
{
        return { item, next };
}

std::string List::to_string() const
{
        std::string result;

        result += fmt::format("list: %{} -> ", item);
        if (next >= 0)
                result += fmt::format("%{}", next);
        else
                result += fmt::format("(nil)");

        return result;
}

// Construct
bool Construct::operator==(const Construct &other) const
{
        return (type == other.type)
                && (args == other.args);
}

Addresses Construct::addresses()
{
        return { type, args };
}

std::string Construct::to_string() const
{
        std::string result;

        result += fmt::format("construct: %{} = ", type);
        if (args == -1)
                result += fmt::format("(nil)");
        else
                result += fmt::format("%{}", args);

        return result;
}

// Call
bool Call::operator==(const Call &other) const
{
        return (type == other.type)
                && (args == other.args)
                && (cid == other.cid);
}

Addresses Call::addresses()
{
        return { args, type };
}

std::string Call::to_string() const
{
        std::string result;

        ire::Callable *cbl = ire::Callable::search_tracked(cid);
        result += fmt::format("call ${}:", cbl->name);
        if (args == -1)
                result += fmt::format(" (nil) -> ");
        else
                result += fmt::format(" %{} -> ", args);
        result += fmt::format("%{}", type);

        return result;
}

// Store
bool Store::operator==(const Store &other) const
{
        return (dst == other.dst)
                && (src == other.src);
}

Addresses Store::addresses()
{
        return { dst, src };
}
        
std::string Store::to_string() const
{
        return fmt::format("store %{} -> %{}", src, dst);
}

// Load
bool Load::operator==(const Load &other) const
{
        return (src == other.src)
                && (idx == other.idx);
}

Addresses Load::addresses()
{
        return { src, Addresses::null() };
}

std::string Load::to_string() const
{
        return fmt::format("load %{} #{}", src, idx);
}

// Branch
bool Branch::operator==(const Branch &other) const
{
        return (cond == other.cond)
                && (failto == other.failto);
}

Addresses Branch::addresses()
{
        return { cond, failto };
}
        
std::string Branch::to_string() const
{
        if (cond >= 0)
                return fmt::format("br %{} -> %{}", cond, failto);

        return fmt::format("br (nil) -> %{}", failto);
}

// While
bool While::operator==(const While &other) const
{
        return (cond == other.cond)
                && (failto == other.failto);
}

Addresses While::addresses()
{
        return { cond, failto };
}
        
std::string While::to_string() const
{
        return fmt::format("while %{} -> %{}", cond, failto);
}

// Returns
bool Returns::operator==(const Returns &other) const
{
        return (args == other.args)
                && (type == other.type);
}

Addresses Returns::addresses()
{
        return { args, type };
}

std::string Returns::to_string() const
{
        return fmt::format("return %{} -> %{}", args, type);
}

// End
bool End::operator==(const End &) const
{
        return true;
}

Addresses End::addresses()
{
        return { Addresses::null(), Addresses::null() };
}

std::string End::to_string() const
{
        return "end";
}

} // namespace jvl::thunder