//
//  parser_struct.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parse_struct_def.h"
#include "parse_expression.h"
#include "parser_primitives.h"
#include "json_support.h"
#include "ast_typeid.h"
#include "ast_typeid_helpers.h"


namespace floyd {
	using std::string;
	using std::vector;
	using std::pair;


	std::pair<ast_json_t, seq_t>  parse_struct_definition_body(const seq_t& p, const std::string& name){
		const auto s2 = skip_whitespace(p);
		read_required_char(s2, '{');
		const auto body_pos = get_balanced(s2);

		vector<member_t> members;
		auto pos = seq_t(trim_ends(body_pos.first));
		while(!pos.empty()){
			const auto member_type = read_required_type(pos);
			const auto member_name = read_required_identifier(member_type.second);
			members.push_back(member_t(member_type.first, member_name.first));
			pos = read_optional_char(skip_whitespace(member_name.second), ';').second;
			pos = skip_whitespace(pos);
		}

		const auto r = json_t::make_array({
			"def-struct",
			json_t::make_object({
				{ "name", name },
				{ "members", members_to_json(members) }
			})
		});
		return { ast_json_t{r}, skip_whitespace(body_pos.second) };
	}

	std::pair<ast_json_t, seq_t>  parse_struct_definition(const seq_t& pos0){
		std::pair<bool, seq_t> token_pos = if_first(pos0, keyword_t::k_struct);
		QUARK_ASSERT(token_pos.first);

		const auto struct_name_pos = read_required_identifier(token_pos.second);

		const auto s2 = skip_whitespace(struct_name_pos.second);
		return parse_struct_definition_body(s2, struct_name_pos.first);
	}

	const std::string k_test_struct0 = "struct a {int x; string y; double z;}";


	QUARK_UNIT_TESTQ("parse_struct_definition", ""){
		const auto r = parse_struct_definition(seq_t(k_test_struct0));

		const auto expected =
		json_t::make_array({
			"def-struct",
			json_t::make_object({
				{ "name", "a" },
				{ "members", json_t::make_array({
					json_t::make_object({ { "name", "x"}, { "type", "^int"} }),
					json_t::make_object({ { "name", "y"}, { "type", "^string"} }),
					json_t::make_object({ { "name", "z"}, { "type", "^double"} })
				}) },
			})
		});

		ut_compare_jsons(r.first._value, expected);
		quark::ut_compare_strings(r.second.str(), "");
	}


}	//	namespace floyd

