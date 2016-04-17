//
//  main.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 27/03/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

/*
	floydrt -- floyd runtime.
	floydgen -- generated code from user program.
	floydc -- floyd compiler
*/

#include "floyd_parser.h"

/*
#define QUARK_ASSERT_ON true
#define QUARK_TRACE_ON true
#define QUARK_UNIT_TESTS_ON true
*/

#include "steady_vector.h"
#include <string>
#include <memory>
#include <map>
#include <iostream>
#include <cmath>

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;

/*
	AST ABSTRACT SYNTAX TREE

https://en.wikipedia.org/wiki/Abstract_syntax_tree

https://en.wikipedia.org/wiki/Parsing_expression_grammar
https://en.wikipedia.org/wiki/Parsing
*/

/*
	Compiler version #1
	===============================
	main()
	functions
	bool, int, string
	true, false
	return
	if-then-else
	foreach
	map
	seq
	(?:)
	assert()

	Later version
	===============================
	struct
	map
	while
	mutable
*/




struct test_value_class_a {
	int _a = 10;
	int _b = 10;

	bool operator==(const test_value_class_a& other){
		return _a == other._a && _b == other._b;
	}
};

QUARK_UNIT_TESTQ("test_value_class_a", "what is needed for basic operations"){
	test_value_class_a a;
	test_value_class_a b = a;

	QUARK_TEST_VERIFY(b._a == 10);
	QUARK_TEST_VERIFY(a == b);
}





typedef pair<string, string> seq;

struct seq2 {
	seq2 substr(size_t pos, size_t count = string::npos){
		return seq2();
	}
	
	const shared_ptr<const string> _str;
	std::size_t _pos;
};

const char* basic_types[] = {
	"bool",
	"char???",
	"-code_point",
	"-double",
	"float",
	"float32",
	"float80",
	"-hash",
	"int",
	"int16",
	"int32",
	"int64",
	"int8",
	"-path",
	"string",
	"-text"
};

const char* _advanced_types[] = {
	"-clock",
	"-defect_exception",
	"-dyn",
	"-dyn**<>",
	"-enum",
	"-exception",
	"map",
	"-protocol",
	"-rights",
	"-runtime_exception",
	"seq",
	"struct",
	"-typedef",
	"-vector"
};

const char* _keywords[] = {
	"assert",
	"-catch",
	"-deserialize()",
	"-diff()",
	"else",
	"-ensure",
	"false",
	"foreach",
	"-hash()",
	"if",
	"-invariant",
	"log",
	"mutable",
	"-namespace???",
	"-null",
	"-private",
	"-property",
	"-prove",
	"-require",
	"return",
	"-serialize()",
	"-swap",
	"-switch",
	"-tag",
	"-test",
	"-this",
	"true",
	"-try",
	"-typecast",
	"-typeof",
	"while"
};



void IncreaseIndent(){
	auto r = quark::get_runtime();
	r->runtime_i__add_log_indent(1);
}

void DecreateIndent(){
	auto r = quark::get_runtime();
	r->runtime_i__add_log_indent(-1);
}


//////////////////////////////////////////////////		Text parsing primitives



const string whitespace_chars = " \n\t";
const string identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
const string brackets = "(){}[]<>";
const string open_brackets = "({[<";
const string type_chars = identifier_chars + brackets;
const string number_chars = "0123456789.";
const string operator_chars = "+-*/.";


seq read_while(const string& s, const string& match){
	size_t pos = 0;
	while(pos < s.size() && match.find(s[pos]) != string::npos){
		pos++;
	}

	return seq(
		s.substr(0, pos),
		s.substr(pos)
	);
}

QUARK_UNIT_TEST("", "read_while()", "", ""){
	QUARK_TEST_VERIFY(read_while("", whitespace_chars) == seq("", ""));
	QUARK_TEST_VERIFY(read_while(" ", whitespace_chars) == seq(" ", ""));
	QUARK_TEST_VERIFY(read_while("    ", whitespace_chars) == seq("    ", ""));

	QUARK_TEST_VERIFY(read_while("while", whitespace_chars) == seq("", "while"));
	QUARK_TEST_VERIFY(read_while(" while", whitespace_chars) == seq(" ", "while"));
	QUARK_TEST_VERIFY(read_while("    while", whitespace_chars) == seq("    ", "while"));
}

seq read_until(const string& s, const string& match){
	size_t pos = 0;
	while(pos < s.size() && match.find(s[pos]) == string::npos){
		pos++;
	}

	return { s.substr(0, pos), s.substr(pos) };
}

QUARK_UNIT_TEST("", "read_until()", "", ""){
	QUARK_TEST_VERIFY(read_until("", ",.") == seq("", ""));
	QUARK_TEST_VERIFY(read_until("ab", ",.") == seq("ab", ""));
	QUARK_TEST_VERIFY(read_until("ab,cd", ",.") == seq("ab", ",cd"));
	QUARK_TEST_VERIFY(read_until("ab.cd", ",.") == seq("ab", ".cd"));
}


/*
	first: skipped whitespaces
	second: all / any text after whitespace.
*/
std::string skip_whitespace(const string& s){
	return read_while(s, whitespace_chars).second;
}

QUARK_UNIT_TEST("", "skip_whitespace()", "", ""){
	QUARK_TEST_VERIFY(skip_whitespace("") == "");
	QUARK_TEST_VERIFY(skip_whitespace(" ") == "");
	QUARK_TEST_VERIFY(skip_whitespace("\t") == "");
	QUARK_TEST_VERIFY(skip_whitespace("int blob()") == "int blob()");
	QUARK_TEST_VERIFY(skip_whitespace("\t\t\t int blob()") == "int blob()");
}


pair<char, string> read_char(const std::string& s){
	if(!s.empty()){
		return { s[0], s.substr(1) };
	}
	else{
		throw std::runtime_error("expected character.");
	}
}

/*
	Returns "rest" if ch is found, else throws exceptions.
*/
std::string read_required_char(const std::string& s, char ch){
	if(s.size() > 0 && s[0] == ch){
		return s.substr(1);
	}
	else{
		throw std::runtime_error("expected character '" + string(1, ch)  + "'.");
	}
}

pair<bool, std::string> read_optional_char(const std::string& s, char ch){
	if(s.size() > 0 && s[0] == ch){
		return { true, s.substr(1) };
	}
	else{
		return { false, s };
	}
}


bool peek_compare_char(const std::string& s, char ch){
	return s.size() > 0 && s[0] == ch;
}

bool peek_string(const std::string& s, const std::string& peek){
	return s.size() >= peek.size() && s.substr(0, peek.size()) == peek;
}

QUARK_UNIT_TEST("", "peek_string()", "", ""){
	QUARK_TEST_VERIFY(peek_string("", "") == true);
	QUARK_TEST_VERIFY(peek_string("a", "a") == true);
	QUARK_TEST_VERIFY(peek_string("a", "b") == false);
	QUARK_TEST_VERIFY(peek_string("", "b") == false);
	QUARK_TEST_VERIFY(peek_string("abc", "abc") == true);
	QUARK_TEST_VERIFY(peek_string("abc", "abx") == false);
	QUARK_TEST_VERIFY(peek_string("abc", "ab") == true);
}

bool is_whitespace(char ch){
	return whitespace_chars.find(string(1, ch)) != string::npos;
}

QUARK_UNIT_TEST("", "is_whitespace()", "", ""){
	QUARK_TEST_VERIFY(is_whitespace('x') == false);
	QUARK_TEST_VERIFY(is_whitespace(' ') == true);
	QUARK_TEST_VERIFY(is_whitespace('\t') == true);
	QUARK_TEST_VERIFY(is_whitespace('\n') == true);
}


