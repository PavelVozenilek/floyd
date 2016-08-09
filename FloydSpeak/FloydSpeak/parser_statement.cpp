//
//  parser_statement.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parser_statement.h"

#include "parser_ast.h"
#include "parser_expression.h"
#include "parser_function.h"
#include "parser_struct.h"
#include "parser_primitives.h"


namespace floyd_parser {

	using std::string;
	using std::vector;
	using std::pair;
	using std::make_shared;
	using std::shared_ptr;


	statement_t make__bind_statement(const bind_statement_t& value){
		return statement_t(value);
	}

	statement_t make__bind_statement(const std::string& identifier, const expression_t& e){
		return statement_t(bind_statement_t{identifier, std::make_shared<expression_t>(e)});
	}

	statement_t make__return_statement(const return_statement_t& value){
		return statement_t(value);
	}


	statement_t makie_return_statement(const expression_t& expression){
		return statement_t(return_statement_t{make_shared<expression_t>(expression)});
	}

	void trace(const statement_t& s){
		if(s._bind_statement){
			std::string t = "bind_statement_t: \"" + s._bind_statement->_identifier + "\"";
			QUARK_SCOPED_TRACE(t);
			trace(*s._bind_statement->_expression);
		}

		else if(s._define_struct){
			QUARK_SCOPED_TRACE("define_struct_statement_t: \"" + s._define_struct->_type_identifier);
			trace(s._define_struct->_struct_def);
		}
		else if(s._define_function){
			QUARK_SCOPED_TRACE("define_function_statement_t: \"" + s._define_function->_type_identifier);
			trace(s._define_function->_function_def);
		}

		else if(s._return_statement){
			QUARK_SCOPED_TRACE("return_statement_t");
			trace(*s._return_statement->_expression);
		}
		else{
			QUARK_ASSERT(false);
		}
	}



	//////////////////////////////////////		statement_t



	bool statement_t::check_invariant() const {
		if(_bind_statement){
			QUARK_ASSERT(_bind_statement);
			QUARK_ASSERT(!_define_struct);
			QUARK_ASSERT(!_define_function);
			QUARK_ASSERT(!_return_statement);
		}
		else if(_define_struct){
			QUARK_ASSERT(!_bind_statement);
			QUARK_ASSERT(_define_struct);
			QUARK_ASSERT(!_define_function);
			QUARK_ASSERT(!_return_statement);
		}
		else if(_define_function){
			QUARK_ASSERT(!_bind_statement);
			QUARK_ASSERT(!_define_struct);
			QUARK_ASSERT(_define_function);
			QUARK_ASSERT(!_return_statement);
		}
		else if(_return_statement){
			QUARK_ASSERT(!_bind_statement);
			QUARK_ASSERT(!_define_struct);
			QUARK_ASSERT(!_define_function);
			QUARK_ASSERT(_return_statement);
		}
		else{
			QUARK_ASSERT(false);
		}
		return true;
	}

}	//	floyd_parser
