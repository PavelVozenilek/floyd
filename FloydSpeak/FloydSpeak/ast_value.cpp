//
//  parser_value.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "ast_value.h"

#include "statement.h"
#include "parser_primitives.h"

using std::string;
using std::make_shared;


namespace floyd {





	//////////////////////////////////////////////////		struct_instance_t


	bool struct_instance_t::check_invariant() const{
		QUARK_ASSERT(_def.check_invariant());

		for(const auto m: _member_values){
			QUARK_ASSERT(m.check_invariant());
		}
		return true;
	}

	bool struct_instance_t::operator==(const struct_instance_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return _def == other._def && _member_values == other._member_values;
	}


	std::string to_compact_string(const struct_instance_t& v){
		std::vector<std::string> members;
		for(int i = 0 ; i < v._def._members.size() ; i++){
			const auto& def = v._def._members[i];
			const auto& value = v._member_values[i];

			const auto m = /*typeid_to_compact_string(def._type) + " " +*/ def._name + "=" + value.to_compact_string_quote_strings();
			members.push_back(m);
		}
		return "struct {" + concat_strings_with_divider(members, ",") + "}";
	}

	ast_json_t to_normalized_json(const struct_instance_t& t){
		return ast_json_t{json_t::make_object(
			{
				{ "struct-def", struct_definition_to_normalized_json(t._def)._value },
				{ "member_values", values_to_json_array(t._member_values) }
			}
		)};
	}


	//////////////////////////////////////////////////		vector_instance_t


		bool vector_instance_t::check_invariant() const{
			for(const auto m: _elements){
				QUARK_ASSERT(m.check_invariant());
				QUARK_ASSERT(m.get_type() == _element_type);
			}
			return true;
		}

		bool vector_instance_t::operator==(const vector_instance_t& other) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			return _element_type == other._element_type && _elements == other._elements;
		}


		std::string to_compact_string(const vector_instance_t& instance){
			std::vector<std::string> elements;
			for(const auto e: instance._elements){
				const auto es = e.to_compact_string_quote_strings();
				elements.push_back(es);
			}
			return "[" + typeid_to_compact_string(instance._element_type) + "]" + "(" + concat_strings_with_divider(elements, ",") + ")";
		}


//////////////////////////////////////////////////		dict_instance_t


bool dict_instance_t::check_invariant() const{
	for(const auto m: _elements){
		QUARK_ASSERT(m.second.check_invariant());
//		QUARK_ASSERT(m.second.get_type() == _value_type);
	}
	return true;
}

bool dict_instance_t::operator==(const dict_instance_t& other) const{
	QUARK_ASSERT(check_invariant());
	QUARK_ASSERT(other.check_invariant());

	return _value_type == other._value_type && _elements == other._elements;
}


std::string to_compact_string(const dict_instance_t& instance){
	std::vector<std::string> elements;
	for(const auto e: instance._elements){
		const auto key_str = quote(e.first);
		const auto value_str = e.second.to_compact_string_quote_strings();
		const auto es = key_str + ": " + value_str;
		elements.push_back(es);
	}
	return "[string:" + typeid_to_compact_string(instance._value_type) + "]" + "{" + concat_strings_with_divider(elements, ",") + "}";
}





	//////////////////////////////////////		vector_def_t



	bool vector_def_t::check_invariant() const{
		QUARK_ASSERT(!_element_type.is_null() && _element_type.check_invariant());
		return true;
	}

	bool vector_def_t::operator==(const vector_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(!(_element_type == other._element_type)){
			return false;
		}
		return true;
	}





	//////////////////////////////////////		dict_def_t



	bool dict_def_t::check_invariant() const{
		QUARK_ASSERT(_value_type.check_invariant());
		QUARK_ASSERT(_value_type.is_null() == false);
		return true;
	}

	bool dict_def_t::operator==(const dict_def_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		if(!(_value_type == other._value_type)){
			return false;
		}
		return true;
	}







	//////////////////////////////////////////////////		function_instance_t



	bool function_instance_t::check_invariant() const {
		return true;
	}

	bool function_instance_t::operator==(const function_instance_t& other) const{
		QUARK_ASSERT(check_invariant());
		QUARK_ASSERT(other.check_invariant());

		return _def == other._def;
	}








int limit(int value, int min, int max){
	if(value < min){
		return min;
	}
	else if(value > max){
		return max;
	}
	else{
		return value;
	}
}

int compare_string(const std::string& left, const std::string& right){
	// ### Better if it doesn't use c_ptr since that is non-pure string handling.
	return limit(std::strcmp(left.c_str(), right.c_str()), -1, 1);
}

