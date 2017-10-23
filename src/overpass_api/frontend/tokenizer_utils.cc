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


#include "tokenizer_utils.h"


void decode_to_utf8(const std::string& input, std::string& result,
                    std::string::size_type& from, std::string::size_type& to,
                    Error_Output* error_output)
{
  std::string::size_type limit = from + 4;
  if (result.size() < limit)
    limit = result.size();
  
  uint val = 0;
  while (from < limit)
  {
    val *= 16;
    if (input[from] <= '9' && input[from] >= '0')
      val += (input[from] - '0');
    else if (input[from] >= 'a' && input[from] <= 'f')
      val += (input[from] - ('a' - 10));
    else if (input[from] >= 'A' && input[from] <= 'F')
      val += (input[from] - ('A' - 10));
    else
      break;
    ++from;
  }
  if (val < 0x20)
  {
    if (error_output)
      error_output->add_parse_error("Invalid UTF-8 character (value below 32) in escape sequence.", 0);
  }
  else if (val < 0x80)
    result[to++] = val;
  else if (val < 0x800)
  {
    result[to++] = 0xc0 | (val>>6);
    result[to++] = 0x80 | (val & 0x3f);
  }
  else
  {
    result[to++] = 0xe0 | (val>>12);
    result[to++] = 0x80 | ((val>>6) & 0x3f);
    result[to++] = 0x80 | (val & 0x3f);
  }
}


std::string decode_json(const std::string& input, Error_Output* error_output)
{
  if (input[0] != '\"' && input[0] != '\'')
    return input;
  
  std::string::size_type j = 0;
  std::string result(input.size(), '\x0');
  for (std::string::size_type i = 1; i < input.size()-1; ++i)
  {
    if (input[i] == '\\')
    {
      ++i;
      if (input[i] == 'n')
        result[j++] = '\n';
      else if (input[i] == 't')
        result[j++] = '\t';
      else if (input[i] == 'u')
      {
        decode_to_utf8(input, result, ++i, j, error_output);
        --i;
      }
      else
        result[j++] = input[i];
    }
    else
      result[j++] = input[i];
  }
  result.resize(j);
    
  return result;
}


std::string decode_to_utf8(const std::string& token, std::string::size_type& pos, Error_Output* error_output)
{
  uint val = 0;
  pos += 2;
  std::string::size_type max_pos = pos + 4;
  if (token.size() < max_pos)
    max_pos = token.size();
  while (pos < max_pos &&
      ((token[pos] >= '0' && token[pos] <= '9')
      || (token[pos] >= 'a' && token[pos] <= 'f')
      || (token[pos] >= 'A' && token[pos] <= 'F')))
  {
    val *= 16;
    if (token[pos] >= '0' && token[pos] <= '9')
      val += (token[pos] - 0x30);
    else if (token[pos] >= 'a' && token[pos] <= 'f')
      val += (token[pos] - 87);
    else if (token[pos] >= 'A' && token[pos] <= 'F')
      val += (token[pos] - 55);
    ++pos;
  }
  if (val < 0x20)
  {
    if (error_output)
      error_output->add_parse_error("Invalid UTF-8 character (value below 32) in escape sequence.", 0);
  }
  else if (val < 0x80)
  {
    std::string buf = " ";
    buf[0] = val;
    return buf;
  }
  else if (val < 0x800)
  {
    std::string buf = "  ";
    buf[0] = (0xc0 | (val>>6));
    buf[1] = (0x80 | (val & 0x3f));
    return buf;
  }
  else
  {
    std::string buf = "   ";
    buf[0] = (0xe0 | (val>>12));
    buf[1] = (0x80 | ((val>>6) & 0x3f));
    buf[2] = (0x80 | (val & 0x3f));
    return buf;
  }
  return "";
}


