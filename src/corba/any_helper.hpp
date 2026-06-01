#pragma once
// Helpers for building and extracting CORBA::Any values that match the wire
// format produced by the Python NxApiLib/any.py module.
//
// The robot API uses two structural patterns:
//   Pattern A (null):   ANY.to_any(None)
//   Pattern B (kv):     ANY.to_any([["key1", v1], ["key2", v2], ...])
//                       → sequence<sequence<any>>
//   Pattern C (single): ANY.to_any([[["key", v]]])
//                       → sequence<sequence<sequence<any>>>
//
// Extraction helpers decode the response Any values back to C++ primitives
// or nlohmann::json for complex / opaque types.

#include <omniORB4/CORBA.h>
#include <omniORB4/dynamic.h>
#include <string>
#include <vector>
#include <utility>
#include <nlohmann/json.hpp>

namespace AnyHelper {

// ---- Primitive Any builders ----
CORBA::Any make_null();
CORBA::Any from_long(CORBA::Long v);
CORBA::Any from_ulong(CORBA::ULong v);
CORBA::Any from_double(CORBA::Double v);
CORBA::Any from_bool(CORBA::Boolean v);
CORBA::Any from_wstring(const std::wstring& s);
CORBA::Any from_string(const std::string& s);       // converted to wstring
CORBA::Any from_long_seq(const std::vector<int>& v);
CORBA::Any from_double_seq(const std::vector<double>& v);

// ---- Compound builders ----
// Pattern B: [["key1",v1],["key2",v2],...] → sequence<sequence<any>>
CORBA::Any make_params(const std::vector<std::pair<std::string, CORBA::Any>>& kvs);

// Pattern C: [[["key",v]]] → sequence<sequence<sequence<any>>>
CORBA::Any make_single_var(const std::string& key, const CORBA::Any& value);

// ---- Extraction helpers ----
CORBA::Long     extract_long(const CORBA::Any& a);
CORBA::Boolean  extract_bool(const CORBA::Any& a);
std::wstring    extract_wstring(const CORBA::Any& a);

// Recursively converts Any → nlohmann::json (handles sequences, primitives, nested structures)
nlohmann::json  to_json(const CORBA::Any& a);

// Unwraps a sequence<any> into a vector of Any elements
std::vector<CORBA::Any> extract_seq(const CORBA::Any& a);

} // namespace AnyHelper
