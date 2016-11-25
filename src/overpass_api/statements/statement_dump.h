/** Copyright 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016 Roland Olbricht et al.
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

#ifndef DE__OSM3S___OVERPASS_API__STATEMENTS__STATEMENT_DUMP_H
#define DE__OSM3S___OVERPASS_API__STATEMENTS__STATEMENT_DUMP_H


#include "statement.h"

#include <map>
#include <string>
#include <vector>


/**
 * The base class for all statements
 */
class Statement_Dump
{
  public:
    struct Factory
    {
      Factory(Statement::Factory& stmt_factory_) : bbox_limitation(0), stmt_factory(&stmt_factory_) {}
      
      Statement_Dump* create_statement(std::string element, int line_number,
				       const std::map< std::string, std::string >& attributes);
      Statement_Dump* create_statement(const Token_Node_Ptr& tree_it);
      
      int bbox_limitation;
      Statement::Factory* stmt_factory;
    };
    
    Statement_Dump(std::string name, const std::map< std::string, std::string >& attributes_)
        : name_(name), attributes(attributes_) {}
    ~Statement_Dump();
    
    void add_statement(Statement_Dump* statement, std::string text);
    std::string dump_xml() const;
    std::string dump_pretty_map_ql() const;
    std::string dump_compact_map_ql() const;
    std::string dump_bbox_map_ql() const;
    
    const std::string& name() const { return name_; }
    std::string attribute(const std::string& key) const;
    
    void add_final_text(std::string text) {}

  private:
    std::string name_;
    std::map< std::string, std::string > attributes;
    std::vector< Statement_Dump* > substatements;
};

#endif
