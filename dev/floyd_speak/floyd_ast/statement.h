//
//  parser_statement.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_statement_hpp
#define parser_statement_hpp

#include "expression.h"
#include "quark.h"

#include <vector>
#include <string>
//#include "../parts/xcode-libcxx-xcode9/variant"	//	https://github.com/youknowone/xcode-libcxx
#include <variant>


namespace floyd {
	struct statement_t;
	struct expression_t;


	//////////////////////////////////////		symbol_t

	/*
		This is an entry in the symbol table, kept for each environment/stack frame.
		When you make a local variable it gets an entry in symbol table, with a type and name but no value. Like a reservered slot.
		You can also add precalculated constants directly to the symbol table.


		# Function values
		These are stored as local variable reservations of correct function-signature-type. They are inited
		during execution, not const-values in symbol table.

		Function calls needs to evaluate callee expression.
		??? TODO: make functions const-values when possible.


		# Structs
		Struct-types are stored in symbol table as precalculated values. Struct instances are not.

		struct pixel_t { int red; int green; int blue; }

		- needs to become a precalculated symbol called "pixel_t" so print(pixel_t) etc works.
		- pixel_t variable =
			type: typeid_t = struct{ int red; int green; int blue; }
			const: value_t::typeid_value =
	*/

	struct symbol_t {
		enum class mutable_mode {
			immutable,
			mutable1
		};

		bool operator==(const symbol_t& other) const {
			return true
				&& _mutable_mode == other._mutable_mode
				&& _value_type == other._value_type
				&& _init == other._init
				;
		}

		public: bool check_invariant() const {
			QUARK_ASSERT(_init.is_undefined() || _init.get_type() == _value_type);
			return true;
		}

		public: symbol_t(mutable_mode mutable_mode, const floyd::typeid_t& value_type, const floyd::value_t& init_value) :
			_mutable_mode(mutable_mode),
			_value_type(value_type),
			_init(init_value)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: floyd::typeid_t get_type() const {
			QUARK_ASSERT(check_invariant());

			return _value_type;
		}

		public: static symbol_t make_immutable(const floyd::typeid_t& value_type){
			return symbol_t{ mutable_mode::immutable, value_type, {} };
		}

		public: static symbol_t make_immutable_arg(const floyd::typeid_t& value_type){
			return symbol_t{ mutable_mode::immutable, value_type, {} };
		}

		public: static symbol_t make_mutable(const floyd::typeid_t& value_type){
			return symbol_t{ mutable_mode::mutable1, value_type, {} };
		}

		public: static symbol_t make_immutable_precalc(const floyd::value_t& init_value){
			return symbol_t{ mutable_mode::immutable, init_value.get_type(), init_value };
		}


		//////////////////////////////////////		STATE
		mutable_mode _mutable_mode;
		floyd::typeid_t _value_type;

		//	If there is no initialization value, this member must be value_t::make_undefined();
		floyd::value_t _init;
	};

	symbol_t make_type_symbol(const floyd::typeid_t& t);
	std::string symbol_to_string(const symbol_t& symbol);



	//////////////////////////////////////		symbol_table_t


	struct symbol_table_t {
		bool check_invariant() const {
			return true;
		}

		bool operator==(const symbol_table_t& other) const {
			return _symbols == other._symbols;
		}


		public: std::vector<std::pair<std::string, symbol_t>> _symbols;
	};

	int add_temp(symbol_table_t& symbols, const std::string& name, const floyd::typeid_t& value_type);
	const floyd::symbol_t* find_symbol(const symbol_table_t& symbol_table, const std::string& name);
	const floyd::symbol_t& find_symbol_required(const symbol_table_t& symbol_table, const std::string& name);

	std::vector<json_t> symbols_to_json(const symbol_table_t& symbols);
	symbol_table_t astjson_to_symbols(const json_t& p);




	//////////////////////////////////////		body_t


	struct body_t {
		body_t(){
		}

//		body_t(const body_t& oth) = default;
//		body_t& operator=(const body_t& oth) = default;


		body_t(const std::vector<statement_t>& s) :
			_statements(s),
			_symbol_table{}
		{
		}

		body_t(const std::vector<statement_t>& statements, const symbol_table_t& symbols) :
			_statements(statements),
			_symbol_table(symbols)
		{
		}
		bool check_types_resolved() const;

		bool check_invariant() const;


		////////////////////		STATE
		std::vector<statement_t> _statements;
		symbol_table_t _symbol_table;
	};

	bool operator==(const body_t& lhs, const body_t& rhs);


