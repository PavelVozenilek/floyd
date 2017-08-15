//
//  expressions.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 03/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef expressions_hpp
#define expressions_hpp

#include "quark.h"
#include <vector>
#include <string>

#include "parser_value.h"

struct json_t;

namespace floyd_ast {
	struct value_t;
	struct statement_t;

	json_t statements_to_json(const std::vector<std::shared_ptr<statement_t>>& e);
	json_t expression_to_json(const expression_t& e);
	json_t expressions_to_json(const std::vector<expression_t> v);

	bool is_simple_expression__2(const std::string& op);


	struct expr_base_t {
		public: virtual ~expr_base_t(){};

		public: virtual typeid_t get_result_type() const = 0;
		public: virtual json_t expr_base__to_json() const{ return json_t(); };
	};






	struct function_call_expr_t : public expr_base_t {
		public: virtual ~function_call_expr_t(){};

		public: function_call_expr_t(
			const expression_t& function,
			std::vector<expression_t> args,
			typeid_t result
		)
		:
			_function(std::make_shared<expression_t>(function)),
			_args(args),
			_result(result)
		{
		}

		public: virtual typeid_t get_result_type() const{
			return _result;
		}

		public: virtual json_t expr_base__to_json() const {
			return json_t::make_array({
				"call",
				expression_to_json(*_function),
				expressions_to_json(_args),
				typeid_to_json(_result)
			});
		}


		const std::shared_ptr<expression_t> _function;
		const std::vector<expression_t> _args;
		const typeid_t _result;
	};

	inline bool operator==(const function_call_expr_t& lhs, const function_call_expr_t& rhs){
		return
			lhs._function == rhs._function
			&& lhs._args == rhs._args
			&& lhs._result == rhs._result;
	}




	struct function_definition_expr_t : public expr_base_t {
		public: virtual ~function_definition_expr_t(){};

		public: function_definition_expr_t(
			const typeid_t& function_type,
			const std::vector<member_t>& args,
			const std::vector<std::shared_ptr<statement_t>> statements,
			const typeid_t& return_type
		)
		:
			_function_type(function_type),
			_args(args),
			_statements(statements),
			_return_type(return_type)
		{
		}

		public: virtual typeid_t get_result_type() const{
			return _return_type;
		}

		public: virtual json_t expr_base__to_json() const {
			return json_t::make_array({
				"func-def",
				typeid_to_json(_function_type),
				members_to_json(_args),
				statements_to_json(_statements),
				typeid_to_json(_return_type)
			});
		}


		const typeid_t _function_type;
		const std::vector<member_t> _args;
		const std::vector<std::shared_ptr<statement_t>> _statements;
		const typeid_t _return_type;
	};

	inline bool operator==(const function_definition_expr_t& lhs, const function_definition_expr_t& rhs){
		return
			lhs._function_type == rhs._function_type
			&& lhs._args == rhs._args
			&& compare_shared_value_vectors(lhs._statements, rhs._statements)
			&& lhs._return_type == rhs._return_type;
	}





	struct literal_expr_t : public expr_base_t {
		public: virtual ~literal_expr_t(){};

		public: literal_expr_t(const value_t& value) 
		:
			_value(value)
		{
		}

		public: virtual typeid_t get_result_type() const{
			return _value.get_type();
		}

		public: virtual json_t expr_base__to_json() const {
			return json_t::make_array({ "k", value_to_json(_value), typeid_to_json(_value.get_type()) });
		}


		const value_t _value;
	};

	inline bool operator==(const literal_expr_t& lhs, const literal_expr_t& rhs){
		return lhs._value == rhs._value;
	}







	//////////////////////////////////////////////////		expression_t

	/*
		Immutable. Value type.
	*/
	struct expression_t {
		public: static expression_t make_constant_value(const value_t& value)
		{
			return expression_t{
				floyd_basics::expression_type::k_constant,
				{},
				{},
				{},
				{},
				std::make_shared<literal_expr_t>(
					literal_expr_t{ value }
				)
			};
		}