/*
	Skip leading whitespace, get string while type-char.
*/
seq get_type(const string& s){
	const auto a = skip_whitespace(s);
	const auto b = read_while(a, type_chars);
	return b;
}

/*
	Skip leading whitespace, get string while identifier char.
*/
seq get_identifier(const string& s){
	const auto a = skip_whitespace(s);
	const auto b = read_while(a, identifier_chars);
	return b;
}


bool is_start_char(char c){
	return c == '(' || c == '[' || c == '{' || c == '<';
}

QUARK_UNIT_TEST("", "is_start_char()", "", ""){
	QUARK_TEST_VERIFY(is_start_char('('));
	QUARK_TEST_VERIFY(is_start_char('['));
	QUARK_TEST_VERIFY(is_start_char('{'));
	QUARK_TEST_VERIFY(is_start_char('<'));

	QUARK_TEST_VERIFY(!is_start_char(')'));
	QUARK_TEST_VERIFY(!is_start_char(']'));
	QUARK_TEST_VERIFY(!is_start_char('}'));
	QUARK_TEST_VERIFY(!is_start_char('>'));

	QUARK_TEST_VERIFY(!is_start_char(' '));
}

bool is_end_char(char c){
	return c == ')' || c == ']' || c == '}' || c == '>';
}

QUARK_UNIT_TEST("", "is_end_char()", "", ""){
	QUARK_TEST_VERIFY(!is_end_char('('));
	QUARK_TEST_VERIFY(!is_end_char('['));
	QUARK_TEST_VERIFY(!is_end_char('{'));
	QUARK_TEST_VERIFY(!is_end_char('<'));

	QUARK_TEST_VERIFY(is_end_char(')'));
	QUARK_TEST_VERIFY(is_end_char(']'));
	QUARK_TEST_VERIFY(is_end_char('}'));
	QUARK_TEST_VERIFY(is_end_char('>'));

	QUARK_TEST_VERIFY(!is_end_char(' '));
}


char start_char_to_end_char(char start_char){
	QUARK_ASSERT(is_start_char(start_char));

	if(start_char == '('){
		return ')';
	}
	else if(start_char == '['){
		return ']';
	}
	else if(start_char == '{'){
		return '}';
	}
	else if(start_char == '<'){
		return '>';
	}
	else {
		QUARK_ASSERT(false);
	}
}

QUARK_UNIT_TEST("", "start_char_to_end_char()", "", ""){
	QUARK_TEST_VERIFY(start_char_to_end_char('(') == ')');
	QUARK_TEST_VERIFY(start_char_to_end_char('[') == ']');
	QUARK_TEST_VERIFY(start_char_to_end_char('{') == '}');
	QUARK_TEST_VERIFY(start_char_to_end_char('<') == '>');
}

/*
	First char is the start char, like '(' or '{'.
*/

seq get_balanced(const string& s){
	QUARK_ASSERT(s.size() > 0);

	const char start_char = s[0];
	QUARK_ASSERT(is_start_char(start_char));

	const char end_char = start_char_to_end_char(start_char);

	int depth = 0;
	size_t pos = 0;
	while(pos < s.size() && !(depth == 1 && s[pos] == end_char)){
		const char c = s[pos];
		if(is_start_char(c)) {
			depth++;
		}
		else if(is_end_char(c)){
			if(depth == 0){
				throw std::runtime_error("unbalanced ([{< >}])");
			}
			depth--;
		}
		pos++;
	}

	return { s.substr(0, pos + 1), s.substr(pos + 1) };
}

QUARK_UNIT_TEST("", "get_balanced()", "", ""){
//	QUARK_TEST_VERIFY(get_balanced("") == seq("", ""));
	QUARK_TEST_VERIFY(get_balanced("()") == seq("()", ""));
	QUARK_TEST_VERIFY(get_balanced("(abc)") == seq("(abc)", ""));
	QUARK_TEST_VERIFY(get_balanced("(abc)def") == seq("(abc)", "def"));
	QUARK_TEST_VERIFY(get_balanced("((abc))def") == seq("((abc))", "def"));
	QUARK_TEST_VERIFY(get_balanced("((abc)[])def") == seq("((abc)[])", "def"));
}

string trim_ends(const string& s){
	return s.substr(1, s.size() - 2);
}




//////////////////////////////////////////////////		data_type_t


/*
	returns "" on unknown type.
*/
data_type_t resolve_type(std::string node_type){
	if(node_type == "int"){
		return data_type_t::make_type("int");
	}
	else if(node_type == "string"){
		return data_type_t::make_type("string");
	}
	else if(node_type == "float"){
		return data_type_t::make_type("float");
	}
	else if(node_type == "function"){
		return data_type_t::make_type("function");
	}
	else if(node_type == "value_type"){
		return data_type_t::make_type("value_type");
	}
	else{
		return "";
	}
}





QUARK_UNIT_TEST("", "math_operation_t==()", "", ""){
	const auto a = make_math_operation(math_operation_t::add, make_constant(3), make_constant(4));
	const auto b = make_math_operation(math_operation_t::add, make_constant(3), make_constant(4));
	QUARK_TEST_VERIFY(a == b);
}




//////////////////////////////////////////////////		VISITOR




struct visitor_i {
	virtual ~visitor_i(){};

	virtual void visitor_interface__on_math_operation(const math_operation_t& e) = 0;
	virtual void visitor_interface__on_make_function_expression(const make_function_expression_t& e) = 0;

	virtual void visitor_interface__on_bind_statement_statement(const bind_statement_t& s) = 0;
	virtual void visitor_interface__on_return_statement(const return_statement_t& s) = 0;
};

void visit_statement(const statement_t& s, visitor_i& visitor){
	if(s._bind_statement){
		visitor.visitor_interface__on_bind_statement_statement(*s._bind_statement);
	}
	else if(s._return_statement){
		visitor.visitor_interface__on_return_statement(*s._return_statement);
	}
	else{
		QUARK_ASSERT(false);
	}
}


void visit_program(const ast_t& program, visitor_i& visitor){
	for(const auto i: program._top_level_statements){
		visit_statement(i, visitor);
	}
}



//////////////////////////////////////////////////		TRACE

//	### Instead of just tracing, generate + trace the parse tree as JSON for easy tooling and debugging.

void trace(const arg_t& arg);
void trace(const expression_t& e);
void trace(const statement_t& s);
void trace(const expression_t& e);
void trace(const function_body_t& body);
void trace(const data_type_t& v);
void trace(const value_t& e);
void trace(const make_function_expression_t& e);
void trace(const math_operation_t& e);
void trace(const math_operation1_t& e);
void trace(const call_function_t& e);

void trace(const ast_t& program);




string operation_to_string(const math_operation_t::operation& op){
	if(op == math_operation_t::add){
		return "add";
	}
	else if(op == math_operation_t::subtract){
		return "subtract";
	}
	else if(op == math_operation_t::multiply){
		return "multiply";
	}
	else if(op == math_operation_t::divide){
		return "divide";
	}
	else{
		QUARK_ASSERT(false);
	}
}

string operation_to_string(const math_operation1_t::operation& op){
	if(op == math_operation1_t::negate){
		return "negate";
	}
	else{
		QUARK_ASSERT(false);
	}
}


void trace(const arg_t& arg){
	QUARK_TRACE("<arg_t> data type: <" + arg._type.to_string() + "> indentifier: \"" + arg._identifier + "\"");
}

