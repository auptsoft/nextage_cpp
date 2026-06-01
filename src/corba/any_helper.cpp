#include "any_helper.hpp"
#include <omniORB4/CORBA.h>
#include <omniORB4/dynAny.h>
#include <omniORB4/omniORB.h>
#include <stdexcept>

using namespace DynamicAny;
#include <codecvt>
#include <locale>

// Helper to convert std::string → std::wstring
static std::wstring to_wstr(const std::string& s) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.from_bytes(s);
}

namespace AnyHelper {

// ---- Primitive builders ----

CORBA::Any make_null() {
    return CORBA::Any();
}

CORBA::Any from_long(CORBA::Long v) {
    CORBA::Any a;
    a <<= v;
    return a;
}

CORBA::Any from_ulong(CORBA::ULong v) {
    CORBA::Any a;
    a <<= v;
    return a;
}

CORBA::Any from_double(CORBA::Double v) {
    CORBA::Any a;
    a <<= v;
    return a;
}

CORBA::Any from_bool(CORBA::Boolean v) {
    CORBA::Any a;
    a <<= CORBA::Any::from_boolean(v);
    return a;
}

CORBA::Any from_wstring(const std::wstring& s) {
    CORBA::Any a;
    a <<= CORBA::Any::from_wstring(s.c_str(), 0);
    return a;
}

CORBA::Any from_string(const std::string& s) {
    return from_wstring(to_wstr(s));
}

CORBA::Any from_long_seq(const std::vector<int>& v) {
    // Build sequence<long> using DynAny
    int _argc = 0; CORBA::ORB_var orb = CORBA::ORB_init(_argc, nullptr);
    DynAnyFactory_var factory = DynAnyFactory::_narrow(
        orb->resolve_initial_references("DynAnyFactory"));

    // TypeCode for sequence<long>
    CORBA::TypeCode_var tc_long = CORBA::TypeCode::_duplicate(CORBA::_tc_long);
    CORBA::TypeCode_var tc_seq  = orb->create_sequence_tc(0, tc_long);

    DynAny_var dyn = factory->create_dyn_any_from_type_code(tc_seq);
    DynSequence_var dynseq = DynSequence::_narrow(dyn);
    dynseq->set_length(static_cast<CORBA::ULong>(v.size()));

    for (size_t i = 0; i < v.size(); ++i) {
        DynAny_var elem = dynseq->current_component();
        elem->insert_long(static_cast<CORBA::Long>(v[i]));
        if (i + 1 < v.size()) dynseq->next();
    }

    CORBA::Any* result = dyn->to_any();
    CORBA::Any copy = *result;
    delete result;
    dyn->destroy();
    return copy;
}

CORBA::Any from_double_seq(const std::vector<double>& v) {
    int _argc = 0; CORBA::ORB_var orb = CORBA::ORB_init(_argc, nullptr);
    DynAnyFactory_var factory = DynAnyFactory::_narrow(
        orb->resolve_initial_references("DynAnyFactory"));

    CORBA::TypeCode_var tc_dbl = CORBA::TypeCode::_duplicate(CORBA::_tc_double);
    CORBA::TypeCode_var tc_seq = orb->create_sequence_tc(0, tc_dbl);

    DynAny_var dyn = factory->create_dyn_any_from_type_code(tc_seq);
    DynSequence_var dynseq = DynSequence::_narrow(dyn);
    dynseq->set_length(static_cast<CORBA::ULong>(v.size()));

    for (size_t i = 0; i < v.size(); ++i) {
        DynAny_var elem = dynseq->current_component();
        elem->insert_double(v[i]);
        if (i + 1 < v.size()) dynseq->next();
    }

    CORBA::Any* result = dyn->to_any();
    CORBA::Any copy = *result;
    delete result;
    dyn->destroy();
    return copy;
}

// ---- Build a sequence<any> from a vector of Any values ----
static CORBA::Any make_any_seq(const std::vector<CORBA::Any>& elems) {
    int _argc = 0; CORBA::ORB_var orb = CORBA::ORB_init(_argc, nullptr);
    DynAnyFactory_var factory = DynAnyFactory::_narrow(
        orb->resolve_initial_references("DynAnyFactory"));

    CORBA::TypeCode_var tc_any = CORBA::TypeCode::_duplicate(CORBA::_tc_any);
    CORBA::TypeCode_var tc_seq = orb->create_sequence_tc(0, tc_any);

    DynAny_var dyn = factory->create_dyn_any_from_type_code(tc_seq);
    DynSequence_var dynseq = DynSequence::_narrow(dyn);
    dynseq->set_length(static_cast<CORBA::ULong>(elems.size()));

    for (size_t i = 0; i < elems.size(); ++i) {
        DynAny_var elem = dynseq->current_component();
        elem->insert_any(const_cast<CORBA::Any&>(elems[i]));
        if (i + 1 < elems.size()) dynseq->next();
    }

    CORBA::Any* result = dyn->to_any();
    CORBA::Any copy = *result;
    delete result;
    dyn->destroy();
    return copy;
}

// ---- Pattern B: [["key1",v1], ["key2",v2], ...] → sequence<sequence<any>> ----
CORBA::Any make_params(const std::vector<std::pair<std::string, CORBA::Any>>& kvs) {
    // Each kv pair becomes a 2-element sequence<any>: [wstring_any, value_any]
    std::vector<CORBA::Any> outer;
    outer.reserve(kvs.size());

    for (const auto& kv : kvs) {
        std::vector<CORBA::Any> inner;
        inner.push_back(from_string(kv.first));   // key as wstring
        inner.push_back(kv.second);                // value
        outer.push_back(make_any_seq(inner));
    }

    return make_any_seq(outer);
}

// ---- Pattern C: [[["key", v]]] → sequence<sequence<sequence<any>>> ----
CORBA::Any make_single_var(const std::string& key, const CORBA::Any& value) {
    // Innermost: [key, value]
    std::vector<CORBA::Any> inner;
    inner.push_back(from_string(key));
    inner.push_back(value);
    CORBA::Any inner_seq = make_any_seq(inner);

    // Middle: [inner_seq]
    std::vector<CORBA::Any> middle{inner_seq};
    CORBA::Any middle_seq = make_any_seq(middle);

    // Outer: [middle_seq]
    std::vector<CORBA::Any> outer{middle_seq};
    return make_any_seq(outer);
}

// ---- Extraction helpers ----

CORBA::Long extract_long(const CORBA::Any& a) {
    CORBA::Long v = 0;
    if (!(a >>= v)) {
        // Try ULong fallback
        CORBA::ULong uv = 0;
        if (a >>= uv) return static_cast<CORBA::Long>(uv);
    }
    return v;
}

CORBA::Boolean extract_bool(const CORBA::Any& a) {
    CORBA::Boolean v = false;
    CORBA::Any::to_boolean tb(v);
    a >>= tb;
    return v;
}

std::wstring extract_wstring(const CORBA::Any& a) {
    CORBA::WChar* ws = nullptr;
    CORBA::Any::to_wstring tw(ws, 0);
    if (a >>= tw) {
        return std::wstring(ws);
    }
    return L"";
}

std::vector<CORBA::Any> extract_seq(const CORBA::Any& a) {
    int _argc = 0; CORBA::ORB_var orb = CORBA::ORB_init(_argc, nullptr);
    DynAnyFactory_var factory = DynAnyFactory::_narrow(
        orb->resolve_initial_references("DynAnyFactory"));

    DynAny_var dyn = factory->create_dyn_any(a);
    DynSequence_var dynseq = DynSequence::_narrow(dyn);

    std::vector<CORBA::Any> result;
    if (CORBA::is_nil(dynseq)) {
        dyn->destroy();
        return result;
    }

    CORBA::ULong len = dynseq->get_length();
    result.reserve(len);

    for (CORBA::ULong i = 0; i < len; ++i) {
        DynAny_var elem = dynseq->current_component();
        CORBA::Any* ea = elem->to_any();
        result.push_back(*ea);
        delete ea;
        if (i + 1 < len) dynseq->next();
    }

    dyn->destroy();
    return result;
}

// ---- Recursive Any → nlohmann::json ----
nlohmann::json to_json(const CORBA::Any& a) {
    // Try primitives first
    {
        CORBA::Long v;
        if (a >>= v) return nlohmann::json(v);
    }
    {
        CORBA::ULong v;
        if (a >>= v) return nlohmann::json(v);
    }
    {
        CORBA::LongLong v;
        if (a >>= v) return nlohmann::json(v);
    }
    {
        CORBA::Double v;
        if (a >>= v) return nlohmann::json(v);
    }
    {
        CORBA::Boolean v = false;
        CORBA::Any::to_boolean tb(v);
        if (a >>= tb) return nlohmann::json(static_cast<bool>(v));
    }
    {
        CORBA::WChar* ws = nullptr;
        CORBA::Any::to_wstring tw(ws, 0);
        if (a >>= tw) {
            std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
            return nlohmann::json(conv.to_bytes(ws));
        }
    }
    {
        const char* s = nullptr;
        if (a >>= s) return nlohmann::json(std::string(s));
    }

    // Try sequence
    CORBA::TypeCode_var tc = a.type();
    if (tc->kind() == CORBA::tk_sequence || tc->kind() == CORBA::tk_array) {
        auto elems = extract_seq(a);
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& e : elems) {
            arr.push_back(to_json(e));
        }
        return arr;
    }

    // Null / void
    if (tc->kind() == CORBA::tk_null || tc->kind() == CORBA::tk_void) {
        return nlohmann::json(nullptr);
    }

    // Struct — recurse via DynAny
    if (tc->kind() == CORBA::tk_struct) {
        int _argc = 0; CORBA::ORB_var orb = CORBA::ORB_init(_argc, nullptr);
        DynAnyFactory_var factory = DynAnyFactory::_narrow(
            orb->resolve_initial_references("DynAnyFactory"));
        DynAny_var dyn = factory->create_dyn_any(a);
        DynStruct_var ds = DynStruct::_narrow(dyn);

        nlohmann::json obj = nlohmann::json::object();
        CORBA::ULong n = tc->member_count();
        for (CORBA::ULong i = 0; i < n; ++i) {
            const char* name = tc->member_name(i);
            DynAny_var mem = ds->current_component();
            CORBA::Any* ma = mem->to_any();
            obj[name] = to_json(*ma);
            delete ma;
            if (i + 1 < n) ds->next();
        }
        dyn->destroy();
        return obj;
    }

    return nlohmann::json(nullptr);
}

} // namespace AnyHelper