QUARK_UNIT_TESTQ("compare_string()", ""){
	QUARK_TEST_VERIFY(compare_string("", "") == 0);
}
QUARK_UNIT_TESTQ("compare_string()", ""){
	QUARK_TEST_VERIFY(compare_string("aaa", "aaa") == 0);
}
QUARK_UNIT_TESTQ("compare_string()", ""){
	QUARK_TEST_VERIFY(compare_string("b", "a") == 1);
}



int value_t::compare_struct_true_deep(const struct_instance_t& left, const struct_instance_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());

	auto a_it = left._member_values.begin();
	auto b_it = right._member_values.begin();

	while(a_it !=left._member_values.end()){
		int diff = compare_value_true_deep(*a_it, *b_it);
		if(diff != 0){
			return diff;
		}

		a_it++;
		b_it++;
	}
	return 0;
}


//	Compare vector element by element.
//	### Think more of equality when vectors have different size and shared elements are equal.
int compare_vector_true_deep(const vector_instance_t& left, const vector_instance_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());
	QUARK_ASSERT(left._element_type == right._element_type);

	const auto shared_count = std::min(left._elements.size(), right._elements.size());
	for(int i = 0 ; i < shared_count ; i++){
		const auto element_result = value_t::compare_value_true_deep(left._elements[i], right._elements[i]);
		if(element_result != 0){
			return element_result;
		}
	}
	if(left._elements.size() == right._elements.size()){
		return 0;
	}
	else if(left._elements.size() > right._elements.size()){
		return -1;
	}
	else{
		return +1;
	}
}

template <typename Map>
bool map_compare (Map const &lhs, Map const &rhs) {
    // No predicate needed because there is operator== for pairs already.
    return lhs.size() == rhs.size()
        && std::equal(lhs.begin(), lhs.end(),
                      rhs.begin());
}




int compare_dict_true_deep(const dict_instance_t& left, const dict_instance_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());
	QUARK_ASSERT(left._value_type == right._value_type);

	
	auto left_it = left._elements.begin();
	auto left_end_it = left._elements.end();

	auto right_it = right._elements.begin();
	auto right_end_it = right._elements.end();

	while(left_it != left_end_it && right_it != right_end_it && *left_it == *right_it){
		left_it++;
		right_it++;
	}

	if(left_it == left_end_it && right_it == right_end_it){
		return 0;
	}
	else if(left_it == left_end_it && right_it != right_end_it){
		return 1;
	}
	else if(left_it != left_end_it && right_it == right_end_it){
		return -1;
	}
	else if(left_it != left_end_it && right_it != right_end_it){
		int key_diff = compare_string(left_it->first, right_it->first);
		if(key_diff != 0){
			return key_diff;
		}
		else {
			return value_t::compare_value_true_deep(left_it->second, right_it->second);
		}
	}
	else{
		QUARK_ASSERT(false)
	}
}



int compare_json_values(const json_t& lhs, const json_t& rhs){
	if(lhs == rhs){
		return 0;
	}
	else{
		// ??? implement compare.
		assert(false);
	}
}

int value_t::compare_value_true_deep(const value_t& left, const value_t& right){
	QUARK_ASSERT(left.check_invariant());
	QUARK_ASSERT(right.check_invariant());
	QUARK_ASSERT(left.get_type() == right.get_type());

	const auto type = left._typeid;
	if(type.is_null()){
		return 0;
	}
	else if(type.is_bool()){
		return (left.get_bool_value() ? 1 : 0) - (right.get_bool_value() ? 1 : 0);
	}
	else if(type.is_int()){
		return limit(left.get_int_value() - right.get_int_value(), -1, 1);
	}
	else if(type.is_float()){
		const auto a = left.get_float_value();
		const auto b = right.get_float_value();
		if(a > b){
			return 1;
		}
		else if(a < b){
			return -1;
		}
		else{
			return 0;
		}
	}
	else if(type.is_string()){
		return compare_string(left.get_string_value(), right.get_string_value());
	}
	else if(type.is_json_value()){
		return compare_json_values(left.get_json_value(), right.get_json_value());
	}
	else if(type.is_typeid()){
		QUARK_ASSERT(false);
		return 0;
	}
	else if(type.is_struct()){
		//	Make sure the EXACT struct types are the same -- not only that they are both structs
		if(left.get_type() != right.get_type()){
			throw std::runtime_error("Cannot compare structs of different type.");
		}
		else{
			//	Shortcut: same obejct == we know values are same without having to check them.
			if(left.get_struct_value() == right.get_struct_value()){
				return 0;
			}
			else{
				return compare_struct_true_deep(*left.get_struct_value(), *right.get_struct_value());
			}
		}
	}
	else if(type.is_vector()){
		//	Make sure the EXACT types are the same -- not only that they are both vectors.
		if(left.get_type() != right.get_type()){
			throw std::runtime_error("Cannot compare structs of different type.");
		}
		else{
			const auto left_vec = left.get_vector_value();
			const auto right_vec = right.get_vector_value();
			return compare_vector_true_deep(*left_vec, *right_vec);
		}
	}
	else if(type.is_dict()){
		//	Make sure the EXACT types are the same -- not only that they are both dicts.
		if(left.get_type() != right.get_type()){
			throw std::runtime_error("Cannot compare dicts of different type.");
		}
		else{
			const auto left2 = left.get_dict_value();
			const auto right2 = right.get_dict_value();
			return compare_dict_true_deep(*left2, *right2);
		}
	}
	else if(type.is_function()){
		QUARK_ASSERT(false);
		return 0;
	}
	else if(type.is_unresolved_type_identifier()){
		QUARK_ASSERT(false);
		return 0;
	}
	else{
		QUARK_ASSERT(false);
	}
}





	//////////////////////////////////////////////////		value_t