std::string get_text_token(Tokenizer_Wrapper& token, Error_Output* error_output,
		      std::string type_of_token)
{
  std::string result = "";
  bool result_valid = true;

  if (!token.good() || (*token).size() == 0)
    result_valid = false;
  else if ((*token)[0] == '"' || (*token)[0] == '\'')
  {
    std::string::size_type start = 1;
    std::string::size_type pos = (*token).find('\\');
    while (pos != std::string::npos)
    {
      result += (*token).substr(start, pos - start);
      if ((*token)[pos + 1] == 'n')
        result += '\n';
      else if ((*token)[pos + 1] == 't')
        result += '\t';
      else if ((*token)[pos + 1] == 'u')
      {
        result += decode_to_utf8(*token, pos, error_output);
        pos -= 2;
      }
      else
        result += (*token)[pos + 1];
      start = pos + 2;
      pos = (*token).find('\\', start);
    }
    result += (*token).substr(start, (*token).size() - start - 1);
  }
  else if (isalpha((*token)[0]) || isdigit((*token)[0]) || (*token)[0] == '_')
    result = *token;
  else if ((*token)[0] == '-' && (*token).size() > 1 && isdigit((*token)[1]))
    result = *token;
  else
    result_valid = false;
  
  if (result_valid)
    ++token;
  else
  {
    if (error_output)
      error_output->add_parse_error(type_of_token + " expected - '" + *token + "' found.", token.line_col().first);
  }
  
  return result;
}


std::string get_identifier_token(Tokenizer_Wrapper& token, Error_Output* error_output,
		      std::string type_of_token)
{
  std::string result = "";
  bool result_valid = true;

  if (!token.good() || (*token).size() == 0)
    result_valid = false;
  else if (isalpha((*token)[0]) || (*token)[0] == '_')
    result = *token;
  else
    result_valid = false;
  
  if (result_valid)
    ++token;
  else
  {
    if (error_output)
      error_output->add_parse_error(type_of_token + " expected - '" + *token + "' found.", token.line_col().first);
  }
  
  return result;
}


void process_after(Tokenizer_Wrapper& token, Error_Output* error_output, bool after)
{
  if (!token.good())
  {
    if (error_output)
      error_output->add_parse_error("Unexpected end of input.", token.line_col().first);
  }
  else if (after)
    ++token;
}


void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, bool after)
{
  if (*token != target_1)
  {
    if (error_output)
      error_output->add_parse_error(std::string("'") + target_1 + "' expected - '"
          + *token + "' found.", token.line_col().first);
    ++token;
  }
  while (token.good() && *token != target_1)
    ++token;
  process_after(token, error_output, after);
}


void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, std::string target_2, bool after)
{
  if (*token != target_1 && *token != target_2)
  {
    if (error_output)
      error_output->add_parse_error
          (std::string("'") + target_1 + "' or '" + target_2 + "' expected - '"
	      + *token + "' found.", token.line_col().first);
    ++token;
  }
  while (token.good() && *token != target_1 && *token != target_2)
    ++token;
  process_after(token, error_output, after);
}


void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, std::string target_2, std::string target_3, bool after)
{
  if (*token != target_1 && *token != target_2 && *token != target_3)
  {
    if (error_output)
      error_output->add_parse_error
      (std::string("'") + target_1 + "', '" + target_2 + "', or '" + target_3 + "'  expected - '"
	      + *token + "' found.", token.line_col().first);
    ++token;
  }
  while (token.good() && *token != target_1 && *token != target_2 && *token != target_3)
    ++token;
  process_after(token, error_output, after);
}


void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, std::string target_2, std::string target_3, std::string target_4,
		       bool after)
{
  if (*token != target_1 && *token != target_2 && *token != target_3 && *token != target_4)
  {
    if (error_output)
      error_output->add_parse_error
          (std::string("'") + target_1 + "', '" + target_2 + "', '" + target_3 + "', or '"
              + target_4 + "'  expected - '"
	      + *token + "' found.", token.line_col().first);
    ++token;
  }
  while (token.good() && *token != target_1 && *token != target_2 && *token != target_3
      && *token != target_4)
    ++token;
  process_after(token, error_output, after);
}


void clear_until_after(Tokenizer_Wrapper& token, Error_Output* error_output,
		       std::string target_1, std::string target_2, std::string target_3, std::string target_4,
		       std::string target_5, bool after)
{
  if (*token != target_1 && *token != target_2 && *token != target_3
      && *token != target_4 && *token != target_5)
  {
    if (error_output)
      error_output->add_parse_error
          (std::string("'") + target_1 + "', '" + target_2 + "', '" + target_3+ "', '" + target_4
	      + "', or '" + target_5 + "'  expected - '"
	      + *token + "' found.", token.line_col().first);
    ++token;
  }
  while (token.good() && *token != target_1 && *token != target_2 && *token != target_3
      && *token != target_4 && *token != target_5)
    ++token;
  process_after(token, error_output, after);
}