	json_t body_to_json(const body_t& e);
	body_t json_to_body(const json_t& json);


	//////////////////////////////////////		statement_t

	/*
		Defines a statement, like "return" including any needed expression trees for the statement.
		Immutable
	*/
	struct statement_t {

		//////////////////////////////////////		return_statement_t



		struct return_statement_t {
			bool operator==(const return_statement_t& other) const {
				return _expression == other._expression;
			}

			expression_t _expression;
		};
		public: static statement_t make__return_statement(const location_t& location, const expression_t& expression){
			return statement_t(location, { return_statement_t{ expression } });
		}


		//////////////////////////////////////		define_struct_statement_t


		struct define_struct_statement_t {
			bool operator==(const define_struct_statement_t& other) const {
				return _name == other._name && _def == other._def;
			}

			std::string _name;
			std::shared_ptr<const struct_definition_t> _def;
		};
		public: static statement_t make__define_struct_statement(const location_t& location, const define_struct_statement_t& value){
			return statement_t(location, { define_struct_statement_t{ value } });
		}


		//////////////////////////////////////		define_function_statement_t


		struct define_function_statement_t {
			bool operator==(const define_function_statement_t& other) const {
				return _name == other._name && _def == other._def;
			}

			std::string _name;
			std::shared_ptr<const function_definition_t> _def;
		};
		public: static statement_t make__define_function_statement(const location_t& location, const define_function_statement_t& value){
			return statement_t(location, { define_function_statement_t{ value } });
		}


		//////////////////////////////////////		bind_local_t

		//	Created a new name in current lexical scope and initialises it with an expression.

		struct bind_local_t {
			enum mutable_mode {
				k_mutable = 2,
				k_immutable
			};

			bool operator==(const bind_local_t& other) const {
				return _new_local_name == other._new_local_name
					&& _bindtype == other._bindtype
					&& _expression == other._expression
					&& _locals_mutable_mode == other._locals_mutable_mode;
			}

			std::string _new_local_name;
			typeid_t _bindtype;
			expression_t _expression;
			mutable_mode _locals_mutable_mode;
		};
		public: static statement_t make__bind_local(const location_t& location, const std::string& new_local_name, const typeid_t& bindtype, const expression_t& expression, bind_local_t::mutable_mode locals_mutable_mode){
			return statement_t(location, { bind_local_t{ new_local_name, bindtype, expression, locals_mutable_mode } });
		}


		//////////////////////////////////////		assign_t

		//	Mutate an existing variable, specified by name.

		struct assign_t {
			bool operator==(const assign_t& other) const {
				return _local_name == other._local_name
					&& _expression == other._expression;
			}

			std::string _local_name;
			expression_t _expression;
		};
		public: static statement_t make__assign(const location_t& location, const std::string& local_name, const expression_t& expression){
			return statement_t(location, { assign_t{ local_name, expression} });
		}


		//////////////////////////////////////		assign2_t


		//	Mutate an existing variable, specified by resolved scope ID.

		struct assign2_t {
			bool operator==(const assign2_t& other) const {
				return _dest_variable == other._dest_variable
					&& _expression == other._expression;
			}

			variable_address_t _dest_variable;
			expression_t _expression;
		};

		public: static statement_t make__assign2(const location_t& location, const variable_address_t& dest_variable, const expression_t& expression){
			return statement_t(location, { assign2_t{ dest_variable, expression} });
		}


		//////////////////////////////////////		init2_t


		//	Initialise an existing variable, specified by resolved scope ID.

		struct init2_t {
			bool operator==(const init2_t& other) const {
				return _dest_variable == other._dest_variable
					&& _expression == other._expression;
			}

			variable_address_t _dest_variable;
			expression_t _expression;
		};

		public: static statement_t make__init2(const location_t& location, const variable_address_t& dest_variable, const expression_t& expression){
			return statement_t(location, { init2_t{ dest_variable, expression} });
		}


		//////////////////////////////////////		block_statement_t


		struct block_statement_t {
			bool operator==(const block_statement_t& other) const {
				return _body == other._body;
			}

			body_t _body;
		};

		public: static statement_t make__block_statement(const location_t& location, const body_t& body){
			return statement_t(location, { block_statement_t{ body} });
		}


		//////////////////////////////////////		ifelse_statement_t


		struct ifelse_statement_t {
			bool operator==(const ifelse_statement_t& other) const {
				return
					_condition == other._condition
					&& _condition == other._condition
					&& _then_body == other._then_body
					&& _else_body == other._else_body
					;
			}