//??? swap(), operator=, copy-constructor.

QUARK_UNIT_TESTQ("value_t()", "null"){
	const auto a = value_t();
	QUARK_TEST_VERIFY(a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_dict());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t());
	QUARK_TEST_VERIFY(a != value_t("test"));
	QUARK_TEST_VERIFY(a.to_compact_string() == "<null>");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "<null>");
}

QUARK_UNIT_TESTQ("value_t()", "bool - true"){
	const auto a = value_t(true);
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_dict());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t(true));
	QUARK_TEST_VERIFY(a != value_t(false));
	QUARK_TEST_VERIFY(a.to_compact_string() == keyword_t::k_true);
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "bool: true");
}

QUARK_UNIT_TESTQ("value_t()", "bool - false"){
	const auto a = value_t(false);
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_dict());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t(false));
	QUARK_TEST_VERIFY(a != value_t(true));
	QUARK_TEST_VERIFY(a.to_compact_string() == keyword_t::k_false);
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "bool: false");
}

QUARK_UNIT_TESTQ("value_t()", "int"){
	const auto a = value_t(13);
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_dict());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t(13));
	QUARK_TEST_VERIFY(a != value_t(14));
	QUARK_TEST_VERIFY(a.to_compact_string() == "13");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "int: 13");
}

QUARK_UNIT_TESTQ("value_t()", "float"){
	const auto a = value_t(13.5f);
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(a.is_float());
	QUARK_TEST_VERIFY(!a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_dict());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t(13.5f));
	QUARK_TEST_VERIFY(a != value_t(14.0f));
	QUARK_TEST_VERIFY(a.to_compact_string() == "13.5");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "float: 13.5");
}

QUARK_UNIT_TESTQ("value_t()", "string"){
	const auto a = value_t("xyz");
	QUARK_TEST_VERIFY(!a.is_null());
	QUARK_TEST_VERIFY(!a.is_bool());
	QUARK_TEST_VERIFY(!a.is_int());
	QUARK_TEST_VERIFY(!a.is_float());
	QUARK_TEST_VERIFY(a.is_string());
	QUARK_TEST_VERIFY(!a.is_struct());
	QUARK_TEST_VERIFY(!a.is_vector());
	QUARK_TEST_VERIFY(!a.is_dict());
	QUARK_TEST_VERIFY(!a.is_function());

	QUARK_TEST_VERIFY(a == value_t("xyz"));
	QUARK_TEST_VERIFY(a != value_t("xyza"));
	QUARK_TEST_VERIFY(a.to_compact_string() == "xyz");
	QUARK_TEST_VERIFY(a.value_and_type_to_string() == "string: \"xyz\"");
}


ast_json_t value_and_type_to_normalized_json(const value_t& v){
	return ast_json_t{json_t::make_array({
		typeid_to_normalized_json(v.get_type())._value,
		value_to_normalized_json(v)._value
	})};
}

ast_json_t value_to_normalized_json(const value_t& v){
	if(v.is_null()){
		return ast_json_t{json_t()};
	}
	else if(v.is_bool()){
		return ast_json_t{json_t(v.get_bool_value())};
	}
	else if(v.is_int()){
		return ast_json_t{json_t(static_cast<double>(v.get_int_value()))};
	}
	else if(v.is_float()){
		return ast_json_t{json_t(static_cast<double>(v.get_float_value()))};
	}
	else if(v.is_string()){
		return ast_json_t{json_t(v.get_string_value())};
	}
	else if(v.is_json_value()){
		return ast_json_t{v.get_json_value()};
	}
	else if(v.is_typeid()){
		return typeid_to_normalized_json(v.get_typeid_value());
	}
	else if(v.is_struct()){
		const auto value = v.get_struct_value();
		return ast_json_t{to_normalized_json(*value)};
	}
	else if(v.is_vector()){
		const auto value = v.get_vector_value();
		std::vector<json_t> result;
		for(int i = 0 ; i < value->_elements.size() ; i++){
			const auto element_value = value->_elements[i];
			result.push_back(value_to_normalized_json(element_value)._value);
		}
		return ast_json_t{result};
	}
	else if(v.is_dict()){
		const auto value = v.get_dict_value();
		std::map<string, json_t> result;
		for(const auto e: value->_elements){
			result[e.first] = value_to_normalized_json(e.second)._value;
		}
		return ast_json_t{result};
	}
	else if(v.is_function()){
		const auto value = v.get_function_value();
		return ast_json_t{json_t::make_object(
			{
				{ "function_type", typeid_to_normalized_json(get_function_type(value->_def))._value }
			}
		)};
	}
	else{
		QUARK_ASSERT(false);
	}
}