template<typename T> void trace_vec(const string& title, const vector<T>& v){
	QUARK_SCOPED_TRACE(title);
	for(auto i: v){
		trace(i);
	}
}

void trace(const expression_t& e);

void trace(const statement_t& s){
	if(s._bind_statement){
		const auto s2 = s._bind_statement;
		string t = "bind_statement_t: \"" + s2->_identifier + "\"";
		QUARK_SCOPED_TRACE(t);
		trace(s2->_expression);
	}
	else if(s._return_statement){
		const auto s2 = s._return_statement;
		QUARK_SCOPED_TRACE("return_statement_t");
		trace(s2->_expression);
	}
	else{
		QUARK_ASSERT(false);
	}
}

void trace(const expression_t& e){
	if(e._constant){
		trace(*e._constant);
	}
	else if(e._make_function_expression){
		const make_function_expression_t& temp = *e._make_function_expression;
		trace(temp);
	}
	else if(e._math_operation){
		trace(*e._math_operation);
	}
	else if(e._math_operation1){
		trace(*e._math_operation1);
	}
	else if(e._nop){
		QUARK_TRACE("NOP");
	}
	else if(e._call_function){
		trace(*e._call_function);
	}
	else{
		QUARK_ASSERT(false);
	}
}

void trace(const function_body_t& body){
	QUARK_SCOPED_TRACE("function_body_t");
	trace_vec("Statements:", body._statements);
}

void trace(const data_type_t& v){
	QUARK_TRACE("data_type_t <" + v.to_string() + ">");
}

void trace(const value_t& e){
	QUARK_TRACE("value_t: " + e.value_and_type_to_string());
}

void trace(const make_function_expression_t& e){
	QUARK_SCOPED_TRACE("make_function_expression_t");

	{
		QUARK_SCOPED_TRACE("return");
		trace(e._return_type);
	}
	{
		trace_vec("arguments", e._args);
	}
	{
		QUARK_SCOPED_TRACE("body");
		trace(e._body);
	}
}

void trace(const math_operation_t& e){
	string s = "math_operation_t: " + operation_to_string(e._operation);
	QUARK_SCOPED_TRACE(s);
	trace(*e._left);
	trace(*e._right);
}
void trace(const math_operation1_t& e){
	string s = "math_operation1_t: " + operation_to_string(e._operation);
	QUARK_SCOPED_TRACE(s);
	trace(*e._input);
}

void trace(const call_function_t& e){
	string s = "call_function_t: " + e._function_name;
	QUARK_SCOPED_TRACE(s);
	for(const auto i: e._inputs){
		trace(*i);
	}
}


void trace(const ast_t& program){
	QUARK_SCOPED_TRACE("program");

	for(const auto i: program._top_level_statements){
		trace(i);
	}
}



//////////////////////////////////////////////////		Syntax-specific reading

/*
	These functions knows about the Floyd syntax.
*/


bool is_string_valid_type(const string& s){
	return s == "int" || s == "bool" || s == "string" || s == "float";
}

pair<data_type_t, string> read_required_type(const string& s){
	const seq type_pos = get_type(s);
	if(!is_string_valid_type(type_pos.first)){
		throw std::runtime_error("expected type");
	}
	const auto type = resolve_type(type_pos.first);
	return { type, type_pos.second };
}

//	Get identifier (name of a defined function or constant variable name).
pair<string, string> read_required_identifier(const string& s){
	const seq type_pos = get_identifier(s);
	if(type_pos.first.empty()){
		throw std::runtime_error("missing identifier");
	}
	const string identifier = type_pos.first;
	return { identifier, type_pos.second };
}

/*
	()
	(int a)
	(int x, int y)
*/
vector<arg_t> parse_functiondef_arguments(const string& s2){
	const auto s(s2.substr(1, s2.length() - 2));
	vector<arg_t> args;
	auto str = s;
	while(!str.empty()){
		const auto arg_type = get_type(str);
		const auto arg_name = get_identifier(arg_type.second);
		const auto optional_comma = read_optional_char(arg_name.second, ',');

		const auto a = arg_t{ resolve_type(arg_type.first), arg_name.first };
		args.push_back(a);
		str = skip_whitespace(optional_comma.second);
	}

	trace_vec("parsed arguments:", args);
	return args;
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY((parse_functiondef_arguments("()") == vector<arg_t>{}));
}

QUARK_UNIT_TEST("", "", "", ""){
	//### include the ( and )!!!"
	const auto r = parse_functiondef_arguments("(int x, string y, float z)");
	QUARK_TEST_VERIFY((r == vector<arg_t>{
		{ resolve_type("int"), "x" },
		{ resolve_type("string"), "y" },
		{ resolve_type("float"), "z" }
	}
	));
}






pair<expression_t, string> parse_summands(const functions_t& functions, const string& s, int depth);


expression_t negate_expression(const expression_t& e){
	//	Shortcut: directly negate numeric constants.
	if(e._constant){
		const value_t& value = *e._constant;
		if(value.get_type() == resolve_type("int")){
			return make_constant(value_t{-value.get_int()});
		}
		else if(value.get_type() == resolve_type("float")){
			return make_constant(value_t{-value.get_float()});
		}
	}

	return make_math_operation1(math_operation1_t::negate, e);
}

