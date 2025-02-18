//
//  parser_statement.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "statement.h"


namespace floyd {

using std::make_shared;
using std::vector;



bool operator==(const body_t& lhs, const body_t& rhs){
	return
		lhs._statements == rhs._statements
		&& lhs._symbol_table == rhs._symbol_table;
}

bool body_t::check_invariant() const {
	for(const auto& i: _statements){
		QUARK_ASSERT(i.check_invariant());
	};
	return true;
}


////////////////////////////////////////////		SYMBOL


symbol_t make_type_symbol(const floyd::typeid_t& t){
	const auto a = value_t::make_typeid_value(t);
	return symbol_t::make_immutable_precalc(a);
}

std::string symbol_to_string(const symbol_t& s){
	std::stringstream out;

	out << "<symbol> {"
		<< (s._mutable_mode == symbol_t::mutable_mode::immutable ? "immutable" : "mutable" )
		<< " type: " << typeid_to_compact_string(s._value_type)
		<< " init: " << (s._init.is_undefined() ? "<none>" : value_and_type_to_string(s._init))
	<< "}";
	return out.str();
}


json_t symbol_to_json(const symbol_t& symbol){
	const auto symbol_type_str = symbol._mutable_mode == symbol_t::mutable_mode::immutable ? "immutable" : "mutable";
	const auto value_type = typeid_to_ast_json(symbol._value_type, json_tags::k_tag_resolve_state);

	const auto e2 = json_t::make_object({
		{ "symbol_type", symbol_type_str },
		{ "value_type", value_type },
		{ "init", value_to_ast_json(symbol._init, json_tags::k_tag_resolve_state) }
	});
	return e2;
}

symbol_t json_to_symbol(const json_t& e){
	const auto symbol_type = e.get_object_element("symbol_type").get_string();
	const auto value_type = e.get_object_element("value_type");

	if(symbol_type == "immutable" || symbol_type == "mutable"){
	}
	else{
		throw std::exception();
	}
	const auto symbol_type1 = symbol_type == "immutable" ? symbol_t::mutable_mode::immutable : symbol_t::mutable_mode::mutable1;
	const auto value_type1 = typeid_from_ast_json(value_type);
	const auto init = e.get_object_element("init");
	const auto init_value1 = init.is_null() ? value_t::make_undefined() : ast_json_to_value(value_type1, init);
	const auto result = symbol_t(symbol_type1, value_type1, init_value1);
	return result;
}



////////////////////////////////////////////		SYMBOLS



int add_temp(symbol_table_t& symbols, const std::string& name, const floyd::typeid_t& value_type){
	const auto s = symbol_t::make_immutable(value_type);
	symbols._symbols.push_back(std::pair<std::string, symbol_t>(name, s));
	return static_cast<int>(symbols._symbols.size() - 1);
}

const floyd::symbol_t* find_symbol(const symbol_table_t& symbol_table, const std::string& name){
	const auto it = std::find_if(
		symbol_table._symbols.begin(), symbol_table._symbols.end(),
		[&name](const std::pair<std::string, symbol_t>& x) { return x.first == name; }
	);
	
	return (it == symbol_table._symbols.end()) ? nullptr : &(it->second);
}
const floyd::symbol_t& find_symbol_required(const symbol_table_t& symbol_table, const std::string& name){
	const auto it = std::find_if(
		symbol_table._symbols.begin(), symbol_table._symbols.end(),
		[&name](const std::pair<std::string, symbol_t>& x) { return x.first == name; }
	);
	if(it == symbol_table._symbols.end()){
		throw std::exception();
	}
	return it->second;
}

std::vector<json_t> symbols_to_json(const symbol_table_t& symbol_table){
	std::vector<json_t> r;
	int symbol_index = 0;
	for(const auto& e: symbol_table._symbols){
		const auto symbol1 = symbol_to_json(e.second);
		const auto e2 = json_t::make_array({
			symbol_index,
			e.first,
			symbol1
		});
		r.push_back(e2);
		symbol_index++;
	}
	return r;
}

symbol_table_t astjson_to_symbols(const json_t& p){
	std::vector<std::pair<std::string, floyd::symbol_t>> result;
	const auto json_array = p.get_array();
	for(const auto& e: json_array){
		const auto symbol_array_json = e;
		const auto symbol_index = e.get_array_n(0).get_number();
		const auto symbol_name = e.get_array_n(1).get_string();
		const auto symbol = json_to_symbol(e.get_array_n(2));

		while(result.size() < symbol_index){
			result.push_back({"dummy_symbol #" + std::to_string(result.size()), symbol});
		}

		QUARK_ASSERT(result.size() == symbol_index);
		result.push_back({ symbol_name, symbol } );
	}
	return symbol_table_t{ result };
}



////////////////////////////////////////////		BODY



json_t body_to_json(const body_t& e){
	std::vector<json_t> statements;
	for(const auto& i: e._statements){
		statements.push_back(statement_to_json(i));
	}

	const auto symbols = symbols_to_json(e._symbol_table);

	return json_t::make_object({
		std::pair<std::string, json_t>{ "statements", json_t::make_array(statements) },
		std::pair<std::string, json_t>{ "symbols", json_t::make_array(symbols) }
	});
}

body_t json_to_body(const json_t& json){
	const auto statements = json.does_object_element_exist("statements") ? json.get_object_element("statements") : json_t();
	const auto statements1 = statements.is_null() ? std::vector<statement_t>() : astjson_to_statements(statements);

	const auto symbols = json.does_object_element_exist("symbols") ? json.get_object_element("symbols") : json_t();
	const auto symbols1 = symbols.is_null() ? symbol_table_t{} : astjson_to_symbols(symbols);

	return body_t(statements1, symbols1);
}





////////////////////////////////////////////		statement_t


bool statement_t::check_types_resolved(const std::vector<std::shared_ptr<statement_t>>& s){
	for(const auto& e: s){
		if(e->check_types_resolved() == false){
			return false;
		}
	}
	return true;
}

bool statement_t::check_types_resolved() const{
	QUARK_ASSERT(check_invariant());

	struct visitor_t {
		bool operator()(const return_statement_t& s) const{
			return s._expression.check_types_resolved();
		}
		bool operator()(const define_struct_statement_t& s) const{
			return s._def->check_types_resolved();
		}
		bool operator()(const define_function_statement_t& s) const{
			return s._def->check_types_resolved();
		}

		bool operator()(const bind_local_t& s) const{
			return true
				&& s._bindtype.check_types_resolved()
				&& s._expression.check_types_resolved()
				;
		}
		bool operator()(const assign_t& s) const{
			return s._expression.check_types_resolved();
		}
		bool operator()(const assign2_t& s) const{
			return s._expression.check_types_resolved();
		}
		bool operator()(const init2_t& s) const{
			return s._expression.check_types_resolved();
		}
		bool operator()(const block_statement_t& s) const{
			return s._body.check_types_resolved();
		}

		bool operator()(const ifelse_statement_t& s) const{
			return true
				&& s._condition.check_types_resolved()
				&& s._then_body.check_types_resolved()
				&& s._else_body.check_types_resolved()
				;
		}
		bool operator()(const for_statement_t& s) const{
			return true
				&& s._start_expression.check_types_resolved()
				&& s._end_expression.check_types_resolved()
				&& s._body.check_types_resolved()
				;
		}
		bool operator()(const while_statement_t& s) const{
			return true
				&& s._condition.check_types_resolved()
				&& s._body.check_types_resolved()
				;
		}

		bool operator()(const expression_statement_t& s) const{
			return s._expression.check_types_resolved();
		}
		bool operator()(const software_system_statement_t& s) const{
			return true;
		}
		bool operator()(const container_def_statement_t& s) const{
			return true;
		}
	};

	return std::visit(visitor_t{}, _contents);
}






statement_t astjson_to_statement__nonlossy(const json_t& statement0){
	QUARK_ASSERT(statement0.check_invariant());
	QUARK_ASSERT(statement0.is_array());

	const auto statement1 = unpack_loc(statement0);
	const auto loc = statement1.second;
	const auto statement = statement1.first;
	const std::string type = statement.get_array_n(0).get_string();

	//	[ "return", [ "k", 3, "int" ] ]
	if(type == statement_opcode_t::k_return){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto expr = astjson_to_expression(statement.get_array_n(1));
		return statement_t::make__return_statement(loc, expr);
	}

	//	[ "bind", "double", "x", EXPRESSION, {} ]
	//	Last element is a list of meta info, like "mutable" etc.
	else if(type == statement_opcode_t::k_bind){
		QUARK_ASSERT(statement.get_array_size() == 4 || statement.get_array_size() == 5);
		const auto bind_type = statement.get_array_n(1);
		const auto name = statement.get_array_n(2);
		const auto expr = statement.get_array_n(3);
		const auto meta = statement.get_array_size() >= 5 ? statement.get_array_n(4) : json_t();

		const auto bind_type2 = typeid_from_ast_json(bind_type);
		const auto name2 = name.get_string();
		const auto expr2 = astjson_to_expression(expr);
		bool mutable_flag = !meta.is_null() && meta.does_object_element_exist("mutable");
		const auto mutable_mode = mutable_flag ? statement_t::bind_local_t::k_mutable : statement_t::bind_local_t::k_immutable;
		return statement_t::make__bind_local(loc, name2, bind_type2, expr2, mutable_mode);
	}

	//	[ "assign", "x", EXPRESSION ]
	else if(type == statement_opcode_t::k_assign){
		QUARK_ASSERT(statement.get_array_size() == 3);
		const auto name = statement.get_array_n(1);
		const auto expr = statement.get_array_n(2);

		const auto name2 = name.get_string();
		const auto expr2 = astjson_to_expression(expr);
		return statement_t::make__assign(loc, name2, expr2);
	}

	//	[ "assign2", parent_index, variable_index, EXPRESSION ]
	else if(type == statement_opcode_t::k_assign2){
		QUARK_ASSERT(statement.get_array_size() == 4);
		const auto parent_index = (int)statement.get_array_n(1).get_number();
		const auto variable_index = (int)statement.get_array_n(2).get_number();
		const auto expr = statement.get_array_n(3);

		const auto expr2 = astjson_to_expression(expr);
		return statement_t::make__assign2(loc, variable_address_t::make_variable_address(parent_index, variable_index), expr2);
	}

	//	[ "init2", parent_index, variable_index, EXPRESSION ]
	else if(type == statement_opcode_t::k_init2){
		QUARK_ASSERT(statement.get_array_size() == 4);
		const auto parent_index = (int)statement.get_array_n(1).get_number();
		const auto variable_index = (int)statement.get_array_n(2).get_number();
		const auto expr = statement.get_array_n(3);

		const auto expr2 = astjson_to_expression(expr);
		return statement_t::make__init2(loc, variable_address_t::make_variable_address(parent_index, variable_index), expr2);
	}

	//	[ "block", [ STATEMENTS ] ]
	else if(type == statement_opcode_t::k_block){
		QUARK_ASSERT(statement.get_array_size() == 2);

		const auto statements = statement.get_array_n(1);
		const auto r = astjson_to_statements(statements);

		const auto body = body_t(r);
		return statement_t::make__block_statement(loc, body);
	}

	else if(type == statement_opcode_t::k_def_struct){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto struct_def = statement.get_array_n(1);
		const auto name = struct_def.get_object_element("name").get_string();
		const auto members = struct_def.get_object_element("members").get_array();

		const auto members2 = members_from_json(members);
		const auto struct_def2 = struct_definition_t(members2);

		const auto s = statement_t::define_struct_statement_t{ name, std::make_shared<struct_definition_t>(struct_def2) };
		return statement_t::make__define_struct_statement(loc, s);
	}

	else if(type == statement_opcode_t::k_def_func){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto def = statement.get_array_n(1);
		const auto name = def.get_object_element("name");
		const auto args = def.get_object_element("args");
		const auto fstatements = def.get_object_element("statements");
		const auto return_type = def.get_object_element("return_type");
		const auto impure = def.get_object_element("impure");

		const auto name2 = name.get_string();
		const auto args2 = members_from_json(args);
		const auto fstatements2 = astjson_to_statements(fstatements);
		const auto return_type2 = typeid_from_ast_json(return_type);

		if(impure.is_true() == false && impure.is_false() == false){
			quark::throw_exception();
		}
		const auto pure = impure.is_false();
		const auto function_typeid = typeid_t::make_function(return_type2, get_member_types(args2), pure ? epure::pure : epure::impure);
		const auto body = body_t{fstatements2};
		const auto function_def = function_definition_t::make_floyd_func(k_no_location, name2, function_typeid, args2, make_shared<body_t>(body));

		const auto s = statement_t::define_function_statement_t{ name2, std::make_shared<function_definition_t>(function_def) };
		return statement_t::make__define_function_statement(loc, s);
	}

	//	[ "if", CONDITION_EXPR, THEN_STATEMENTS, ELSE_STATEMENTS ]
	//	Else is optional.
	else if(type == statement_opcode_t::k_if){
		QUARK_ASSERT(statement.get_array_size() == 3 || statement.get_array_size() == 4);
		const auto condition_expression = statement.get_array_n(1);
		const auto then_statements = statement.get_array_n(2);
		const auto else_statements = statement.get_array_size() == 4 ? statement.get_array_n(3) : json_t::make_array();

		const auto condition_expression2 = astjson_to_expression(condition_expression);
		const auto then_statements2 = astjson_to_statements(then_statements);
		const auto else_statements2 = astjson_to_statements(else_statements);

		return statement_t::make__ifelse_statement(
			loc,
			condition_expression2,
			body_t{ then_statements2 },
			body_t{ else_statements2 }
		);
	}
	else if(type == statement_opcode_t::k_for){
		QUARK_ASSERT(statement.get_array_size() == 6);
		const auto for_mode = statement.get_array_n(1);
		const auto iterator_name = statement.get_array_n(2);
		const auto start_expression = statement.get_array_n(3);
		const auto end_expression = statement.get_array_n(4);
		const auto body_statements = statement.get_array_n(5);

		const auto start_expression2 = astjson_to_expression(start_expression);
		const auto end_expression2 = astjson_to_expression(end_expression);
		const auto body_statements2 = astjson_to_statements(body_statements);

		const auto range_type = for_mode.get_string() == "open-range" ? statement_t::for_statement_t::k_open_range : statement_t::for_statement_t::k_closed_range;
		return statement_t::make__for_statement(
			loc,
			iterator_name.get_string(),
			start_expression2,
			end_expression2,
			body_t{ body_statements2 },
			range_type
		);
	}
	else if(type == statement_opcode_t::k_while){
		QUARK_ASSERT(statement.get_array_size() == 3);
		const auto expression = statement.get_array_n(1);
		const auto body_statements = statement.get_array_n(2);

		const auto expression2 = astjson_to_expression(expression);
		const auto body_statements2 = astjson_to_statements(body_statements);

		return statement_t::make__while_statement(loc, expression2, body_t{body_statements2});
	}

	//	[ "expression-statement", EXPRESSION ]
	else if(type == statement_opcode_t::k_expression_statement){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto expr = statement.get_array_n(1);
		const auto expr2 = astjson_to_expression(expr);
		return statement_t::make__expression_statement(loc, expr2);
	}

	else if(type == statement_opcode_t::k_software_system){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto json_data = statement.get_array_n(1);

		return statement_t::make__software_system_statement(loc, json_data);
	}
	else if(type == statement_opcode_t::k_container_def){
		QUARK_ASSERT(statement.get_array_size() == 2);
		const auto json_data = statement.get_array_n(1);

		return statement_t::make__container_def_statement(loc, json_data);
	}

	else{
		quark::throw_runtime_error("Illegal statement.");
	}
}

const std::vector<statement_t> astjson_to_statements(const json_t& p){
	QUARK_ASSERT(p.check_invariant());
	QUARK_ASSERT(p.is_array());

	vector<statement_t> statements2;
	for(const auto& statement: p.get_array()){
		const auto s2 = astjson_to_statement__nonlossy(statement);
		statements2.push_back(s2);
	}
	return statements2;
}

json_t statement_to_json(const statement_t& e){
	QUARK_ASSERT(e.check_invariant());

	struct visitor_t {
		const statement_t& statement;

		json_t operator()(const statement_t::return_statement_t& s) const{
			return make_statement1(k_no_location, statement_opcode_t::k_return, expression_to_json(s._expression));
		}
		json_t operator()(const statement_t::define_struct_statement_t& s) const{
			return make_statement2(k_no_location, statement_opcode_t::k_def_struct, json_t(s._name), struct_definition_to_ast_json(*s._def));
		}
		json_t operator()(const statement_t::define_function_statement_t& s) const{
			return make_statement3(
				statement.location,
				statement_opcode_t::k_def_func,
				json_t(s._name),
				function_def_expression_to_ast_json(*s._def),
				s._def->_function_type.get_function_pure() == epure::impure ? true : false
			);
		}

		json_t operator()(const statement_t::bind_local_t& s) const{
			bool mutable_flag = s._locals_mutable_mode == statement_t::bind_local_t::k_mutable;
			const auto meta = mutable_flag
				? json_t::make_object({
					std::pair<std::string, json_t>{"mutable", mutable_flag}
				})
				: json_t();

			return make_statement4(
				statement.location,
				statement_opcode_t::k_bind,
				s._new_local_name,
				typeid_to_ast_json(s._bindtype, json_tags::k_tag_resolve_state),
				expression_to_json(s._expression),
				meta
			);
		}
		json_t operator()(const statement_t::assign_t& s) const{
			return make_statement2(
				statement.location,
				statement_opcode_t::k_assign,
				s._local_name,
				expression_to_json(s._expression)
			);
		}
		json_t operator()(const statement_t::assign2_t& s) const{
			return make_statement3(
				statement.location,
				statement_opcode_t::k_assign2,
				s._dest_variable._parent_steps,
				s._dest_variable._index,
				expression_to_json(s._expression)
			);
		}
		json_t operator()(const statement_t::init2_t& s) const{
			return make_statement3(
				statement.location,
				statement_opcode_t::k_init2,
				s._dest_variable._parent_steps,
				s._dest_variable._index,
				expression_to_json(s._expression)
			);
		}
		json_t operator()(const statement_t::block_statement_t& s) const{
			return make_statement1(k_no_location, statement_opcode_t::k_block, body_to_json(s._body));
		}

		json_t operator()(const statement_t::ifelse_statement_t& s) const{
			return make_statement3(
				statement.location,
				statement_opcode_t::k_if,
				expression_to_json(s._condition),
				body_to_json(s._then_body),
				body_to_json(s._else_body)
			);
		}
		json_t operator()(const statement_t::for_statement_t& s) const{
			return make_statement4(
				statement.location,
				statement_opcode_t::k_for,
				//??? open_range?
				json_t("closed_range"),
				expression_to_json(s._start_expression),
				expression_to_json(s._end_expression),
				body_to_json(s._body)
			);
		}
		json_t operator()(const statement_t::while_statement_t& s) const{
			return make_statement2(
				statement.location,
				statement_opcode_t::k_while,
				expression_to_json(s._condition),
				body_to_json(s._body)
			);
		}


		json_t operator()(const statement_t::expression_statement_t& s) const{
			return make_statement1(
				statement.location,
				statement_opcode_t::k_expression_statement,
				expression_to_json(s._expression)
			);
		}
		json_t operator()(const statement_t::software_system_statement_t& s) const{
			return make_statement1(
				statement.location,
				statement_opcode_t::k_software_system,
				s._json_data
			);
		}
		json_t operator()(const statement_t::container_def_statement_t& s) const{
			return make_statement1(
				statement.location,
				statement_opcode_t::k_container_def,
				s._json_data
			);
		}
	};

	return std::visit(visitor_t{ e }, e._contents);
}






QUARK_UNIT_TEST("", "", "", ""){
	const auto a = statement_t::make__block_statement(k_no_location, {});
	const auto b = statement_t::make__block_statement(k_no_location, {});
	QUARK_UT_VERIFY(a == b);
}

}