QUARK_UNIT_TESTQ("value_to_normalized_json()", ""){
	quark::ut_compare(value_to_normalized_json(value_t("hello"))._value, json_t("hello"));
}

QUARK_UNIT_TESTQ("value_to_normalized_json()", ""){
	quark::ut_compare(value_to_normalized_json(value_t(123))._value, json_t(123.0));
}

QUARK_UNIT_TESTQ("value_to_normalized_json()", ""){
	quark::ut_compare(value_to_normalized_json(value_t(true))._value, json_t(true));
}

QUARK_UNIT_TESTQ("value_to_normalized_json()", ""){
	quark::ut_compare(value_to_normalized_json(value_t(false))._value, json_t(false));
}

QUARK_UNIT_TESTQ("value_to_normalized_json()", ""){
	quark::ut_compare(value_to_normalized_json(value_t())._value, json_t());
}



value_t value_from_normalized_json(const ast_json_t& v2){
	QUARK_ASSERT(v2._value.check_invariant());

	const auto v = v2._value;

	if(v.is_object()){
		const auto obj = v.get_object();
		std::map<string, value_t> obj2;
		for(const auto e: obj){
			const auto key = e.first;
			const auto value = e.second;
			const auto value2 = make_json_value(value);
			obj2[key] = value2;
		}
		return make_dict_value(typeid_t::make_json_value(), obj2);
	}
	else if(v.is_array()){
		const auto elements = v.get_array();
		std::vector<value_t> elements2;
		for(const auto e: elements){
			const auto e2 = make_json_value(e);
			elements2.push_back(e2);
		}
		return make_vector_value(typeid_t::make_json_value(), elements2);
	}
	else if(v.is_string()){
		return value_t(v.get_string());
	}
	else if(v.is_number()){
		return value_t(static_cast<float>(v.get_number()));
	}
	else if(v.is_true()){
		return value_t(true);
	}
	else if(v.is_false()){
		return value_t(false);
	}
	else if(v.is_null()){
		return value_t();
	}
	else{
		QUARK_ASSERT(false);
	}
}






	//////////////////////////////////////////////////		function_definition_t


	function_definition_t::function_definition_t(
		const std::vector<member_t>& args,
		const std::vector<std::shared_ptr<statement_t>> statements,
		const typeid_t& return_type
	)
	:
		_args(args),
		_statements(statements),
		_host_function(0),
		_return_type(return_type)
	{
	}

	function_definition_t::function_definition_t(
		const std::vector<member_t>& args,
		const int host_function,
		const typeid_t& return_type
	)
	:
		_args(args),
		_host_function(host_function),
		_return_type(return_type)
	{
	}

	ast_json_t function_definition_t::to_json() const {
		typeid_t function_type = get_function_type(*this);
		return ast_json_t{json_t::make_array({
			"func-def",
			typeid_to_normalized_json(function_type)._value,
			members_to_json(_args),
			statements_to_json(_statements)._value,
			typeid_to_normalized_json(_return_type)._value
		})};
	}

	bool operator==(const function_definition_t& lhs, const function_definition_t& rhs){
		return
			lhs._args == rhs._args
			&& compare_shared_value_vectors(lhs._statements, rhs._statements)
			&& lhs._host_function == rhs._host_function
			&& lhs._return_type == rhs._return_type;
	}

	typeid_t get_function_type(const function_definition_t& f){
		return typeid_t::make_function(f._return_type, get_member_types(f._args));
	}

	std::string to_string(const function_definition_t& v){
		return "???missing impl for to_string(function_definition_t)";
/*
		auto s = _parts[0].to_string() + " (";

		//??? doesn't work when size() == 2, that is ONE argument.
		if(_parts.size() > 2){
			for(int i = 1 ; i < _parts.size() - 1 ; i++){
				s = s + _parts[i].to_string() + ",";
			}
			s = s + _parts[_parts.size() - 1].to_string();
		}
		s = s + ")";
		return s;
*/
	}


}	//	floyd