		public: static expression_t make_constant_null();
		public: static expression_t make_constant_int(const int i);
		public: static expression_t make_constant_bool(const bool i);
		public: static expression_t make_constant_float(const float i);
		public: static expression_t make_constant_string(const std::string& s);

		public: bool is_constant() const;
		public: const value_t& get_constant() const;



		public: static expression_t make_simple_expression__2(
			floyd_basics::expression_type op,
			const expression_t& left,
			const expression_t& right
		);
		public: static expression_t make_unary_minus(const expression_t& expr);
		public: static expression_t make_conditional_operator(
			const expression_t& condition,
			const expression_t& a,
			const expression_t& b
		);

		public: static expression_t make_function_call(
			const expression_t& function,
			const std::vector<expression_t>& args,
			const typeid_t& result
		)
		{
			return expression_t{
				floyd_basics::expression_type::k_call,
				{},
				{},
				{},
				{},
				std::make_shared<function_call_expr_t>(
					function_call_expr_t{ function, args, result }
				)
			};
		}

		public: static expression_t make_function_definition(
			const typeid_t& function_type,
			const std::vector<member_t>& args,
			const std::vector<std::shared_ptr<statement_t>> statements,
			const typeid_t& return_type
		)
		{
			return expression_t{
				floyd_basics::expression_type::k_define_function,
				{},
				{},
				{},
				function_type,
				std::make_shared<function_definition_expr_t>(
					function_definition_expr_t{ function_type, args, statements, return_type }
				)
			};
		}

		/*
			Specify free variables.
			It will be resolved via static scopes: (global variable) <-(function argument) <- (function local variable) etc.
		*/
		public: static expression_t make_variable_expression(
			const std::string& variable,
			const typeid_t& result
		);

		/*
			Specifies a member of a struct.
		*/
		public: static expression_t make_resolve_member(
			const expression_t& parent_address,
			const std::string& member_name,
			const typeid_t& result
		);

		/*
			Looks up using a key. They key can be a sub-expression. Can be any type: index, string etc.
		*/
		public: static expression_t make_lookup(
			const expression_t& parent_address,
			const expression_t& lookup_key,
			const typeid_t& result
		);

		public: bool check_invariant() const;

		public: bool operator==(const expression_t& other) const;

		/*
			Returns pre-computed result of the expression - the type of value it represents.
			null if not resolved.
		*/
		public: typeid_t get_expression_type() const{
			QUARK_ASSERT(check_invariant());

			if(_expr){
				return _expr->get_result_type();
			}
			return _result_type;
		}

		public: floyd_basics::expression_type get_operation() const;
		public: const std::vector<expression_t>& get_expressions() const;
		public: const std::string& get_symbol() const;
		public: typeid_t get_result_type() const;
		public: const expr_base_t* get_expr() const{
			return _expr.get();
		}

		public: const function_call_expr_t* get_function_call() const {
			return dynamic_cast<const function_call_expr_t*>(_expr.get());
		}

		//////////////////////////		INTERNALS


		private: expression_t(
			const floyd_basics::expression_type operation,
			const std::vector<expression_t>& expressions,
			const std::shared_ptr<value_t>& constant,
			const std::string& symbol,
			const typeid_t& result_type,
			const std::shared_ptr<const expr_base_t>& expr
		);


		//////////////////////////		STATE
		private: std::string _debug;
		private: floyd_basics::expression_type _operation;
		private: std::vector<expression_t> _expressions;
		private: std::string _symbol;

		//	Tell what type of value this expression represents. Null if not yet defined.
		private: typeid_t _result_type;

		private: std::shared_ptr<const expr_base_t> _expr;
	};






	void trace(const expression_t& e);

	/*
		An expression is a json array where entries may be other json arrays.
		["+", ["+", 1, 2], ["k", 10]]
	*/
	json_t expression_to_json(const expression_t& e);

	json_t expressions_to_json(const std::vector<expression_t> v);

}	//	floyd_ast


#endif /* expressions_hpp */