int operator_priority(const std::string& operator_name, bool unary)
{
  static std::map< std::string, int > priority;
  if (priority.empty())
  {
    priority[","] = 1;
    priority["="] = 2;
    priority["?"] = 3;
    priority[":"] = 4;
    priority["||"] = 5;
    priority["&&"] = 6;
    priority["=="] = 7;
    priority["!="] = 7;
    priority["<"] = 8;
    priority["<="] = 8;
    priority[">"] = 8;
    priority[">="] = 8;
    priority["+"] = 10;
    priority["-"] = 10;
    priority["*"] = 11;
    priority["/"] = 11;
    priority["%"] = 11;
    priority["."] = 12;
    priority["->"] = 12;
    priority["!"] = 13;
    priority["::"] = 15;
  }
  
  if (unary)
    return 13;
  
  std::map< std::string, int >::const_iterator prio_it = priority.find(operator_name);
  if (prio_it != priority.end())
    return prio_it->second;
  
  return 0;
}


Token_Tree::Token_Tree(Tokenizer_Wrapper& token, Error_Output* error_output, bool parenthesis_expected)
{
  tree.push_back(Token_Node("", std::make_pair(0u, 0u)));
  std::vector< uint > stack;
  stack.push_back(0);
  
  while (token.good() && *token != ";" && *token != "->")
  {
    if (*token == "(" || *token == "[" || *token == "{")
    {
      tree.push_back(Token_Node(*token, token.line_col()));
      tree.back().lhs = tree[stack.back()].rhs;
      tree[stack.back()].rhs = tree.size()-1;
      stack.push_back(tree.size()-1);
    }
    else if (*token == ")" || *token == "]" ||*token == "}")
    {
      int stack_pos = stack.size()-1;
      while (stack_pos >= 0 && tree[stack[stack_pos]].token != "("
          && tree[stack[stack_pos]].token != "[" && tree[stack[stack_pos]].token != "{")
        --stack_pos;
      if (stack_pos < 0)
      {
        if (parenthesis_expected)
          return;
        else
          error_output->add_parse_error(std::string("Unmatched ") + *token, token.line_col().first);
      }
      else
        stack.resize(stack_pos);
    }
    else
    {
      int prio = operator_priority(*token, false);
      bool unary_minus = false;
      if ((*token).size() >= 2 && (*token)[0] == '-' && tree[stack.back()].rhs)
      {
        prio = 10;
        unary_minus = true;
      }
      
      if (prio > 0)
      {
        if (stack.back() < tree.size())
        {
          uint stack_pos = stack.size()-1;
          while (stack_pos > 0)
          {
            if (operator_priority(tree[stack[stack_pos]].token, false) < prio)
              break;
            --stack_pos;
          }
          stack.resize(stack_pos+1);
        }
        tree.push_back(Token_Node(unary_minus ? "-" : *token, token.line_col()));
        tree.back().lhs = tree[stack.back()].rhs;
        tree[stack.back()].rhs = tree.size()-1;
        stack.push_back(tree.size()-1);
        
        if (unary_minus)
        {
          tree.push_back(Token_Node((*token).substr(1), token.line_col()));
          tree[stack.back()].rhs = tree.size()-1;
        }
      }
      else if (!tree[stack.back()].rhs)
      {
        tree.push_back(Token_Node(*token, token.line_col()));
        tree[stack.back()].rhs = tree.size()-1;
      }
      else
        error_output->add_parse_error(std::string("Operator expected, but \"") + *token + "\"found.",
                                      token.line_col().first);
    }
    ++token;
  }
  
  int stack_pos = stack.size()-1;
  while (stack_pos >= 0 && tree[stack[stack_pos]].token != "("
      && tree[stack[stack_pos]].token != "[" && tree[stack[stack_pos]].token != "{")
    --stack_pos;
  if (stack_pos > 0)
    error_output->add_parse_error(std::string("Left ") + tree[stack[stack_pos]].token
        + " not closed.", token.line_col().first);
}


const std::string* Token_Node_Ptr::function_name() const
{
  if (tree && pos < tree->tree.size())
  {
    if (operator*().token == "(")
    {
      if (operator*().lhs)
        return &lhs()->token;
    }
    
    return &rhs().lhs()->token;
  }
  
  return 0;
}