float parse_float(const string& pos){
	size_t end = -1;
	auto res = std::stof(pos, &end);
	if(isnan(res) || end == 0){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
	return res;
}



/*
	### add lambda / local function
	### add member access: my_data.name.first_name

	Constant literal
		3
		3.0
		"three"
	Function call
		f ()
		f(g())
		f(a + "xyz")
		f(a + "xyz", 1000 * 3)
	Variable
		x1
		hello2
*/
pair<expression_t, string> parse_single_internal(const functions_t& functions, const string& s) {
	QUARK_ASSERT(functions.check_invariant());
	QUARK_ASSERT(s.size() > 0);

	string pos = s;

	//	" => string constant.
	if(peek_string(pos, "\"")){
		pos = pos.substr(1);
		const auto string_constant_pos = read_until(pos, "\"");

		pos = string_constant_pos.second;
		pos = pos.substr(1);
		return { value_t{ string_constant_pos.first }, pos };
	}

	// [0-9] and "."  => numeric constant.
	else if((number_chars + ".").find(pos[0]) != string::npos){
		const auto number_pos = read_while(pos, number_chars + ".");
		if(number_pos.first.empty()){
			throw std::runtime_error("EEE_WRONG_CHAR");
		}

		//	If it contains a "." its a float, else an int.
		if(number_pos.first.find('.') != string::npos){
			pos = pos.substr(number_pos.first.size());
			float value = parse_float(number_pos.first);
			return { value_t{value}, pos };
		}
		else{
			pos = pos.substr(number_pos.first.size());
			int value = atoi(number_pos.first.c_str());
			return { value_t{value}, pos };
		}
	}

	/*
		Start of identifier: can be variable access or function call.

		"hello"
		"f ()"
		"f(x + 10)"
	*/
	else if(identifier_chars.find(pos[0]) != string::npos){
		const auto identifier_pos = read_required_identifier(pos);


		if(functions._functions.find(identifier_pos.first) == functions._functions.end()){
			throw std::runtime_error("Unknown function \"" + identifier_pos.first + "\"");
		}

		string p2 = skip_whitespace(identifier_pos.second);

		//???	Lookup function!
		//	Function call.
		if(!p2.empty() && p2[0] == '('){
			const auto arg_list_pos = get_balanced(p2);
			const auto args = trim_ends(arg_list_pos.first);

			p2 = args;
			vector<shared_ptr<expression_t>> args_expressions;
			while(!p2.empty()){
				const auto p3 = read_until(skip_whitespace(p2), ",");
				expression_t arg_expre = parse_expression(functions, p3.first);
				args_expressions.push_back(make_shared<expression_t>(arg_expre));
				p2 = p3.second[0] == ',' ? p3.second.substr(1) : p3.second;
			}
			return { call_function_t{identifier_pos.first, args_expressions }, p2 };
		}

		//	Variable-read.
		else{
			QUARK_ASSERT(false);
		}
	}
	else{
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
}

pair<expression_t, string> parse_single(const functions_t& functions, const string& s){
	QUARK_ASSERT(functions.check_invariant());

	const auto result = parse_single_internal(functions, s);
	trace(result.first);
	return result;
}


shared_ptr<const make_function_expression_t> make_log(){
	vector<arg_t> args{ {resolve_type("float"), "value"} };
	function_body_t body{
		{
			make__return_statement(
				return_statement_t{ make_constant(value_t(123.f)) }
			)
		}
	};

	return make_shared<const make_function_expression_t>(make_function_expression_t{ resolve_type("float"), args, body });
}

shared_ptr<const make_function_expression_t> make_log2(){
	vector<arg_t> args{ {resolve_type("string"), "s"}, {resolve_type("float"), "v"} };
	function_body_t body{
		{
			make__return_statement(
				return_statement_t{ make_constant(value_t(456.7f)) }
			)
		}
	};

	return make_shared<const make_function_expression_t>(make_function_expression_t{ resolve_type("float"), args, body });
}


functions_t make_test_functions(){
	functions_t result;
	result._functions["log"] = make_log();
	result._functions["log2"] = make_log2();
	result._functions["f"] = make_log();
	return result;
}






QUARK_UNIT_TESTQ("parse_single", "number"){
	functions_t functions;
	QUARK_TEST_VERIFY((parse_single(functions, "9.0") == pair<expression_t, string>(value_t{ 9.0f }, "")));
}

QUARK_UNIT_TESTQ("parse_single", "function call"){
	const auto functions = make_test_functions();
	const auto a = parse_single(functions, "log(34.5)");
	QUARK_TEST_VERIFY(a.first._call_function->_function_name == "log");
	QUARK_TEST_VERIFY(a.first._call_function->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._call_function->_inputs[0]->_constant == value_t(34.5f));
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "function call with two args"){
	const auto functions = make_test_functions();
	const auto a = parse_single(functions, "log2(\"123\" + \"xyz\", 1000 * 3)");
	QUARK_TEST_VERIFY(a.first._call_function->_function_name == "log2");
	QUARK_TEST_VERIFY(a.first._call_function->_inputs.size() == 2);
	QUARK_TEST_VERIFY(a.first._call_function->_inputs[0]->_math_operation);
	QUARK_TEST_VERIFY(a.first._call_function->_inputs[1]->_math_operation);
	QUARK_TEST_VERIFY(a.second == "");
}

QUARK_UNIT_TESTQ("parse_single", "nested function calls"){
	const auto functions = make_test_functions();
	const auto a = parse_single(functions, "log2(2.1, f(3.14))");
	QUARK_TEST_VERIFY(a.first._call_function->_function_name == "log2");
	QUARK_TEST_VERIFY(a.first._call_function->_inputs.size() == 2);
	QUARK_TEST_VERIFY(a.first._call_function->_inputs[0]->_constant);
	QUARK_TEST_VERIFY(a.first._call_function->_inputs[1]->_call_function->_function_name == "f");
	QUARK_TEST_VERIFY(a.first._call_function->_inputs[1]->_call_function->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._call_function->_inputs[1]->_call_function->_inputs[0] == value_t(3.14f));
	QUARK_TEST_VERIFY(a.second == "");
}



// Parse a constant or an expression in parenthesis
pair<expression_t, string> parse_atom(const functions_t& functions, const string& s, int depth) {
	string pos = skip_whitespace(s);

	//	Handle the sign before parenthesis (or before number)
	auto negative = false;
	while(pos.size() > 0 && (is_whitespace(pos[0]) || (pos[0] == '-' || pos[0] == '+'))){
		if(pos[0] == '-') {
			negative = !negative;
		}
		pos = pos.substr(1);
	}

	// Check if there is parenthesis
	if(pos.size() > 0 && pos[0] == '(') {
		pos = pos.substr(1);

		auto res = parse_summands(functions, pos, depth);
		pos = skip_whitespace(res.second);
		if(!(res.second.size() > 0 && res.second[0] == ')')) {
			// Unmatched opening parenthesis
			throw std::runtime_error("EEE_PARENTHESIS");
		}
		pos = pos.substr(1);
		const auto expr = res.first;
		return { negative ? negate_expression(expr) : expr, pos };
	}

	//	Parse constant literal / function call / variable-access.
	if(pos.size() > 0){
		const auto single_pos = parse_single(functions, pos);
		return { negative ? negate_expression(single_pos.first) : single_pos.first, single_pos.second };
	}

	throw std::runtime_error("Expected number");
}

QUARK_UNIT_TEST("", "parse_atom", "", ""){
	functions_t functions;

	QUARK_TEST_VERIFY((parse_atom(functions, "0.0", 0) == pair<expression_t, string>(value_t{ 0.0f }, "")));
	QUARK_TEST_VERIFY((parse_atom(functions, "9.0", 0) == pair<expression_t, string>(value_t{ 9.0f }, "")));
	QUARK_TEST_VERIFY((parse_atom(functions, "12345.0", 0) == pair<expression_t, string>(value_t{ 12345.0f }, "")));

	QUARK_TEST_VERIFY((parse_atom(functions, "10.0", 0) == pair<expression_t, string>(value_t{ 10.0f }, "")));
	QUARK_TEST_VERIFY((parse_atom(functions, "-10.0", 0) == pair<expression_t, string>(value_t{ -10.0f }, "")));
	QUARK_TEST_VERIFY((parse_atom(functions, "+10.0", 0) == pair<expression_t, string>(value_t{ 10.0f }, "")));

	QUARK_TEST_VERIFY((parse_atom(functions, "4.0+", 0) == pair<expression_t, string>(value_t{ 4.0f }, "+")));


	QUARK_TEST_VERIFY((parse_atom(functions, "\"hello\"", 0) == pair<expression_t, string>(value_t{ "hello" }, "")));
}



pair<expression_t, string> parse_factors(const functions_t& functions, const string& s, int depth) {
	const auto num1_pos = parse_atom(functions, s, depth);
	auto result_expression = num1_pos.first;
	string pos = skip_whitespace(num1_pos.second);
	while(!pos.empty() && (pos[0] == '*' || pos[0] == '/')){
		const auto op_pos = read_char(pos);
		const auto expression2_pos = parse_atom(functions, op_pos.second, depth);
		if(op_pos.first == '/') {
			result_expression = make_math_operation(math_operation_t::divide, result_expression, expression2_pos.first);
		}
		else{
			result_expression = make_math_operation(math_operation_t::multiply, result_expression, expression2_pos.first);
		}
		pos = skip_whitespace(expression2_pos.second);
	}
	return { result_expression, pos };
}

pair<expression_t, string> parse_summands(const functions_t& functions, const string& s, int depth) {
	const auto num1_pos = parse_factors(functions, s, depth);
	auto result_expression = num1_pos.first;
	string pos = num1_pos.second;
	while(!pos.empty() && (pos[0] == '-' || pos[0] == '+')) {
		const auto op_pos = read_char(pos);

		const auto expression2_pos = parse_factors(functions, op_pos.second, depth);
		if(op_pos.first == '-'){
			result_expression = make_math_operation(math_operation_t::subtract, result_expression, expression2_pos.first);
		}
		else{
			result_expression = make_math_operation(math_operation_t::add, result_expression, expression2_pos.first);
		}

		pos = skip_whitespace(expression2_pos.second);
	}
	return { result_expression, pos };
}





/*
	### Add variable as sources (arguments and locals and globals).


	Example input:
		0
		3
		(3)
		(1 + 2) * 3
		"test"
		"test number: " +

		x
		x + y

		f()
		f(10, 122)

		(my_fun1("hello, 3) + 4) * my_fun2(10))
*/
expression_t parse_expression(const functions_t& functions, string expression){
	if(expression.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}
	auto result = parse_summands(functions, expression, 0);
	if(!result.second.empty()){
		throw std::runtime_error("EEE_WRONG_CHAR");
	}

	trace("Expression: \"" + expression + "\"");
	trace(result.first);
	return result.first;
}









value_t run_function(const functions_t& functions, const make_function_expression_t& f, const vector<value_t>& args){
	const auto body = f._body;

	std::map<string, value_t> locals;
	int statement_index = 0;
	while(statement_index < body._statements.size()){
		const auto statement = body._statements[statement_index];
		if(statement._bind_statement){
			const auto s = statement._bind_statement;
			const auto name = s->_identifier;
			if(locals.count(name) != 0){
				throw std::runtime_error("local constant already exists");
			}
			const auto result = evaluate3(functions, s->_expression);
			if(!result._constant){
				throw std::runtime_error("unknown variables");
			}
			locals[name] = *result._constant;
		}
		else if(statement._return_statement){
			const auto expr = statement._return_statement->_expression;
			const auto result = evaluate3(functions, expr);

			if(!result._constant){
				throw std::runtime_error("undefined");
			}

			return *result._constant;
		}
		else{
			QUARK_ASSERT(false);
		}
		statement_index++;
	}
	throw std::runtime_error("function missing return statement");
}



bool compare_float_approx(float value, float expected){
	float diff = static_cast<float>(fabs(value - expected));
	return diff < 0.00001;
}

//### Test string + etc.


expression_t evaluate3(const functions_t& functions, const expression_t& e){
	QUARK_ASSERT(functions.check_invariant());
	QUARK_ASSERT(e.check_invariant());

	if(e._constant){
		return e;
	}
	else if(e._math_operation){
		const auto e2 = *e._math_operation;
		const auto left = evaluate3(functions, *e2._left);
		const auto right = evaluate3(functions, *e2._right);

		//	Both left and right are constant, replace the math_operation with a constant!
		if(left._constant && right._constant){
			const auto left_value = left._constant;
			const auto right_value = right._constant;
			if(left_value->get_type() == resolve_type("int") && right_value->get_type() == resolve_type("int")){
				if(e2._operation == math_operation_t::add){
					return make_constant(left_value->get_int() + right_value->get_int());
				}
				else if(e2._operation == math_operation_t::subtract){
					return make_constant(left_value->get_int() - right_value->get_int());
				}
				else if(e2._operation == math_operation_t::multiply){
					return make_constant(left_value->get_int() * right_value->get_int());
				}
				else if(e2._operation == math_operation_t::divide){
					if(right_value->get_int() == 0){
						throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
					}
					return make_constant(left_value->get_int() / right_value->get_int());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(left_value->get_type() == resolve_type("float") && right_value->get_type() == resolve_type("float")){
				if(e2._operation == math_operation_t::add){
					return make_constant(left_value->get_float() + right_value->get_float());
				}
				else if(e2._operation == math_operation_t::subtract){
					return make_constant(left_value->get_float() - right_value->get_float());
				}
				else if(e2._operation == math_operation_t::multiply){
					return make_constant(left_value->get_float() * right_value->get_float());
				}
				else if(e2._operation == math_operation_t::divide){
					if(right_value->get_float() == 0.0f){
						throw std::runtime_error("EEE_DIVIDE_BY_ZERO");
					}
					return make_constant(left_value->get_float() / right_value->get_float());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(left_value->get_type() == resolve_type("string") && right_value->get_type() == resolve_type("string")){
				if(e2._operation == math_operation_t::add){
					return make_constant(value_t(left_value->get_string() + right_value->get_string()));
				}
				else{
					throw std::runtime_error("Arithmetics failed.");
				}
			}
			else{
				throw std::runtime_error("Arithmetics failed.");
			}
		}

		//	Else use a math_operation to make the calculation later. We make a NEW math_operation since sub-nodes may have been evaluated.
		else{
			return make_math_operation(e2._operation, left, right);
		}
	}
	else if(e._math_operation1){
		const auto e2 = *e._math_operation1;
		const auto input = evaluate3(functions, *e2._input);

		//	Replace the with a constant!
		if(input._constant){
			const auto value = input._constant;
			if(value->get_type() == resolve_type("int")){
				if(e2._operation == math_operation1_t::negate){
					return make_constant(-value->get_int());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(value->get_type() == resolve_type("float")){
				if(e2._operation == math_operation1_t::negate){
					return make_constant(-value->get_float());
				}
				else{
					QUARK_ASSERT(false);
				}
			}
			else if(value->get_type() == resolve_type("string")){
				throw std::runtime_error("Arithmetics failed.");
			}
			else{
				throw std::runtime_error("Arithmetics failed.");
			}
		}

		//	Else use a math_operation to make the calculation later. We make a NEW math_operation since sub-nodes may have been evaluated.
		else{
			return make_math_operation1(e2._operation, input);
		}
	}

	/*
		If inputs are constant, replace function call with a constant!
	*/
	else if(e._call_function){
		const auto& call_function_expression = *e._call_function;

		const auto it = functions._functions.find(call_function_expression._function_name);
		QUARK_ASSERT(it != functions._functions.end());

		const auto& make_function_expression = *it->second;

		//??? Validate function call argument count when parsing!
		QUARK_ASSERT(make_function_expression._args.size() == call_function_expression._inputs.size());

		//	Simplify each argument.
		vector<expression_t> simplified_args;
		for(const auto& i: call_function_expression._inputs){
			const auto arg_expr = evaluate3(functions, *i);
			simplified_args.push_back(arg_expr);
		}

		//	All arguments to functions are constants? Else return new call_function, but with simplified arguments.
		for(const auto& i: simplified_args){
			if(!i._constant){
				return { call_function_t{ call_function_expression._function_name, call_function_expression._inputs } };
			}
		}

		//	WOha: all arguments were constants - replace this expression with result of function call instead!
		vector<value_t> constant_args;
		for(const auto& i: simplified_args){
			constant_args.push_back(*i._constant);
			if(!i._constant){
				return { call_function_t{ call_function_expression._function_name, call_function_expression._inputs } };
			}
		}
		const value_t result = run_function(functions, make_function_expression, constant_args);
		return make_constant(result);
	}
	else if(e._nop){
		return e;
	}
	else{
		QUARK_ASSERT(false);
	}
}


namespace {
	expression_t test_evaluate_simple(string expression_string){
		functions_t functions;
		const auto e = parse_expression(functions, expression_string);
		const auto e2 = evaluate3(functions, e);
		return e2;
	}
}


//??? Add tests for strings and floats.
QUARK_UNIT_TEST("", "evaluate()", "", "") {
	// Some simple expressions
	QUARK_TEST_VERIFY(test_evaluate_simple("1234") == make_constant(1234));
	QUARK_TEST_VERIFY(test_evaluate_simple("1+2") == make_constant(3));
	QUARK_TEST_VERIFY(test_evaluate_simple("1+2+3") == make_constant(6));
	QUARK_TEST_VERIFY(test_evaluate_simple("3*4") == make_constant(12));
	QUARK_TEST_VERIFY(test_evaluate_simple("3*4*5") == make_constant(60));
	QUARK_TEST_VERIFY(test_evaluate_simple("1+2*3") == make_constant(7));

	// Parenthesis
	QUARK_TEST_VERIFY(test_evaluate_simple("5*(4+4+1)") == make_constant(45));
	QUARK_TEST_VERIFY(test_evaluate_simple("5*(2*(1+3)+1)") == make_constant(45));
	QUARK_TEST_VERIFY(test_evaluate_simple("5*((1+3)*2+1)") == make_constant(45));

	// Spaces
	QUARK_TEST_VERIFY(test_evaluate_simple(" 5 * ((1 + 3) * 2 + 1) ") == make_constant(45));
	QUARK_TEST_VERIFY(test_evaluate_simple(" 5 - 2 * ( 3 ) ") == make_constant(-1));
	QUARK_TEST_VERIFY(test_evaluate_simple(" 5 - 2 * ( ( 4 )  - 1 ) ") == make_constant(-1));

	// Sign before parenthesis
	QUARK_TEST_VERIFY(test_evaluate_simple("-(2+1)*4") == make_constant(-12));
	QUARK_TEST_VERIFY(test_evaluate_simple("-4*(2+1)") == make_constant(-12));
	
	// Fractional numbers
	QUARK_TEST_VERIFY(compare_float_approx(test_evaluate_simple("5.5/5.0")._constant->get_float(), 1.1f));
//	QUARK_TEST_VERIFY(test_evaluate_simple("1/5e10") == 2e-11);
	QUARK_TEST_VERIFY(compare_float_approx(test_evaluate_simple("(4.0-3.0)/(4.0*4.0)")._constant->get_float(), 0.0625f));
	QUARK_TEST_VERIFY(test_evaluate_simple("1.0/2.0/2.0") == make_constant(0.25f));
	QUARK_TEST_VERIFY(test_evaluate_simple("0.25 * .5 * 0.5") == make_constant(0.0625f));
	QUARK_TEST_VERIFY(test_evaluate_simple(".25 / 2.0 * .5") == make_constant(0.0625f));
	
	// Repeated operators
	QUARK_TEST_VERIFY(test_evaluate_simple("1+-2") == make_constant(-1));
	QUARK_TEST_VERIFY(test_evaluate_simple("--2") == make_constant(2));
	QUARK_TEST_VERIFY(test_evaluate_simple("2---2") == make_constant(0));
	QUARK_TEST_VERIFY(test_evaluate_simple("2-+-2") == make_constant(4));


	// === Errors ===

	if(true){
		//////////////////////////		Parenthesis error
		try{
			test_evaluate_simple("5*((1+3)*2+1");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_PARENTHESIS");
		}

		try{
			test_evaluate_simple("5*((1+3)*2)+1)");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}


		//////////////////////////		Repeated operators (wrong)
		try{
			test_evaluate_simple("5*/2");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}

		//////////////////////////		Wrong position of an operator
		try{
			test_evaluate_simple("*2");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}

		try{
			test_evaluate_simple("2+");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "Expected number");
		}

		try{
			test_evaluate_simple("2*");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "Expected number");
		}

		//////////////////////////		Division by zero

		try{
			test_evaluate_simple("2/0");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
		}

		try{
			test_evaluate_simple("3+1/(5-5)+4");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_DIVIDE_BY_ZERO");
		}

		try{
			test_evaluate_simple("2/");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "Expected number");
		}

		//////////////////////////		Invalid characters
		try{
			test_evaluate_simple("~5");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}
		try{
			test_evaluate_simple("5x");
			QUARK_TEST_VERIFY(false);
		}
		catch(const std::runtime_error& e){
			QUARK_TEST_VERIFY(string(e.what()) == "EEE_WRONG_CHAR");
		}


		//////////////////////////		Multiply errors
			//	Multiple errors not possible/relevant now that we use exceptions for errors.
	/*
		//////////////////////////		Only one error will be detected (in this case, the last one)
		QUARK_TEST_VERIFY(test_evaluate_simple("3+1/0+4$") == EEE_WRONG_CHAR);

		QUARK_TEST_VERIFY(test_evaluate_simple("3+1/0+4") == EEE_DIVIDE_BY_ZERO);

		// ...or the first one
		QUARK_TEST_VERIFY(test_evaluate_simple("q+1/0)") == EEE_WRONG_CHAR);
		QUARK_TEST_VERIFY(test_evaluate_simple("+1/0)") == EEE_PARENTHESIS);
		QUARK_TEST_VERIFY(test_evaluate_simple("+1/0") == EEE_DIVIDE_BY_ZERO);
	*/

		// An emtpy string
		try{
			test_evaluate_simple("");
			QUARK_TEST_VERIFY(false);
		}
		catch(...){
//			QUARK_TEST_VERIFY(error == "EEE_WRONG_CHAR");
		}
	}
}











/*
	"int a = 10;"
	"float b = 0.3;"
	"int c = a + b;"
	"int b = f(a);"
	"string hello = f(a) + \"_suffix\";";

	...can contain trailing whitespace.
*/
pair<statement_t, string> parse_assignment_statement(const functions_t& functions, const string& s){
	QUARK_SCOPED_TRACE("parse_assignment_statement()");
	QUARK_ASSERT(functions.check_invariant());

	const auto token_pos = read_until(s, whitespace_chars);
	const auto type = resolve_type(token_pos.first);
	QUARK_ASSERT(resolve_type(read_until(s, whitespace_chars).first) != "");

	const auto variable_pos = read_until(skip_whitespace(token_pos.second), whitespace_chars + "=");
	const auto equal_rest = read_required_char(skip_whitespace(variable_pos.second), '=');
	const auto expression_pos = read_until(skip_whitespace(equal_rest), ";");

	const auto expression = parse_expression(functions, expression_pos.first);

	const auto statement = make__bind_statement(variable_pos.first, expression);
	trace(statement);

	//	Skip trailing ";".
	return { statement, expression_pos.second.substr(1) };
}

QUARK_UNIT_TESTQ("parse_assignment_statement", "int"){
	const auto a = parse_assignment_statement(functions_t(), "int a = 10; \n");
	QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "a");
	QUARK_TEST_VERIFY(*a.first._bind_statement->_expression._constant == value_t(10));
	QUARK_TEST_VERIFY(a.second == " \n");
}

QUARK_UNIT_TESTQ("parse_assignment_statement", "float"){
	const auto a = parse_assignment_statement(functions_t(), "float b = 0.3; \n");
	QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "b");
	QUARK_TEST_VERIFY(*a.first._bind_statement->_expression._constant == value_t(0.3f));
	QUARK_TEST_VERIFY(a.second == " \n");
}

QUARK_UNIT_TESTQ("parse_assignment_statement", "function call"){
	const auto functions = make_test_functions();
	const auto a = parse_assignment_statement(functions, "float test = log(\"hello\");\n");
	QUARK_TEST_VERIFY(a.first._bind_statement->_identifier == "test");
	QUARK_TEST_VERIFY(a.first._bind_statement->_expression._call_function->_function_name == "log");
	QUARK_TEST_VERIFY(a.first._bind_statement->_expression._call_function->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a.first._bind_statement->_expression._call_function->_inputs[0]->_constant ==value_t("hello"));
	QUARK_TEST_VERIFY(a.second == "\n");
}



/*
	### Split-out parse_statement().
	### Add struct {}
	### Add variables
	### Add local functions

	Must not have whitespace before / after {}.
	{}

	{
		return 3;
	}

	{
		return 3 + 4;
	}
	{
		return f(3, 4) + 2;
	}


	//	Example: binding constants to constants, result of function calls and math operations.
	{
		int a = 10;
		int b = f(a);
		int c = a + b;
		return c;
	}

	//	Local scope.
	{
		{
			int a = 10;
		}
	}
	{
		struct point2d {
			int _x;
			int _y;
		}
	}

	{
		int my_func(string a, string b){
			int c = a + b;
			return c;
		}
	}
*/
function_body_t parse_function_body(const functions_t& functions, const string& s){
	QUARK_SCOPED_TRACE("parse_function_body()");
	QUARK_ASSERT(s.size() >= 2);
	QUARK_ASSERT(s[0] == '{' && s[s.size() - 1] == '}');

	const string body_str = skip_whitespace(s.substr(1, s.size() - 2));

	vector<statement_t> statements;

	string pos = body_str;
	while(!pos.empty()){
		const auto token_pos = read_until(pos, whitespace_chars);
		if(token_pos.first == "return"){
			const auto expression_pos = read_until(skip_whitespace(token_pos.second), ";");
			const auto expression = parse_expression(functions, expression_pos.first);
			const auto statement = statement_t(return_statement_t{ expression });
			statements.push_back(statement);

			//	Skip trailing ";".
			pos = skip_whitespace(expression_pos.second.substr(1));
		}

		/*
			"int a = 10;"
			"string hello = f(a) + \"_suffix\";";
		*/
		else if(resolve_type(token_pos.first) != ""){
			pair<statement_t, string> assignment_statement = parse_assignment_statement(functions, pos);
			statements.push_back(assignment_statement.first);

			//	Skips trailing ";".
			pos = skip_whitespace(assignment_statement.second);
		}
		else{
			throw std::runtime_error("syntax error");
		}
	}
	const auto result = function_body_t{ statements };
	trace(result);
	return result;
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY((parse_function_body({}, "{}")._statements.empty()));
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY(parse_function_body({}, "{return 3;}")._statements.size() == 1);
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY(parse_function_body({}, "{\n\treturn 3;\n}")._statements.size() == 1);
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto functions = make_test_functions();
	const auto a = parse_function_body(functions,
		"{	float test = log(10.11);\n"
		"	return 3;\n}"
	);
	QUARK_TEST_VERIFY(a._statements.size() == 2);
	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_identifier == "test");
	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_expression._call_function->_function_name == "log");
	QUARK_TEST_VERIFY(a._statements[0]._bind_statement->_expression._call_function->_inputs.size() == 1);
	QUARK_TEST_VERIFY(*a._statements[0]._bind_statement->_expression._call_function->_inputs[0]->_constant == value_t(10.11f));

	QUARK_TEST_VERIFY(*a._statements[1]._return_statement->_expression._constant == value_t(3));
}



/*
	Just defines an unnamed function - it has no name.

	Named function:

	int myfunc(string a, int b){
		...
		return b + 1;
	}


	LATER:
	Lambda:

	int myfunc(string a){
		() => {
		}
	}
*/
pair<pair<string, make_function_expression_t>, string> read_function(const functions_t& functions, const string& pos){
	QUARK_ASSERT(functions.check_invariant());

	const auto type_pos = read_required_type(pos);
	const auto identifier_pos = read_required_identifier(type_pos.second);

	//	Skip whitespace.
	const auto rest = skip_whitespace(identifier_pos.second);

	if(!peek_compare_char(rest, '(')){
		throw std::runtime_error("expected function argument list enclosed by (),");
	}

	//### create pair<string, string> get_required_balanced(string, '(', ')') that returns empty string if not found.

	const auto arg_list_pos = get_balanced(rest);
	const auto args = parse_functiondef_arguments(arg_list_pos.first);
	const auto body_rest_pos = skip_whitespace(arg_list_pos.second);

	if(!peek_compare_char(body_rest_pos, '{')){
		throw std::runtime_error("expected function body enclosed by {}.");
	}
	const auto body_pos = get_balanced(body_rest_pos);
	const auto body = parse_function_body(functions, body_pos.first);
	const auto a = make_function_expression_t{ type_pos.first, args, body };
//	trace_node(a);

	return { { identifier_pos.first, a }, body_pos.second };
}

QUARK_UNIT_TEST("", "read_function()", "", ""){
	try{
		const auto result = read_function({}, "int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TEST("", "read_function()", "", ""){
	const auto result = read_function({}, "int f(){}");
	QUARK_TEST_VERIFY(result.first.first == "f");
	QUARK_TEST_VERIFY(result.first.second._return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY(result.first.second._args.empty());
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}

QUARK_UNIT_TEST("", "read_function()", "Test many arguments of different types", ""){
	const auto result = read_function({}, "int printf(string a, float barry, int c){}");
	QUARK_TEST_VERIFY(result.first.first == "printf");
	QUARK_TEST_VERIFY(result.first.second._return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY((result.first.second._args == vector<arg_t>{
		{ resolve_type("string"), "a" },
		{ resolve_type("float"), "barry" },
		{ resolve_type("int"), "c" },
	}));
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}

/*
QUARK_UNIT_TEST("", "read_function()", "Test exteme whitespaces", ""){
	const auto result = read_function("    int    printf   (   string    a   ,   float   barry  ,   int   c  )  {  }  ");
	QUARK_TEST_VERIFY(result.first.first == "printf");
	QUARK_TEST_VERIFY(result.first.second._return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY((result.first.second._args == vector<arg_t>{
		{ resolve_type("string"), "a" },
		{ resolve_type("float"), "barry" },
		{ resolve_type("int"), "c" },
	}));
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}
*/





//////////////////////////////////////////////////		read_toplevel_statement()


/*
	??? Only supports functions right now.

	Function definitions
		int my_func(){ return 100; };
		string test3(int a, float b){ return "sdf" };

	Assign global constants
		int my_global = 3;

	Define data structures
*/

pair<statement_t, string> read_toplevel_statement(const functions_t& functions, const string& pos){
	QUARK_ASSERT(functions.check_invariant());

	const auto type_pos = read_required_type(pos);
	const auto identifier_pos = read_required_identifier(type_pos.second);

	pair<pair<string, make_function_expression_t>, string> function = read_function(functions, pos);
	const auto bind = bind_statement_t{ function.first.first, function.first.second };
	return { bind, function.second };
}

QUARK_UNIT_TEST("", "read_toplevel_statement()", "", ""){
	try{
		const auto result = read_toplevel_statement({}, "int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

QUARK_UNIT_TEST("", "read_toplevel_statement()", "", ""){
	const auto result = read_toplevel_statement({}, "int f(){}");
	QUARK_TEST_VERIFY(result.first._bind_statement);
	QUARK_TEST_VERIFY(result.first._bind_statement->_identifier == "f");
	QUARK_TEST_VERIFY(result.first._bind_statement->_expression._make_function_expression);

	const auto make_function_expression = result.first._bind_statement->_expression._make_function_expression;
	QUARK_TEST_VERIFY(make_function_expression->_return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY(make_function_expression->_args.empty());
	QUARK_TEST_VERIFY(make_function_expression->_body._statements.empty());

	QUARK_TEST_VERIFY(result.second == "");
}


//////////////////////////////////////////////////		program_to_ast()



ast_t program_to_ast(const string& program){
	vector<statement_t> top_level_statements;
	functions_t functions;
	auto pos = program;
	pos = skip_whitespace(pos);
	while(!pos.empty()){
		const auto statement_pos = read_toplevel_statement(functions, pos);
		top_level_statements.push_back(statement_pos.first);

		if(statement_pos.first._bind_statement){
			string identifier = statement_pos.first._bind_statement->_identifier;
			const auto e = statement_pos.first._bind_statement->_expression;

			if(e._make_function_expression){
				const auto foundIt = functions._functions.find(identifier);
				if(foundIt != functions._functions.end()){
					throw std::runtime_error("Function \"" + identifier + "\" already defined.");
				}

				//	shared_ptr
				functions._functions[identifier] = e._make_function_expression;
			}
		}
		else{
			QUARK_ASSERT(false);
		}
		pos = skip_whitespace(statement_pos.second);
	}

	const ast_t result{ functions, top_level_statements };
	trace(result);

	return result;
}

QUARK_UNIT_TEST("", "program_to_ast()", "kProgram1", ""){
	const string kProgram1 =
	"int main(string args){\n"
	"	return 3;\n"
	"}\n";

	const auto result = program_to_ast(kProgram1);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 1);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement->_identifier == "main");
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement->_expression._make_function_expression);

	const auto make_function_expression = result._top_level_statements[0]._bind_statement->_expression._make_function_expression;
	QUARK_TEST_VERIFY(make_function_expression->_return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY((make_function_expression->_args == vector<arg_t>{ arg_t{ data_type_t::make_type("string"), "args" }}));
}


QUARK_UNIT_TEST("", "program_to_ast()", "three arguments", ""){
	const string kProgram =
	"int f(int x, int y, string z){\n"
	"	return 3;\n"
	"}\n"
	;

	const auto result = program_to_ast(kProgram);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 1);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement->_identifier == "f");
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement->_expression._make_function_expression);

	const auto make_function_expression = result._top_level_statements[0]._bind_statement->_expression._make_function_expression;
	QUARK_TEST_VERIFY(make_function_expression->_return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY((make_function_expression->_args == vector<arg_t>{
		arg_t{ data_type_t::make_type("int"), "x" },
		arg_t{ data_type_t::make_type("int"), "y" },
		arg_t{ data_type_t::make_type("string"), "z" }
	}));
}


QUARK_UNIT_TEST("", "program_to_ast()", "two functions", ""){
	const string kProgram =
	"string hello(int x, int y, string z){\n"
	"	return \"test abc\";\n"
	"}\n"
	"int main(string args){\n"
	"	return 3;\n"
	"}\n"
	;
	QUARK_TRACE(kProgram);

	const auto result = program_to_ast(kProgram);
	QUARK_TEST_VERIFY(result._top_level_statements.size() == 2);

	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement->_identifier == "hello");
	QUARK_TEST_VERIFY(result._top_level_statements[0]._bind_statement->_expression._make_function_expression);

	const auto hello = result._top_level_statements[0]._bind_statement->_expression._make_function_expression;
	QUARK_TEST_VERIFY(hello->_return_type == data_type_t::make_type("string"));
	QUARK_TEST_VERIFY((hello->_args == vector<arg_t>{
		arg_t{ data_type_t::make_type("int"), "x" },
		arg_t{ data_type_t::make_type("int"), "y" },
		arg_t{ data_type_t::make_type("string"), "z" }
	}));


	QUARK_TEST_VERIFY(result._top_level_statements[1]._bind_statement);
	QUARK_TEST_VERIFY(result._top_level_statements[1]._bind_statement->_identifier == "main");
	QUARK_TEST_VERIFY(result._top_level_statements[1]._bind_statement->_expression._make_function_expression);

	const auto main = result._top_level_statements[1]._bind_statement->_expression._make_function_expression;
	QUARK_TEST_VERIFY(main->_return_type == data_type_t::make_type("int"));
	QUARK_TEST_VERIFY((main->_args == vector<arg_t>{
		arg_t{ data_type_t::make_type("string"), "args" }
	}));

}




/*
	"string hello(int x, int y, string z){\n"
	"	return \"test abc\";\n"
	"}\n"
	"int main(string args){\n"
	"	return 3;\n"
	"}\n"




	string hello(int x, int y, string z){
		return "test abc";
	}

	int main(string args){
		return 3;
	}


{
	"program_as_json" : [
		{
			"_func" : {
				"_prototype" : "string hello(int x, int y, string z)",
				"_body" : "{ return \"test abc\"; }"
			}
		},
		{
			"_func" : {
				"_prototype" : "int main(string args)",
				"_body" : "{ return 3; }"
			}
		}
	]
}



{
	"program_as_json" : [
		{
			"_func" : {
				"return": "string",
				"name": "hello",
				"args": [
					{ "type": "int", "name": "x" },
					{ "type": "int", "name": "y" },
					{ "type": "string", "name": "z" },
				],
				"body": [
					[ "assign", "a",
						[
							[ "return",
								[ "*", "a", "y" ]
							]
						],
					[ "return", "test abc" ]
				]
				"_body" : "{ return \"test abc\"; }"
			}
		},
		{
			"_func" : {
				"_prototype" : "int main(string args)",
				"_body" : "{ return 3; }"
			}
		}
	]
}

!!! JSON does not support multi-line strings.

*/






QUARK_UNIT_TESTQ("program_to_ast()", ""){
	const string kProgram2 =
	"float testx(float v){\n"
	"	return 13.4;\n"
	"}\n"
	"int main(string args){\n"
	"	float test = testx(1234);\n"
	"	return 3;\n"
	"}\n";
	auto r = program_to_ast(kProgram2);
	QUARK_TEST_VERIFY(r._top_level_statements.size() == 2);
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement);
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement->_identifier == "testx");
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement->_expression._make_function_expression->_return_type == resolve_type("float"));
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement->_expression._make_function_expression->_args.size() == 1);
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement->_expression._make_function_expression->_args[0]._type == resolve_type("float"));
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement->_expression._make_function_expression->_args[0]._identifier == "v");
	QUARK_TEST_VERIFY(r._top_level_statements[0]._bind_statement->_expression._make_function_expression->_body._statements.size() == 1);

	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement);
	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement->_identifier == "main");
	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement->_expression._make_function_expression->_return_type == resolve_type("int"));
	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement->_expression._make_function_expression->_args.size() == 1);
	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement->_expression._make_function_expression->_args[0]._type == resolve_type("string"));
	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement->_expression._make_function_expression->_args[0]._identifier == "args");
	QUARK_TEST_VERIFY(r._top_level_statements[1]._bind_statement->_expression._make_function_expression->_body._statements.size() == 2);
	//### Test body?
}




#if false
QUARK_UNIT_TEST("", "run()", "", ""){
	auto ast = program_to_ast(
		"int main(string args){\n"
		"	return \"hello, world!\";\n"
		"}\n"
	);

	value_t result = run(ast);
	QUARK_TEST_VERIFY(result == value_t("hello, world!"));
}
#endif

