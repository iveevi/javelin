#include "thunder/atom.hpp"
#include "thunder/enumerations.hpp"
#include "thunder/tracked_buffer.hpp"

namespace jvl::thunder {

auto header(auto name, auto _)
{
        return fmt::format(fmt::emphasis::bold, "{:15}", name);
}

auto fmtaddr(Index i)
{
        if (i < 0)
                return fmt::format("-");

        return fmt::format("%{}", i);
}

auto fmtidx(Index i)
{
        if (i < 0)
                return fmt::format("-");

        return fmt::format("@{}", i);
}

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

std::string Qualifier::to_assembly_string() const
{
        return fmt::format("qualifier {} {} {}",
                fmtaddr(underlying),
                fmtidx(numerical),
                tbl_qualifier_kind[kind]);
}

std::string Qualifier::to_pretty_string() const
{
        return header("QUALIFIER", fmt::color::orange)
                + fmt::format("\n          :: kind: {}", tbl_qualifier_kind[kind])
                + fmt::format("\n          :: numerical: {}", numerical)
                + fmt::format("\n          :: underlying: %{}", underlying);
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

std::string TypeInformation::to_assembly_string() const
{
        return fmt::format("type {} {} {}",
                fmtaddr(down),
                fmtaddr(next),
                tbl_primitive_types[item]);
}

std::string TypeInformation::to_pretty_string() const
{
        std::string result;

        result += header("TYPE", fmt::color::antique_white);

        if (item != bad)
                result += fmt::format("\n          :: item: {}", tbl_primitive_types[item]);
        else if (down != -1)
                result += fmt::format("\n          :: nested: %{}", down);
        else
                result += fmt::format("\n          :: BAD");

        if (next != -1)
                result += fmt::format("\n          :: next: %{}", next);
        
        return result;
}

// Primitive
bool Primitive::operator==(const Primitive &other) const
{
        if (type != other.type)
                return false;

        switch (type) {
        case boolean: return bdata == other.bdata;
        case i32: return idata == other.idata;
        case u32: return udata == other.udata;
        case f32: return fdata == other.fdata;
        case u64: return udata == other.udata;
        default:
                break;
        }

        return false;
}

Addresses Primitive::addresses()
{
        return { Addresses::null(), Addresses::null() };
}

std::string Primitive::to_assembly_string() const
{
        switch (type) {
        case boolean:
                return fmt::format("bool {}", bdata);
        case i32:
                return fmt::format("i32 {}", idata);
        case u32:
                return fmt::format("u32 {}", udata);
        case u64:
                return fmt::format("u64 {}", udata);
        case f32:
                return fmt::format("f32 {}", fdata);
        default:
                break;
        }

        return "UNKNOWN";
}

std::string Primitive::to_pretty_string() const
{
        std::string result;
        
        result += header("PRIMITIVE", fmt::color::purple);
        
        switch (type) {
        case boolean:
                result += fmt::format("\n          :: bool: {}", bdata);
                break;
        case i32:
                result += fmt::format("\n          :: i32: {}", idata);
                break;
        case u32:
                result += fmt::format("\n          :: u32: {}", udata);
                break;
        case u64:
                result += fmt::format("\n          :: u64: {}", udata);
                break;
        case f32:
                result += fmt::format("\n          :: f32: {}", fdata);
                break;
        default:
                result += fmt::format("\n          :: UNKNOWN");
                break;
        }

        return result;
}

std::string Primitive::value_string() const
{
	switch (type) {
	case boolean:
		return fmt::format("{}", bdata);
	case i32:
		return fmt::format("{}", idata);
	case u32:
		return fmt::format("{}", udata);
	case u64:
		// Full data unavailable...
		return fmt::format("{}", udata);
	case f32:
		return fmt::format("{}", fdata);
	default:
		break;
	}

        return "?";
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

std::string Swizzle::to_assembly_string() const
{
        return fmt::format(".{} {}", tbl_swizzle_code[code], fmtaddr(src));
}

std::string Swizzle::to_pretty_string() const
{
        return header("SWIZZLE", fmt::color::azure)
                + fmt::format("\n          :: src: %{}", src)
                + fmt::format("\n          :: swizzle: {}", tbl_swizzle_code[code]);
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

std::string Operation::to_assembly_string() const
{
        return fmt::format(".{} {} {}",
                tbl_operation_code[code],
                fmtaddr(a),
                fmtaddr(b));
}
        
std::string Operation::to_pretty_string() const
{
        return header("OPERATION", fmt::color::indigo)
                + fmt::format("\n          :: code: {}", tbl_operation_code[code])
                + fmt::format("\n          :: first: %{}", a)
                + fmt::format("\n          :: second: %{}", b);
}

// Intrinsic
bool Intrinsic::operator==(const Intrinsic &other) const
{
        return (args == other.args) && (opn == other.opn);
}

Addresses Intrinsic::addresses()
{
        return { args, Addresses::null() };
}

std::string Intrinsic::to_assembly_string() const
{
        return fmt::format(".{} {}",
                tbl_intrinsic_operation[opn],
                fmtaddr(args));
}

std::string Intrinsic::to_pretty_string() const
{
        return header("INTRINSIC", fmt::color::yellow)
                + fmt::format("\n          :: code: {}", tbl_intrinsic_operation[opn])
                + fmt::format("\n          :: args: %{}", args);
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

std::string List::to_assembly_string() const
{
        return fmt::format("list {} {}",
                fmtaddr(item),
                fmtaddr(next));
}

std::string List::to_pretty_string() const
{
        std::string result;

        result += header("LIST", fmt::color::brown);
        result += fmt::format("\n          :: item: %{}", item);
        if (next >= 0)
                result += fmt::format("\n          :: next: %{}", next);

        return result;
}

// Construct
bool Construct::operator==(const Construct &other) const
{
        return (type == other.type)
                && (args == other.args)
                && (mode == other.mode);
}

Addresses Construct::addresses()
{
        return { type, args };
}

std::string Construct::to_assembly_string() const
{
        return fmt::format("construct {} {} {}",
                fmtaddr(type),
                fmtaddr(args),
                tbl_constructor_mode[mode]);
}

std::string Construct::to_pretty_string() const
{
        std::string result;
        
        result += header("CONSTRUCT", fmt::color::olive);
        result += fmt::format("\n          :: mode: {}", tbl_constructor_mode[mode]);
        result += fmt::format("\n          :: type: %{}", type);

        if (args == -1)
                result += fmt::format("\n          :: args: (nil)");
        else
                result += fmt::format("\n          :: args: %{}", args);

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

std::string Call::to_assembly_string() const
{
        auto &buffer = TrackedBuffer::cache_load(cid);

        return fmt::format("call ${} {} {}",
                buffer.name,
                fmtaddr(args),
                fmtaddr(type));
}

std::string Call::to_pretty_string() const
{
        std::string result;

        auto &buffer = TrackedBuffer::cache_load(cid);
       
        result += header("CALL", fmt::color::dark_magenta);
        result += fmt::format("\n          :: callable: ${}", buffer.name);
        if (args != -1)
                result += fmt::format("\n          :: args: %{}", args);

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

std::string Store::to_assembly_string() const
{
        return fmt::format("store {} {}",
                fmtaddr(src),
                fmtaddr(dst));
}
        
std::string Store::to_pretty_string() const
{
        return header("STORE", fmt::color::blue)
                + fmt::format("\n          :: src: %{}", src)
                + fmt::format("\n          :: dst: %{}", dst);
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

std::string Load::to_assembly_string() const
{
        return fmt::format("load {} {}",
                fmtaddr(src),
                fmtidx(idx));
}

std::string Load::to_pretty_string() const
{
        return header("LOAD", fmt::color::green)
                + fmt::format("\n          :: src: %{}", src)
                + fmt::format("\n          :: field: %{}", idx);
}

// Array access
bool ArrayAccess::operator==(const ArrayAccess &other) const
{
        return (src == other.src)
                && (loc == other.loc);
}

Addresses ArrayAccess::addresses()
{
        return { src, loc };
}

std::string ArrayAccess::to_assembly_string() const
{
        return fmt::format("index {} {}",
                fmtaddr(src),
                fmtaddr(loc));
}

std::string ArrayAccess::to_pretty_string() const
{
        return header("ACCESS", fmt::color::red)
                + fmt::format("\n          :: src: %{}", src)
                + fmt::format("\n          :: loc: %{}", loc);
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

std::string Branch::to_assembly_string() const
{
        if (kind == control_flow_end)
                return "end";

        if (cond >= 0) {
                return fmt::format("branch {} {} {}",
                        tbl_branch_kind[kind],
                        fmtaddr(cond),
                        fmtaddr(failto));
        }

        return fmt::format("branch {} {}",
                tbl_branch_kind[kind],
                fmtaddr(failto));
}
        
std::string Branch::to_pretty_string() const
{
        std::string hstr = header("BRANCH", fmt::color::cyan);
        
        if (kind == control_flow_end)
                return hstr + fmt::format("\n          :: END");

        if (cond  >= 0) {
                return hstr
                        + fmt::format("\n          :: kind: %{}", tbl_branch_kind[kind])
                        + fmt::format("\n          :: cond: %{}", cond)
                        + fmt::format("\n          :: fails: %{}", failto);
        }

        return hstr
                + fmt::format("\n          :: kind: %{}", tbl_branch_kind[kind])
                + fmt::format("\n          :: fails: %{}", failto);
}

// Returns
bool Returns::operator==(const Returns &other) const
{
        return value == other.value;
}

Addresses Returns::addresses()
{
        return { value, Addresses::null() };
}

std::string Returns::to_assembly_string() const
{
        return fmt::format("return {}", fmtaddr(value));
}

std::string Returns::to_pretty_string() const
{
        return header("RETURN", fmt::color::black)
                + fmt::format("\n          :: value: %{}", value);
}

// Top level atom
Addresses Atom::addresses()
{
        auto ftn = [](auto &x) -> Addresses { return x.addresses(); };
        return std::visit(ftn, *this);
}

void Atom::reindex(const jvl::reindex <Index> &reindexer)
{
        auto &&addrs = addresses();
        if (addrs.a0 != -1) reindexer(addrs.a0);
        if (addrs.a1 != -1) reindexer(addrs.a1);
}

std::string Atom::to_assembly_string() const
{
        auto ftn = [](const auto &x) -> std::string { return x.to_assembly_string(); };
        return std::visit(ftn, *this);
}

std::string Atom::to_pretty_string() const
{
        auto ftn = [](const auto &x) -> std::string { return x.to_pretty_string(); };
        return std::visit(ftn, *this);
}

std::string format_as(const Atom &atom)
{
	return atom.to_pretty_string();
}

} // namespace jvl::thunder