bool Token_Node_Ptr::assert_is_function(Error_Output* error_output) const
{
  if (tree && pos < tree->tree.size())
  {
    if (operator*().token == "(")
    {
      if (!operator*().lhs)
      {
        if (error_output)
          error_output->add_parse_error("Function expected, but no function name found",
              + (tree && pos < tree->tree.size() ? operator*().line_col.first : 0));
        return false;
      }
      
      Token_Node_Ptr lhs_ = lhs();
      if (lhs_->lhs || lhs_->rhs)
      {
        if (error_output)
          error_output->add_parse_error("Function name expected, compound expression found",
              + (tree && pos < tree->tree.size() ? operator*().line_col.first : 0));
        return false;
      }
      
      return true;
    }
    else if (operator*().token == ".")
    {
      if (!operator*().rhs)
      {
        if (error_output)
          error_output->add_parse_error("Function expected, but token right of \".\" is void",
              + (tree && pos < tree->tree.size() ? operator*().line_col.first : 0));
        return false;
      }
      
      Token_Node_Ptr rhs_ = rhs();
      if (rhs_->token != "(")
      {
        if (error_output)
          error_output->add_parse_error(std::string("Function left parenthesis expected, but ")
              + "\"" + rhs_->token + "\" found.", rhs_->line_col.first);
        return false;
      }
      
      if (!rhs_->lhs)
      {
        if (error_output)
          error_output->add_parse_error("Function expected, but no function name found",
              + (tree && pos < tree->tree.size() ? operator*().line_col.first : 0));
        return false;
      }
      
      Token_Node_Ptr lhs_ = rhs_.lhs();
      if (lhs_->lhs || lhs_->rhs)
      {
        if (error_output)
          error_output->add_parse_error("Function name expected, compound expression found",
              + (tree && pos < tree->tree.size() ? operator*().line_col.first : 0));
        return false;
      }
      
      return true;
    }
  }
  
  if (error_output)
    error_output->add_parse_error(std::string("Function expected, but ")
        + (tree && pos < tree->tree.size() ?
            "\"" + operator*().token + "\"" : "void token") + " found.",
        (tree && pos < tree->tree.size() ? operator*().line_col.first : 0));
  
  return false;
}


bool Token_Node_Ptr::assert_has_input_set(Error_Output* error_output, bool expected) const
{
  // We assume that this is a function to avoid double checking
  
  if (operator*().token == "(")
  {
    if (expected)
    {
      if (error_output)
        error_output->add_parse_error("Input set expected, but only function name found",
            (tree && pos < tree->tree.size() ? operator*().line_col.first : 0));
      return false;
    }
    else
      return true;
  }
  else if (operator*().token == ".")
  {
    if (!operator*().lhs)
    {
      if (error_output)
        error_output->add_parse_error("Input set expected, but token left of \".\" is void",
            (tree && pos < tree->tree.size() ? operator*().line_col.first : 0));
      return false;
    }
      
    if (expected)
    {
      Token_Node_Ptr lhs_ = lhs();
      if (lhs_->lhs || lhs_->rhs)
      {
        if (error_output)
          error_output->add_parse_error("Input set expected, compound expression found",
              (tree && pos < tree->tree.size() ? operator*().line_col.first : 0));
        return false;
      }
      return true;
    }
    else
    {
      const std::string* func_name = function_name();
      if (error_output)
        error_output->add_parse_error((func_name ? *func_name + "(...)" : "Void function") + " cannot have an input set",
            (tree && pos < tree->tree.size() ? operator*().line_col.first : 0));
      return false;
    }
  }
    
  return false;
}


bool Token_Node_Ptr::assert_has_arguments(Error_Output* error_output, bool expected) const
{
  // We assume that this is a function to avoid double checking
  
  bool has_arguments = operator*().rhs;  
  if (operator*().token == ".")
    has_arguments = rhs()->rhs;
  
  if (has_arguments)
  {
    if (expected)
      return true;
    
    const std::string* func_name = function_name();
    if (error_output)
      error_output->add_parse_error((func_name ? *func_name + "(...)" : "Void function") 
          + " does not accept any arguments",
          (tree && pos < tree->tree.size() ? operator*().line_col.first : 0));
    return false;
  }
  else
  {
    if (!expected)
      return true;
    
    const std::string* func_name = function_name();
    if (error_output)
      error_output->add_parse_error((func_name ? *func_name + "(...)" : "Void function") 
          + " must have one or more arguments",
          (tree && pos < tree->tree.size() ? operator*().line_col.first : 0));
    return false;
  }
}
