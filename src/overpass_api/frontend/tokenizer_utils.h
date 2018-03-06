/** Copyright 2008, 2009, 2010, 2011, 2012 Roland Olbricht
*
* This file is part of Overpass_API.
*
* Overpass_API is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Overpass_API is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Overpass_API.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef DE__OSM3S___OVERPASS_API__FRONTEND__TOKENIZER_UTILS_H
#define DE__OSM3S___OVERPASS_API__FRONTEND__TOKENIZER_UTILS_H

#include "decode_text.h"
#include "../core/datatypes.h"
#include "../../expat/map_ql_input.h"


#include <string>


std::string get_text_token(Tokenizer_Wrapper& token, Error_Output* error_output,
		      std::string type_of_token);
std::string get_identifier_token(Tokenizer_Wrapper& token, Error_Output* error_output,
		      std::string type_of_token);

void process_after(Tokenizer_Wrapper& token, Error_Output* error_output, bool after);

void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, bool after = true);

void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, std::string target_2, bool after = true);

void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, std::string target_2, std::string target_3, bool after = true);

void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, std::string target_2, std::string target_3, std::string target_4,
		       bool after = true);

void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, std::string target_2, std::string target_3, std::string target_4,
		       std::string target_5, bool after = true);


struct Token_Node
{
  Token_Node(const std::string& token_, std::pair< uint, uint > line_col_)
      : token(token_), line_col(line_col_), lhs(0), rhs(0) {}

  std::string token;
  std::pair< uint, uint > line_col;
  uint lhs;
  uint rhs;
};


int operator_priority(const std::string& operator_name, bool unary);


struct Token_Tree
{
  Token_Tree(Tokenizer_Wrapper& token, Error_Output* error_output, bool parenthesis_expected);

  std::vector< Token_Node > tree;
};


struct Token_Node_Ptr
{
  Token_Node_Ptr(const Token_Tree& tree_, uint pos_ = 0) : tree(&tree_), pos(pos_) {}

  const Token_Node& operator*() const { return tree->tree[pos]; }
  const Token_Node* operator->() const { return &tree->tree[pos]; }
  Token_Node_Ptr lhs() const { return Token_Node_Ptr(*tree, tree->tree[pos].lhs); }
  Token_Node_Ptr rhs() const { return Token_Node_Ptr(*tree, tree->tree[pos].rhs); }

  const std::string* function_name() const;
  bool assert_is_function(Error_Output* error_output) const;
  bool assert_has_input_set(Error_Output* error_output, bool expected) const;
  bool assert_has_arguments(Error_Output* error_output, bool expected) const;

private:
  const Token_Tree* tree;
  uint pos;
};


#endif