			expression_t _condition;
			body_t _then_body;
			body_t _else_body;
		};
		public: static statement_t make__ifelse_statement(
			const location_t& location,
			const expression_t& condition,
			const body_t& then_body,
			const body_t& else_body
		){
			return statement_t(location, { ifelse_statement_t{ condition, then_body, else_body} });
		}


		//////////////////////////////////////		for_statement_t


		struct for_statement_t {
			enum range_type {
				k_open_range,	//	..<
				k_closed_range	//	...
			};

			bool operator==(const for_statement_t& other) const {
				return
					_iterator_name == other._iterator_name
					&& _start_expression == other._start_expression
					&& _end_expression == other._end_expression
					&& _body == other._body
					&& _range_type == other._range_type
					;
			}

			std::string _iterator_name;
			expression_t _start_expression;
			expression_t _end_expression;
			body_t _body;
			range_type _range_type;
		};
		public: static statement_t make__for_statement(
			const location_t& location,
			const std::string iterator_name,
			const expression_t& start_expression,
			const expression_t& end_expression,
			const body_t& body,
			for_statement_t::range_type range_type
		){
			return statement_t(location, { for_statement_t{ iterator_name, start_expression, end_expression, body, range_type } });
		}


		//////////////////////////////////////		software_system_statement_t


		struct software_system_statement_t {
			bool operator==(const software_system_statement_t& other) const {
				return _json_data == other._json_data;
			}

			json_t _json_data;
		};

		public: static statement_t make__software_system_statement(
			const location_t& location,
			json_t json_data
		){
			return statement_t(location, { software_system_statement_t{ json_data } });
		}


		//////////////////////////////////////		container_def_statement_t


		struct container_def_statement_t {
			bool operator==(const container_def_statement_t& other) const {
				return _json_data == other._json_data;
			}

			json_t _json_data;
		};

		public: static statement_t make__container_def_statement(
			const location_t& location,
			json_t json_data
		){
			return statement_t(location, { container_def_statement_t{ json_data } });
		}


		//////////////////////////////////////		while_statement_t


		struct while_statement_t {
			bool operator==(const while_statement_t& other) const {
				return
					_condition == other._condition
					&& _body == other._body;
			}

			expression_t _condition;
			body_t _body;
		};

		public: static statement_t make__while_statement(
			const location_t& location,
			const expression_t& condition,
			const body_t& body
		){
			return statement_t(location, { while_statement_t{ condition, body } });
		}


		//////////////////////////////////////		expression_statement_t


		struct expression_statement_t {
			bool operator==(const expression_statement_t& other) const {
				return _expression == other._expression;
			}

			expression_t _expression;
		};
		public: static statement_t make__expression_statement(const location_t& location, const expression_t& expression){
			return statement_t(location, { expression_statement_t{ expression } });
		}


		//////////////////////////////////////		statement_t


		typedef std::variant<
			return_statement_t,
			define_struct_statement_t,
			define_function_statement_t,
			bind_local_t,
			assign_t,
			assign2_t,
			init2_t,
			block_statement_t,
			ifelse_statement_t,
			for_statement_t,
			while_statement_t,
			expression_statement_t,
			software_system_statement_t,
			container_def_statement_t
		> statement_variant_t;

		statement_t(const location_t& location, const statement_variant_t& contents) :
			debug_string(""),
			location(location),
			_contents(contents)
		{
//			const auto json = statement_to_json(*this);
//			debug_string = json_to_compact_string(json);

		}

		bool check_invariant() const {
			return true;
		}

		//??? make into free function
 		public: static bool check_types_resolved(const std::vector<std::shared_ptr<statement_t>>& s);

		public: bool check_types_resolved() const;


		//////////////////////////////////////		STATE

		std::string debug_string;
		location_t location;
		statement_variant_t _contents;
	};

	static bool operator==(const statement_t& lhs, const statement_t& rhs){
		return lhs.location == rhs.location && lhs._contents == rhs._contents;
	}


const std::vector<statement_t> astjson_to_statements(const json_t& p);
json_t statement_to_json(const statement_t& e);



	inline bool body_t::check_types_resolved() const{
		for(const auto& e: _statements){
			if(e.check_types_resolved() == false){
				return false;
			}
		}
		for(const auto& s: _symbol_table._symbols){
			if(s.first != "**undef**" && s.second._value_type.check_types_resolved() == false){
				return false;
			}
			if(s.first != "**undef**" && s.second._init.is_undefined() == false && s.second._init.get_type().check_types_resolved() == false){
				return false;
			}
		}
		return true;
	}



}	//	floyd


#endif /* parser_statement_hpp */
