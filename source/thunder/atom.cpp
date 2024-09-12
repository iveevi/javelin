#include "thunder/atom.hpp"
#include "ire/callable.hpp"
#include "thunder/enumerations.hpp"

namespace jvl::thunder {

// TODO: top level formatter...

// Qualifier
bool Qualifier::operator==(const Qualifier &other) const
{
        return (underlying == other.underlying)
                && (numerical  == other.numerical)
                && (kind == other.kind);
}

Addresses Qualifier::addresses()
{
        return { underlying, Addresses::null() };
}

std::string Qualifier::to_string() const
{
        return fmt::format("{:10} {}(%{})[{}]",
                "qualifier",
                tbl_qualifier_kind[kind],
                underlying,
                numerical);
}

// TypeInformation
bool TypeInformation::operator==(const TypeInformation &other) const
{
        return (down == other.down)
                && (next == other.next)
                && (item == other.item);
}

Addresses TypeInformation::addresses()
{
        return { down, next };
}

std::string TypeInformation::to_string() const
{
        std::string result;

        result += fmt::format("{:10} item: ", "type");
        if (item != bad)
                result += fmt::format("{}", tbl_primitive_types[item]);
        else if (down != -1)
                result += fmt::format("%{}", down);
        else
                result += fmt::format("<?>");

        if (next != -1)
                result += fmt::format(" next: %{}", next);

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
        result = fmt::format("{:10} value: ", "primitive");

        switch (type) {
        case i32:
                result += fmt::format("{}", idata);
                break;
        case u32:
                result += fmt::format("{}", udata);
                break;
        case f32:
                result += fmt::format("{:.6f}", fdata);
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
        return fmt::format("{:10} src: %{} component: #{}",
                "swizzle", src, tbl_swizzle_code[code]);
}

// Operation
bool Operation::operator==(const Operation &other) const
{
        return (a == other.a)
                && (b == other.b)
                && (code == other.code);
}

Addresses Operation::addresses()
{
        return { a, b };
}
        
std::string Operation::to_string() const
{
        return fmt::format("{:10} $({}) a: %{} b: %{}",
                "operation",
                tbl_operation_code[code],
                a, b);
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
        return fmt::format("{:10} $({}) args: %{}",
                "intrinsic", tbl_intrinsic_operation[opn], args);
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

        result += fmt::format("{:10} item: %{} next: ", "list", item);
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
                && (args == other.args)
                && (transient == other.transient);
}

Addresses Construct::addresses()
{
        return { type, args };
}

std::string Construct::to_string() const
{
        std::string result;

        result += fmt::format("{:10} type: %{} ", "construct", type);

        if (transient)
		return result + "[transient]";

	result += "args: ";
        if (args == -1)
		result += "(nil)";
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
        return fmt::format("{:10} src: %{} dst: %{} bss: {}", "store", src, dst, bss);
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
        return fmt::format("{:10} src: %{} field: #{}", "load", src, idx);
}

// Load
bool ArrayAccess::operator==(const ArrayAccess &other) const
{
        return (src == other.src)
                && (loc == other.loc);
}

Addresses ArrayAccess::addresses()
{
        return { src, loc };
}

std::string ArrayAccess::to_string() const
{
        return fmt::format("{:10} src: %{} loc: %{}", "access", src, loc);
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
        if (kind == control_flow_end)
                return "end";

        if (cond >= 0)
                return fmt::format("br: %{} ({}) -> %{}", cond, tbl_branch_kind[kind], failto);

        return fmt::format("br: (nil) ({}) -> %{}", tbl_branch_kind[kind], failto);
}

// Returns
bool Returns::operator==(const Returns &other) const
{
        return (value == other.value)
                && (type == other.type);
}

Addresses Returns::addresses()
{
        return { value, type };
}

std::string Returns::to_string() const
{
        return fmt::format("{:10} value: %{}", "return", value);
}

} // namespace jvl::thunder