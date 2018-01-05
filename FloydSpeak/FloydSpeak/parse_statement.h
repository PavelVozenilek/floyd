//
//  parser_statement.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parse_statement_hpp
#define parse_statement_hpp


#include "quark.h"

struct json_t;
struct seq_t;

namespace floyd_parser {



	/*
		INPUT:
			"{}"
			"{\n}"
			"{ int x = 1; }"
			"{ int x = 1; ; int y = 2; }"

		OUTPUT:
			["block", [ STATEMENTS ] ]
	*/
	std::pair<json_t, seq_t> parse_block(const seq_t& s);


	/*
		INPUT:
		Must start with "return".

		Examples:
			"return 0;"
			"return x + y;"

		OUTPUT:
			["return", EXPRESSION ]
	*/
	std::pair<json_t, seq_t> parse_return_statement(const seq_t& s);


	/*
		"int a = 10;"
		"float b = 0.3;"
		"int c = a + b;"
		"int b = f(a);"
		"string hello = f(a) + \"_suffix\";";

		...can contain trailing whitespace.


		"int b = f("hello");"
		"bool a = is_hello("hello")";

		OUTPUT:
			[ "bind", "<float>", "x", EXPRESSION ]
	*/
	std::pair<json_t, seq_t> parse_assignment_statement(const seq_t& s);


	/*
		A:
			if (2 > 1){
				...
			}


		B:
			if (2 > 1){
				...
			}
			else{
				...
			}

		C:
			if (2 > 1){
				...
			}
			else if(2 > 3){
				...
			}
			else{
				...
			}


		OUTPUT
			["if", EXPRESSION, THEN_STATEMENTS ]
			["if", EXPRESSION, THEN_STATEMENTS, ELSE_STATEMENTS ]
	*/
	std::pair<json_t, seq_t> parse_if_statement(const seq_t& pos);


	/*
		for (index in 1...5) {
			print(index)
		}
		for (tickMark in 0..<minutes) {
		}

		OUTPUT
			INIT_STATEMENT, CONDITION_EXPRESSION and POST_STATEMENT can also be null.
			[ "for", INIT_STATEMENT, CONDITION_EXPRESSION, POST_STATEMENT´ ]
			[ "for", null, CONDITION_EXPRESSION, null´ ]
			[ "for", null, null, null´ ]
	*/
	std::pair<json_t, seq_t> parse_for_statement(const seq_t& pos);


}	//	floyd_parser


#endif /* parser_statement_hpp */
