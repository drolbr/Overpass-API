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

#include <cstdlib>
#include <iomanip>
#include <iostream>


double way_1_south(unsigned int pattern_size)
{
  return 51. + 1.5/pattern_size;
}

double way_1_north(unsigned int pattern_size)
{
  return 51. + 1.5/pattern_size;
}

double way_1_west(unsigned int pattern_size)
{
  return 7. + .5/pattern_size;
}

double way_1_east(unsigned int pattern_size)
{
  return 7. + 1.5/pattern_size;
}

unsigned int way_1_zoom(unsigned int pattern_size)
{
  if (pattern_size < 80)
    return 13;
  else if (pattern_size < 120)
    return 14;
  return 15;
}

double way_2_south(unsigned int pattern_size)
{
  return 51. + 1.5/pattern_size;
}

double way_2_north(unsigned int pattern_size)
{
  return 51. + 1.5/pattern_size;
}

double way_2_west(unsigned int pattern_size)
{
  return 7. + 1.5/pattern_size;
}

double way_2_east(unsigned int pattern_size)
{
  return 7. + 2.5/pattern_size;
}

unsigned int way_2_zoom(unsigned int pattern_size)
{
  if (pattern_size < 80)
    return 13;
  else if (pattern_size < 120)
    return 14;
  return 15;
}

double relation_1_south(unsigned int pattern_size)
{
  return 51. + .5 / pattern_size;
}

double relation_1_north(unsigned int pattern_size)
{
  return 51. + 1.5 / pattern_size;
}

double relation_1_west(unsigned int pattern_size)
{
  return 7. + .5 / pattern_size;
}

double relation_1_east(unsigned int pattern_size)
{
  return 7. + 1.5 / pattern_size;
}

unsigned int relation_1_zoom(unsigned int pattern_size)
{
  if (pattern_size < 80)
    return 13;
  else if (pattern_size < 120)
    return 14;
  return 15;
}

double way_meta_south(unsigned int pattern_size)
{
  if (pattern_size > 440)
    return 9.99752956;
  else if (pattern_size > 120)
    return 9.9909760;
  else if (pattern_size > 60)
    return 10.0040832;
  else
    return 9.9778688;
}

double way_meta_north(unsigned int pattern_size)
{
  if (pattern_size > 440)
    return 10.00408306;
  else if (pattern_size > 120)
    return 10.0171903;
  else if (pattern_size > 60)
    return 10.0302975;
  else
    return 10.0827263;
}

double way_meta_west(unsigned int pattern_size)
{
  return 0.9961472;
}

double way_meta_east(unsigned int pattern_size)
{
  if (pattern_size > 440)
    return 1.0027007;
  else if (pattern_size > 60)
    return 1.0223615;
  else
    return 1.1010047;
}

unsigned int way_meta_zoom(unsigned int pattern_size)
{
  if (pattern_size > 440)
    return 14;
  else if (pattern_size > 60)
    return 13;
  else
    return 11;
}

double relation_meta_south(unsigned int pattern_size)
{
  if (pattern_size > 440)
    return 9.99752956;
  else if (pattern_size > 120)
    return 9.9909760;
  else if (pattern_size > 60)
    return 10.0040832;
  else
    return 9.9778688;
}

double relation_meta_north(unsigned int pattern_size)
{
  if (pattern_size > 440)
    return 10.00408306;
  else if (pattern_size > 120)
    return 10.0171903;
  else if (pattern_size > 60)
    return 10.0302975;
  else
    return 10.0827263;
}

double relation_meta_west(unsigned int pattern_size)
{
  return 0.9961472;
}

double relation_meta_east(unsigned int pattern_size)
{
  if (pattern_size > 440)
    return 1.0027007;
  else if (pattern_size > 60)
    return 1.0223615;
  else
    return 1.1010047;
}

unsigned int relation_meta_zoom(unsigned int pattern_size)
{
  if (pattern_size > 440)
    return 14;
  else if (pattern_size > 60)
    return 13;
  else
    return 11;
}


void print_applicable_tags(unsigned int id)
{
  bool frame_printed = false;
  
  if (id % 11 == 0)
  {
    if (!frame_printed)
      std::cout<<",\n  \"tags\": {\n";
    else
      std::cout<<",\n";
    frame_printed = true;
    
    std::cout<<"    \"way_key_11\": \"way_value_"<<(id / 11 + 1)<<"\"";
  }
  if (id % 15 == 0)
  {
    if (!frame_printed)
      std::cout<<",\n  \"tags\": {\n";
    else
      std::cout<<",\n";
    frame_printed = true;
    
    std::cout<<"    \"way_key_15\": \"way_value_15\"";
  }
  if (id % 2 == 0)
  {
    if (!frame_printed)
      std::cout<<",\n  \"tags\": {\n";
    else
      std::cout<<",\n";
    frame_printed = true;
    
    std::cout<<"    \"way_key_2/4\": \"way_value_0\"";
  }
  if (id % 4 == 1)
  {
    if (!frame_printed)
      std::cout<<",\n  \"tags\": {\n";
    else
      std::cout<<",\n";
    frame_printed = true;
    
    std::cout<<"    \"way_key_2/4\": \"way_value_1\"";
  }
  if (id % 5 == 0)
  {
    if (!frame_printed)
      std::cout<<",\n  \"tags\": {\n";
    else
      std::cout<<",\n";
    frame_printed = true;
    
    std::cout<<"    \"way_key_5\": \"way_value_5\"";
  }
  if (id % 7 == 0)
  {
    if (!frame_printed)
      std::cout<<",\n  \"tags\": {\n";
    else
      std::cout<<",\n";
    frame_printed = true;
    
    std::cout<<"    \"way_key_7\": \"way_value_"<<(id % 21 / 7)<<"\"";
  }
    
  if (frame_printed)
    std::cout<<"\n  }";
}


int main(int argc, char* args[])
{
  if (argc < 3)
  {
    std::cout<<"Usage: "<<args[0]<<" Data_Size Test_Case\n";
    return 0;
  }

  unsigned int pattern_size = 2;
  if (argc > 1)
    pattern_size = atoi(args[1]);

  if (std::string(args[2]) == "interpreter_1" || std::string(args[2]) == "interpreter_2")
    std::cout<<
    "Content-type: application/osm3s+xml\n"
    "\n"
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<osm version=\"0.6\" generator=\"Overpass API\">\n"
    "<note>The data included in this document is from www.openstreetmap.org. "
    "The data is made available under ODbL.</note>\n"
    "<meta osm_base=\"mock-up-init\"/>\n"
    "\n"
    "\n"
    "</osm>\n";
  else if (std::string(args[2]) == "interpreter_3" || std::string(args[2]) == "interpreter_4"
      || std::string(args[2]) == "interpreter_5" || std::string(args[2]) == "interpreter_6"
      || std::string(args[2]) == "interpreter_7" || std::string(args[2]) == "interpreter_8"
      || std::string(args[2]) == "interpreter_9" || std::string(args[2]) == "interpreter_10")
  {
    std::cout<<
    "Status: 400 Bad Request\n"
    "Content-type: text/html; charset=utf-8\n"
    "\n"
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
    "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n"
    "<head>\n"
    "  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
    "  <title>OSM3S Response</title>\n"
    "</head>\n"
    "<body>\n"
    "\n"
    "<p>The data included in this document is from www.openstreetmap.org. "
    "The data is made available under ODbL.</p>\n";
    if (std::string(args[2]) == "interpreter_3")
      std::cout<<
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 1: parse error: Unknown type \"foo\" </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 1: parse error: An empty query is not allowed </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 1: parse error: Unknown type \";\" </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 1: parse error: An empty query is not allowed </p>\n";
    else if (std::string(args[2]) == "interpreter_4")
      std::cout<<
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 3: static error: Unknown tag \"foo\" in line 3. </p>\n";
    else if (std::string(args[2]) == "interpreter_5")
      std::cout<<
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 1: parse error: Unknown type \"foo\" </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 1: parse error: An empty query is not allowed </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 1: parse error: Unknown type \";\" </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 2: parse error: An empty query is not allowed </p>\n";
    else if (std::string(args[2]) == "interpreter_6")
      std::cout<<
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 2: parse error: Unknown type \"foo\" </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 2: parse error: An empty query is not allowed </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 2: parse error: Unknown type \";\" </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 2: parse error: An empty query is not allowed </p>\n";
    else if (std::string(args[2]) == "interpreter_7")
      std::cout<<
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 1: parse error: Unknown type \"foo\" </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 1: parse error: An empty query is not allowed </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 1: parse error: Unknown type \";\" </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 2: parse error: An empty query is not allowed </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 2: parse error: Unknown type \"foo\" </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 2: parse error: An empty query is not allowed </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 2: parse error: Unknown type \";\" </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 2: parse error: An empty query is not allowed </p>\n";
    else if (std::string(args[2]) == "interpreter_8")
      std::cout<<
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 3: static error: Unknown tag \"foo\" in line 3. </p>\n";
    else if (std::string(args[2]) == "interpreter_9")
      std::cout<<
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 4: static error: Unknown tag \"foo\" in line 4. </p>\n";
    else if (std::string(args[2]) == "interpreter_10")
      std::cout<<
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 3: static error: Unknown tag \"foo\" in line 3. </p>\n"
      "<p><strong style=\"color:#FF0000\">Error</strong>: line 4: static error: Unknown tag \"foo\" in line 4. </p>\n";
    std::cout<<
    "\n"
    "</body>\n"
    "</html>\n";
  }
  else if (std::string(args[2]) == "interpreter_11" || std::string(args[2]) == "interpreter_12"
      || std::string(args[2]) == "interpreter_13" || std::string(args[2]) == "interpreter_14"
      || std::string(args[2]) == "interpreter_15" || std::string(args[2]) == "interpreter_16"
      || std::string(args[2]) == "interpreter_17" || std::string(args[2]) == "interpreter_18"
      || std::string(args[2]) == "interpreter_19" || std::string(args[2]) == "interpreter_20"
      || std::string(args[2]) == "interpreter_21" || std::string(args[2]) == "interpreter_22"
      || std::string(args[2]) == "interpreter_23" || std::string(args[2]) == "interpreter_38"
      || std::string(args[2]) == "interpreter_45")
  {
    std::cout<<
    "Content-type: application/json\n"
    "\n";
    if (std::string(args[2]) == "interpreter_38")
      std::cout<<"foo(";
    std::cout<<
    "{\n"
    "  \"version\": 0.6,\n"
    "  \"generator\": \"Overpass API\",\n"
    "  \"osm3s\": {\n"
    "    \"timestamp_osm_base\": \"mock-up-init\",\n"
    "    \"copyright\": \"The data included in this document is from www.openstreetmap.org. "
    "The data is made available under ODbL.\"\n"
    "  },\n"
    "  \"elements\": [\n"
    "\n";
    if (std::string(args[2]) == "interpreter_11" || std::string(args[2]) == "interpreter_12"
        || std::string(args[2]) == "interpreter_17" || std::string(args[2]) == "interpreter_18"
	|| std::string(args[2]) == "interpreter_20" || std::string(args[2]) == "interpreter_23"
	|| std::string(args[2]) == "interpreter_38" || std::string(args[2]) == "interpreter_45")
      std::cout<<
      "{\n"
      "  \"type\": \"node\",\n"
      "  \"id\": 1,\n"
      "  \"lat\": "<<std::setprecision(7)<<std::fixed<<51 + 0.5/pattern_size<<",\n"
      "  \"lon\": "<<std::setprecision(7)<<std::fixed<<7 + 0.5/pattern_size<<",\n"
      "  \"tags\": {\n"
      "    \"node_key\": \"node_few\"\n"
      "  }\n"
      "}";
    if (std::string(args[2]) == "interpreter_12")
      std::cout<<
      ",\n"
      "{\n"
      "  \"type\": \"node\",\n"
      "  \"id\": "<<3*pattern_size + 3<<",\n"
      "  \"lat\": "<<std::setprecision(7)<<std::fixed<<51 + 3.5/pattern_size<<",\n"
      "  \"lon\": "<<std::setprecision(7)<<std::fixed<<7 + 2.5/pattern_size<<"\n"
      "}";
    if (std::string(args[2]) == "interpreter_17" || std::string(args[2]) == "interpreter_20"
        || std::string(args[2]) == "interpreter_23")
      std::cout<<",\n";
    if (std::string(args[2]) == "interpreter_13" || std::string(args[2]) == "interpreter_14"
        || std::string(args[2]) == "interpreter_17" || std::string(args[2]) == "interpreter_19"
	|| std::string(args[2]) == "interpreter_20" || std::string(args[2]) == "interpreter_23")
      std::cout<<
      "{\n"
      "  \"type\": \"way\",\n"
      "  \"id\": 1,\n"
      "  \"nodes\": [\n"
      "    "<<pattern_size + 1<<",\n"
      "    "<<pattern_size + 2<<"\n"
      "  ],\n"
      "  \"tags\": {\n"
      "    \"way_key\": \"way_few\",\n"
      "    \"way_key_2/4\": \"way_value_1\"\n"
      "  }\n"
      "}";
    if (std::string(args[2]) == "interpreter_14")
      std::cout<<
      ",\n"
      "{\n"
      "  \"type\": \"way\",\n"
      "  \"id\": 2,\n"
      "  \"nodes\": [\n"
      "    "<<pattern_size + 2<<",\n"
      "    "<<pattern_size + 3<<"\n"
      "  ],\n"
      "  \"tags\": {\n"
      "    \"way_key\": \"way_few\",\n"
      "    \"way_key_2/4\": \"way_value_0\"\n"
      "  }\n"
      "}";
    if (std::string(args[2]) == "interpreter_45")
    {
      std::cout<<
      ",\n"
      "{\n"
      "  \"type\": \"way\",\n"
      "  \"id\": "<<(pattern_size*pattern_size/2 - pattern_size + 1)<<",\n"
      "  \"bounds\": {\n"
      "    \"minlat\": "<<51.5 - .5/pattern_size<<",\n"
      "    \"minlon\": "<<7.5 + .5/pattern_size<<",\n"
      "    \"maxlat\": "<<52. - .5/pattern_size<<",\n"
      "    \"maxlon\": "<<7.5 + .5/pattern_size<<"\n"
      "  },\n"
      "  \"nodes\": [\n"
      "    "<<pattern_size/2 + 1;
      for (uint i = pattern_size/2 + pattern_size + 1; i < pattern_size*pattern_size; i += pattern_size)
        std::cout<<",\n    "<<i;
      std::cout<<
      "\n  ],\n"
      "  \"geometry\": ["
      "\n    null";
      for (uint i = 2; i < pattern_size/2; ++i)
        std::cout<<",\n    null";
      for (uint i = 0; i < pattern_size/2 + 1; ++i)
        std::cout<<",\n    { \"lat\": "<<51.5 - .5/pattern_size + double(i)/pattern_size
            <<", \"lon\": "<<7.5 + .5/pattern_size<<" }";
      std::cout<<
      "\n  ]";
      print_applicable_tags(pattern_size*pattern_size/2 - pattern_size + 1);
      std::cout<<
      "\n}"
      ",\n"
      "{\n"
      "  \"type\": \"way\",\n"
      "  \"id\": "<<(pattern_size*pattern_size/2 - pattern_size/2)<<",\n"
      "  \"bounds\": {\n"
      "    \"minlat\": "<<51.5 + .5/pattern_size<<",\n"
      "    \"minlon\": "<<7.5 - .5/pattern_size<<",\n"
      "    \"maxlat\": "<<51.5 + .5/pattern_size<<",\n"
      "    \"maxlon\": "<<8. - .5/pattern_size<<"\n"
      "  },\n"
      "  \"nodes\": [\n"
      "    "<<pattern_size*pattern_size/2 + pattern_size;
      for (uint i = pattern_size*pattern_size/2 + pattern_size - 1; i > pattern_size*pattern_size/2; --i)
        std::cout<<",\n    "<<i;
      std::cout<<
      "\n  ],\n"
      "  \"geometry\": [";
      for (uint i = 0; i < pattern_size/2 + 1; ++i)
        std::cout<<"\n    { \"lat\": "<<51.5 + .5/pattern_size
            <<", \"lon\": "<<8. - double(i)/pattern_size - .5/pattern_size<<" },";
      std::cout<<"\n    null";
      for (uint i = 2; i < pattern_size/2; ++i)
        std::cout<<",\n    null";
      std::cout<<
      "\n  ]";
      print_applicable_tags(pattern_size*pattern_size/2 - pattern_size/2);
      std::cout<<
      "\n},\n"
      "{\n" 
      "  \"type\": \"way\",\n"
      "  \"id\": "<<(pattern_size*pattern_size/2 - 1)<<",\n"
      "  \"nodes\": [\n"
      "    "<<(pattern_size*pattern_size/2 + 1)<<",\n"
      "    "<<(pattern_size/2 + 1)<<"\n"
      "  ],\n"
      "  \"geometry\": ["
      "\n    null,"
      "\n    null"
      "\n  ]";
      print_applicable_tags(pattern_size*pattern_size/2 - 1);
      std::cout<<
      "\n},\n"
      "{\n"
      "  \"type\": \"relation\",\n"
      "  \"id\": 7,\n"
      "  \"bounds\": {\n"
      "    \"minlat\": "<<51.5 - .5/pattern_size<<",\n"
      "    \"minlon\": "<<7.5 - .5/pattern_size<<",\n"
      "    \"maxlat\": "<<52. - .5/pattern_size<<",\n"
      "    \"maxlon\": "<<8. - .5/pattern_size<<"\n"
      "  },\n"
      "  \"members\": [\n"
      "    {\n"
      "      \"type\": \"way\",\n"
      "      \"ref\": "<<(pattern_size*pattern_size/2 - pattern_size + 1)<<",\n"
      "      \"role\": \"two\",\n"
      "      \"geometry\": ["
      "\n         null";
      for (uint i = 2; i < pattern_size/2; ++i)
        std::cout<<",\n         null";
      for (uint i = 0; i < pattern_size/2 + 1; ++i)
        std::cout<<",\n         { \"lat\": "<<51.5 - .5/pattern_size + double(i)/pattern_size
            <<", \"lon\": "<<7.5 + .5/pattern_size<<" }";
      std::cout<<
      "\n      ]\n"
      "    },\n"
      "    {\n"
      "      \"type\": \"way\",\n"
      "      \"ref\": "<<(pattern_size*pattern_size/2 - pattern_size/2)<<",\n"
      "      \"role\": \"one\",\n"
      "      \"geometry\": [";
      for (uint i = 0; i < pattern_size/2 + 1; ++i)
        std::cout<<"\n         { \"lat\": "<<51.5 + .5/pattern_size
            <<", \"lon\": "<<8. - double(i)/pattern_size - .5/pattern_size<<" },";
      std::cout<<"\n         null";
      for (uint i = 2; i < pattern_size/2; ++i)
        std::cout<<",\n         null";
      std::cout<<
      "\n      ]\n"
      "    },\n"
      "    {\n"
      "      \"type\": \"way\",\n"
      "      \"ref\": "<<(pattern_size*pattern_size/2 - 1)<<",\n"
      "      \"role\": \"zero\",\n"
      "      \"geometry\": [\n"
      "         null,\n"
      "         null\n"
      "      ]\n"
      "    }\n"
      "  ],\n"
      "  \"tags\": {\n"
      "    \"relation_key\": \"relation_few\",\n"
      "    \"relation_key_7\": \"relation_value_1\"\n"
      "  }\n"
      "}";
    }
    if (std::string(args[2]) == "interpreter_18" || std::string(args[2]) == "interpreter_19"
        || std::string(args[2]) == "interpreter_20" || std::string(args[2]) == "interpreter_23")
      std::cout<<",\n";
    if (std::string(args[2]) == "interpreter_15" || std::string(args[2]) == "interpreter_16"
        || std::string(args[2]) == "interpreter_18" || std::string(args[2]) == "interpreter_19"
	|| std::string(args[2]) == "interpreter_20" || std::string(args[2]) == "interpreter_23")
      std::cout<<
      "{\n"
      "  \"type\": \"relation\",\n"
      "  \"id\": 1,\n"
      "  \"members\": [\n"
      "    {\n"
      "      \"type\": \"node\",\n"
      "      \"ref\": 1,\n"
      "      \"role\": \"one\"\n"
      "    },\n"
      "    {\n"
      "      \"type\": \"node\",\n"
      "      \"ref\": "<<pattern_size + 2<<",\n"
      "      \"role\": \"two\"\n"
      "    }\n"
      "  ],\n"
      "  \"tags\": {\n"
      "    \"relation_key\": \"relation_few\",\n"
      "    \"relation_key_2/4\": \"relation_value_1\"\n"
      "  }\n"
      "}";
    if (std::string(args[2]) == "interpreter_16")
      std::cout<<
      ",\n"
      "{\n"
      "  \"type\": \"relation\",\n"
      "  \"id\": 9,\n"
      "  \"members\": [\n"
      "    {\n"
      "      \"type\": \"relation\",\n"
      "      \"ref\": 1,\n"
      "      \"role\": \"three\"\n"
      "    },\n"
      "    {\n"
      "      \"type\": \"relation\",\n"
      "      \"ref\": 2,\n"
      "      \"role\": \"zero\"\n"
      "    },\n"
      "    {\n"
      "      \"type\": \"relation\",\n"
      "      \"ref\": 3,\n"
      "      \"role\": \"one\"\n"
      "    },\n"
      "    {\n"
      "      \"type\": \"relation\",\n"
      "      \"ref\": 4,\n"
      "      \"role\": \"two\"\n"
      "    },\n"
      "    {\n"
      "      \"type\": \"relation\",\n"
      "      \"ref\": 5,\n"
      "      \"role\": \"three\"\n"
      "    },\n"
      "    {\n"
      "      \"type\": \"relation\",\n"
      "      \"ref\": 6,\n"
      "      \"role\": \"zero\"\n"
      "    }\n"
      "  ],\n"
      "  \"tags\": {\n"
      "    \"relation_key\": \"relation_few\",\n"
      "    \"relation_key_2/4\": \"relation_value_1\"\n"
      "  }\n"
      "}";
    if (std::string(args[2]) == "interpreter_21")
      std::cout<<
      "{\n"
      "  \"type\": \"node\",\n"
      "  \"id\": 1\n"
      "},\n"
      "{\n"
      "  \"type\": \"way\",\n"
      "  \"id\": 1\n"
      "},\n"
      "{\n"
      "  \"type\": \"relation\",\n"
      "  \"id\": 1\n"
      "}";
    if (std::string(args[2]) == "interpreter_22")
      std::cout<<
      "{\n"
      "  \"type\": \"node\",\n"
      "  \"id\": 1,\n"
      "  \"lat\": "<<std::setprecision(7)<<std::fixed<<51 + 0.5/pattern_size<<",\n"
      "  \"lon\": "<<std::setprecision(7)<<std::fixed<<7 + 0.5/pattern_size<<"\n"
      "},\n"
      "{\n"
      "  \"type\": \"way\",\n"
      "  \"id\": 1,\n"
      "  \"nodes\": [\n"
      "    "<<pattern_size + 1<<",\n"
      "    "<<pattern_size + 2<<"\n"
      "  ]\n"
      "},\n"
      "{\n"
      "  \"type\": \"relation\",\n"
      "  \"id\": 1,\n"
      "  \"members\": [\n"
      "    {\n"
      "      \"type\": \"node\",\n"
      "      \"ref\": 1,\n"
      "      \"role\": \"one\"\n"
      "    },\n"
      "    {\n"
      "      \"type\": \"node\",\n"
      "      \"ref\": "<<pattern_size + 2<<",\n"
      "      \"role\": \"two\"\n"
      "    }\n"
      "  ]\n"
      "}";
    std::cout<<
    "\n"
    "\n"
    "  ]\n"
    "}";
    if (std::string(args[2]) == "interpreter_38")
      std::cout<<");\n";
    else
      std::cout<<"\n";
  }
  else if (std::string(args[2]) == "interpreter_39" || std::string(args[2]) == "interpreter_40"
       || std::string(args[2]) == "interpreter_41")
  {
    std::cout<<"Status: 302 Moved\n";
    if (std::string(args[2]) == "interpreter_39")
      std::cout<<"Location: http://www.openstreetmap.org/browse/way/1\n\n";
    else if (std::string(args[2]) == "interpreter_40" || std::string(args[2]) == "interpreter_41")
      std::cout<<"Location: http://www.openstreetmap.org/?way=1\n\n";
  }
  else if (std::string(args[2]) == "interpreter_24" || std::string(args[2]) == "interpreter_25"
    || std::string(args[2]) == "interpreter_26" || std::string(args[2]) == "interpreter_27"
    || std::string(args[2]) == "interpreter_28" || std::string(args[2]) == "interpreter_29"
    || std::string(args[2]) == "interpreter_30" || std::string(args[2]) == "interpreter_31"
    || std::string(args[2]) == "interpreter_32" || std::string(args[2]) == "interpreter_33"
    || std::string(args[2]) == "interpreter_34" || std::string(args[2]) == "interpreter_35"
    || std::string(args[2]) == "interpreter_36" || std::string(args[2]) == "interpreter_37")
  {
    std::cout<<
    "Content-type: text/html; charset=utf-8\n"
    "\n"
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
    "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n"
    "<head>\n"
    "  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
    "  <title>OSM3S Response</title>\n"
    "</head>\n";
    if (std::string(args[2]) == "interpreter_24")
      std::cout<<"<body>\n";
    else
      std::cout<<"<body onload=\"init()\">\n";
    std::cout<<
    "\n"
    "<p>The data included in this document is from www.openstreetmap.org. "
    "The data is made available under ODbL.</p>\n"
    "<p>Data included until: mock-up-init</p>\n";
    if (std::string(args[2]) == "interpreter_24")
      std::cout<<"<p>No results found.</p>\n";
    else
    {
      std::cout<<
      "  <script src=\"http://openlayers.org/api/OpenLayers.js\"></script>\n"
      "  <script src=\"http://openstreetmap.org/openlayers/OpenStreetMap.js\"></script>\n"
      "  <script type=\"text/javascript\">\n"
      "\n"
      "      function make_remove_closure(id) {\n"
      "          var elem = document.getElementById(id);\n"
      "          return function(evt) {\n"
      "              elem.parentElement.removeChild(elem);\n"
      "          };\n"
      "      }\n"
      "\n"
      "      function display_map(div_id, query_url, lat, lon, zoom) {\n"
      "\n"
      "          map = new OpenLayers.Map(div_id, {\n"
      "          controls:[\n"
      "              new OpenLayers.Control.Navigation(),\n"
      "              new OpenLayers.Control.PanZoomBar(),\n"
      "              new OpenLayers.Control.LayerSwitcher(),\n"
      "              new OpenLayers.Control.Attribution()],\n"
      "              maxExtent: new OpenLayers.Bounds(-20037508.34,-20037508.34,20037508.34,20037508.34),\n"
      "              maxResolution: 156543.0399,\n"
      "              numZoomLevels: 19,\n"
      "              units: 'm',\n"
      "              projection: new OpenLayers.Projection(\"EPSG:900913\"),\n"
      "              displayProjection: new OpenLayers.Projection(\"EPSG:4326\")\n"
      "          } );\n"
      "\n"
      "          layerMapnik = new OpenLayers.Layer.OSM.Mapnik(\"Mapnik\");\n"
      "          map.addLayer(layerMapnik);\n"
      "\n"
      "          var lonLat = new OpenLayers.LonLat(lon, lat).transform(new OpenLayers.Projection(\"EPSG:4326\"), new OpenLayers.Projection(\"EPSG:900913\"));\n"
      "\n"
      "          map.setCenter (lonLat, zoom);\n"
      "\n"
      "          //Initialise the vector layer using OpenLayers.Format.OSM\n"
      "          var styleMap = new OpenLayers.StyleMap({\n"
      "              strokeColor: \"blue\",\n"
      "              strokeWidth: 10,\n"
      "              pointRadius: 20,\n"
      "              fillColor: \"blue\"\n"
      "          });\n"
      "          var layer = new OpenLayers.Layer.Vector(\"Polygon\", {\n"
      "              strategies: [new OpenLayers.Strategy.Fixed()],\n"
      "              protocol: new OpenLayers.Protocol.HTTP({\n"
      "                  url: query_url,\n"
      "                  format: new OpenLayers.Format.OSM()\n"
      "              }),\n"
      "              styleMap: styleMap,\n"
      "              projection: new OpenLayers.Projection(\"EPSG:4326\")\n"
      "          });\n"
      "\n"
      "          layer.events.register(\"featuresadded\", layer,\n"
      "              make_remove_closure(div_id + \"_progressbar\"));\n"
      "\n"
      "          map.addLayers([layer]);\n"
      "\n"
      "          return map;\n"
      "      }\n"
      "\n"
      "      var maps = new Array();\n"
      "\n"
      "      function init(){\n"
      "\n"
      "          var map_hooks = document.forms;\n"
      "          for (var i in map_hooks)\n"
      "          {\n"
      "              if (map_hooks[i].getAttribute(\"class\") == \"map_descriptor\")\n"
      "                  maps.push(display_map\n"
      "                      (map_hooks[i].getAttribute(\"action\"),\n"
      "                       map_hooks[i].url.value,\n"
      "                       map_hooks[i].lat.value, map_hooks[i].lon.value,\n"
      "                       map_hooks[i].zoom.value));\n"
      "          }\n"
      "\n"
      "      }\n"
      "  </script>\n"
      "\n";
      if (std::string(args[2]) == "interpreter_25" || std::string(args[2]) == "interpreter_27"
	  || std::string(args[2]) == "interpreter_29")
        std::cout<<"<h1>1 results found</h1>\n";
      else if (std::string(args[2]) == "interpreter_26" || std::string(args[2]) == "interpreter_28"
	  || std::string(args[2]) == "interpreter_30" || std::string(args[2]) == "interpreter_31"
	  || std::string(args[2]) == "interpreter_32" || std::string(args[2]) == "interpreter_33")
        std::cout<<"<h1>2 results found</h1>\n";
      else
	std::cout<<"<h1>3 results found</h1>\n";
      std::cout<<
      "\n"
      "\n";
      if (std::string(args[2]) == "interpreter_25" || std::string(args[2]) == "interpreter_26"
	  || std::string(args[2]) == "interpreter_31" || std::string(args[2]) == "interpreter_32"
	  || std::string(args[2]) == "interpreter_34" || std::string(args[2]) == "interpreter_36"
	  || std::string(args[2]) == "interpreter_37")
      {
        std::cout<<
        "\n"
        "<div style=\"min-height:300px;\">\n"
        "<div id=\"map_node_1\" style=\"width:300px;height:300px;float:right;\">\n"
        "<div id=\"map_node_1_progressbar\">Loading data ...</div>\n"
        "<form class=\"map_descriptor\" action=\"map_node_1\">\n"
        "  <input type=\"hidden\" name=\"url\" value=\"interpreter?data=node%281%29%3Bout+skel%3B\"/>\n"
	"  <input type=\"hidden\" name=\"lat\" value=\""<<std::setprecision(7)<<std::fixed<<51 + 0.5/pattern_size<<"\"/>\n"
	"  <input type=\"hidden\" name=\"lon\" value=\""<<std::setprecision(7)<<std::fixed<<7 + 0.5/pattern_size<<"\"/>\n"
        "  <input type=\"hidden\" name=\"zoom\" value=\"17\"/>\n"
        "</form>\n"
        "</div>\n"
        "\n"
	"<p>Node <strong>1</strong>, lat: "<<std::setprecision(7)<<std::fixed<<51 + 0.5/pattern_size<<", lon: "<<std::setprecision(7)<<std::fixed<<7 + 0.5/pattern_size<<",<br/>\n";
	if (std::string(args[2]) != "interpreter_36")
	  std::cout<<
          "node_key = node_few<br/>\n";
	std::cout<<
        "\n"
        "<a href=\"http://www.openstreetmap.org/browse/node/1\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
      }
      if (std::string(args[2]) == "interpreter_35")
        std::cout<<
        "\n"
        "<div style=\"min-height:300px;\">\n"
        "\n"
	"\n"
	"<p>Node <strong>1</strong>,<br/>\n"
        "\n"
        "<a href=\"http://www.openstreetmap.org/browse/node/1\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
      if (std::string(args[2]) == "interpreter_26")
	std::cout<<
        "\n"
        "<div style=\"min-height:300px;\">\n"
        "<div id=\"map_node_"<<3*pattern_size + 3<<"\" style=\"width:300px;height:300px;float:right;\">\n"
        "<div id=\"map_node_"<<3*pattern_size + 3<<"_progressbar\">Loading data ...</div>\n"
        "<form class=\"map_descriptor\" action=\"map_node_"<<3*pattern_size + 3<<"\">\n"
        "  <input type=\"hidden\" name=\"url\" value=\"interpreter?data=node%28"<<3*pattern_size + 3<<"%29%3Bout+skel%3B\"/>\n"
        "  <input type=\"hidden\" name=\"lat\" value=\""<<std::setprecision(7)<<std::fixed<<51 + 3.5/pattern_size<<"\"/>\n"
        "  <input type=\"hidden\" name=\"lon\" value=\""<<std::setprecision(7)<<std::fixed<<7 + 2.5/pattern_size<<"\"/>\n"
        "  <input type=\"hidden\" name=\"zoom\" value=\"17\"/>\n"
        "</form>\n"
        "</div>\n"
        "\n"
        "<p>Node <strong>"<<3*pattern_size + 3<<"</strong>, lat: "<<std::setprecision(7)<<std::fixed<<51 + 3.5/pattern_size<<", lon: "<<std::setprecision(7)<<std::fixed<<7 + 2.5/pattern_size<<",<br/>\n"
        "\n"
        "<a href=\"http://www.openstreetmap.org/browse/node/"<<3*pattern_size + 3<<"\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
      if (std::string(args[2]) == "interpreter_27" || std::string(args[2]) == "interpreter_28"
	   || std::string(args[2]) == "interpreter_31" || std::string(args[2]) == "interpreter_33"
	   || std::string(args[2]) == "interpreter_34")
      {
	std::cout<<
	"\n"
	"<div style=\"min-height:300px;\">\n"
        "<div id=\"map_way_1\" style=\"width:300px;height:300px;float:right;\">\n"
        "<div id=\"map_way_1_progressbar\">Loading data ...</div>\n"
        "<form class=\"map_descriptor\" action=\"map_way_1\">\n"
        "  <input type=\"hidden\" name=\"url\" value=\"interpreter?data=%28way%281%29%3Bnode%28w%29%29%3Bout+skel%3B\"/>\n"
        "  <input type=\"hidden\" name=\"lat\" value=\""<<std::setprecision(7)<<std::fixed<<(way_1_south(pattern_size) + way_1_north(pattern_size))/2.0<<"\"/>\n"
	"  <input type=\"hidden\" name=\"lon\" value=\""<<std::setprecision(7)<<std::fixed<<(way_1_west(pattern_size) + way_1_east(pattern_size))/2.0<<"\"/>\n"
	"  <input type=\"hidden\" name=\"zoom\" value=\""<<way_1_zoom(pattern_size)<<"\"/>\n"
        "</form>\n"
        "</div>\n"
        "\n"
        "<p>Way <strong>1</strong>, bounding box south: "<<std::setprecision(7)<<std::fixed<<way_1_south(pattern_size)<<", west: "<<std::setprecision(7)<<std::fixed<<way_1_west(pattern_size)<<", north: "<<std::setprecision(7)<<std::fixed<<way_1_north(pattern_size)<<", east: "<<std::setprecision(7)<<std::fixed<<way_1_east(pattern_size)<<",<br/>\n";
	if (std::string(args[2]) != "interpreter_36")
	  std::cout<<
          "way_key = way_few<br/>\n"
          "way_key_2/4 = way_value_1<br/>\n";
	std::cout<<
	" members: "<<pattern_size + 1<<", "<<pattern_size + 2<<"<br/>\n"
        "<a href=\"http://www.openstreetmap.org/browse/way/1\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
      }
      if (std::string(args[2]) == "interpreter_35")
	std::cout<<
	"\n"
	"<div style=\"min-height:300px;\">\n"
	"\n"
	"\n"
        "<p>Way <strong>1</strong>, no geographic reference<br/>\n"
        "<br/>\n"
        "<a href=\"http://www.openstreetmap.org/browse/way/1\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
      if (std::string(args[2]) == "interpreter_36")
	std::cout<<
	"\n"
	"<div style=\"min-height:300px;\">\n"
	"\n"
	"\n"
        "<p>Way <strong>1</strong>, no geographic reference<br/>\n"
	" members: "<<pattern_size+1<<", "<<pattern_size+2<<"<br/>\n"
        "<a href=\"http://www.openstreetmap.org/browse/way/1\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
      if (std::string(args[2]) == "interpreter_37")
	std::cout<<
	"\n"
	"<div style=\"min-height:300px;\">\n"
	"\n"
	"\n"
        "<p>Way <strong>1</strong>, no geographic reference<br/>\n"
        "way_key = way_few<br/>\n"
        "way_key_2/4 = way_value_1<br/>\n"
	" members: "<<pattern_size+1<<", "<<pattern_size+2<<"<br/>\n"
        "<a href=\"http://www.openstreetmap.org/browse/way/1\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
      if (std::string(args[2]) == "interpreter_28")
	std::cout<<
	"\n"
        "<div style=\"min-height:300px;\">\n"
        "<div id=\"map_way_2\" style=\"width:300px;height:300px;float:right;\">\n"
        "<div id=\"map_way_2_progressbar\">Loading data ...</div>\n"
        "<form class=\"map_descriptor\" action=\"map_way_2\">\n"
        "  <input type=\"hidden\" name=\"url\" value=\"interpreter?data=%28way%282%29%3Bnode%28w%29%29%3Bout+skel%3B\"/>\n"
	"  <input type=\"hidden\" name=\"lat\" value=\""<<std::setprecision(7)<<std::fixed<<(way_2_south(pattern_size) + way_2_north(pattern_size))/2.0<<"\"/>\n"
	"  <input type=\"hidden\" name=\"lon\" value=\""<<std::setprecision(7)<<std::fixed<<(way_2_west(pattern_size) + way_2_east(pattern_size))/2.0<<"\"/>\n"
	"  <input type=\"hidden\" name=\"zoom\" value=\""<<way_2_zoom(pattern_size)<<"\"/>\n"
        "</form>\n"
        "</div>\n"
        "\n"
        "<p>Way <strong>2</strong>, bounding box south: "<<std::setprecision(7)<<std::fixed<<way_2_south(pattern_size)<<", west: "<<std::setprecision(7)<<std::fixed<<way_2_west(pattern_size)<<", north: "<<std::setprecision(7)<<std::fixed<<way_2_north(pattern_size)<<", east: "<<std::setprecision(7)<<std::fixed<<way_2_east(pattern_size)<<",<br/>\n"
        "way_key = way_few<br/>\n"
        "way_key_2/4 = way_value_0<br/>\n"
	" members: "<<pattern_size + 2<<", "<<pattern_size + 3<<"<br/>\n"
        "<a href=\"http://www.openstreetmap.org/browse/way/2\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
      if (std::string(args[2]) == "interpreter_30"
	   || std::string(args[2]) == "interpreter_32" || std::string(args[2]) == "interpreter_33"
	   || std::string(args[2]) == "interpreter_34")
      {
	std::cout<<
	"\n"
	"<div style=\"min-height:300px;\">\n"
        "<div id=\"map_relation_1\" style=\"width:300px;height:300px;float:right;\">\n"
        "<div id=\"map_relation_1_progressbar\">Loading data ...</div>\n"
        "<form class=\"map_descriptor\" action=\"map_relation_1\">\n"
        "  <input type=\"hidden\" name=\"url\" value=\"interpreter?data=%28relation%281%29%3Bnode%28r%29%2D%3E%2Ex%3Bway%28r%29%3Bnode%28w%29%3B%29%3Bout+skel%3B\"/>\n"
	"  <input type=\"hidden\" name=\"lat\" value=\""<<std::setprecision(7)<<std::fixed<<(relation_1_south(pattern_size) + relation_1_north(pattern_size))/2.0<<"\"/>\n"
	"  <input type=\"hidden\" name=\"lon\" value=\""<<std::setprecision(7)<<std::fixed<<(relation_1_west(pattern_size) + relation_1_east(pattern_size))/2.0<<"\"/>\n"
	"  <input type=\"hidden\" name=\"zoom\" value=\""<<relation_1_zoom(pattern_size)<<"\"/>\n"
        "</form>\n"
        "</div>\n"
	"\n"
	"<p>Relation <strong>1</strong>, bounding box south: "<<std::setprecision(7)<<std::fixed<<relation_1_south(pattern_size)<<", west: "<<std::setprecision(7)<<std::fixed<<relation_1_west(pattern_size)<<", north: "<<std::setprecision(7)<<std::fixed<<relation_1_north(pattern_size)<<", east: "<<std::setprecision(7)<<std::fixed<<relation_1_east(pattern_size)<<",<br/>\n";
	if (std::string(args[2]) != "interpreter_36")
	  std::cout<<
          "relation_key = relation_few<br/>\n"
          "relation_key_2/4 = relation_value_1<br/>\n";
	std::cout<<
	" members: node <strong>1</strong> &quot;one&quot;, node <strong>"<<pattern_size + 2<<"</strong> &quot;two&quot;<br/>\n"
        "<a href=\"http://www.openstreetmap.org/browse/relation/1\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
      }
      if (std::string(args[2]) == "interpreter_35")
	std::cout<<
	"\n"
	"<div style=\"min-height:300px;\">\n"
	"\n"
	"\n"
        "<p>Relation <strong>1</strong>, no geographic reference<br/>\n"
        "<br/>\n"
        "<a href=\"http://www.openstreetmap.org/browse/relation/1\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
      if (std::string(args[2]) == "interpreter_36")
	std::cout<<
	"\n"
	"<div style=\"min-height:300px;\">\n"
	"\n"
	"\n"
        "<p>Relation <strong>1</strong>, no geographic reference<br/>\n"
	" members: node <strong>1</strong> &quot;one&quot;, node <strong>"<<pattern_size+2<<"</strong> &quot;two&quot;<br/>\n"
        "<a href=\"http://www.openstreetmap.org/browse/relation/1\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
      if (std::string(args[2]) == "interpreter_29" || std::string(args[2]) == "interpreter_37")
	std::cout<<
	"\n"
	"<div style=\"min-height:300px;\">\n"
	"\n"
	"\n"
        "<p>Relation <strong>1</strong>, no geographic reference<br/>\n"
        "relation_key = relation_few<br/>\n"
        "relation_key_2/4 = relation_value_1<br/>\n"
	" members: node <strong>1</strong> &quot;one&quot;, node <strong>"<<pattern_size+2<<"</strong> &quot;two&quot;<br/>\n"
        "<a href=\"http://www.openstreetmap.org/browse/relation/1\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
      if (std::string(args[2]) == "interpreter_30")
	std::cout<<
	"\n"
        "<div style=\"min-height:300px;\">\n"
	"\n"
	"\n"
	"<p>Relation <strong>9</strong>, no geographic reference<br/>\n"
        "relation_key = relation_few<br/>\n"
        "relation_key_2/4 = relation_value_1<br/>\n"
	" members: relation <strong>1</strong> &quot;three&quot;, relation <strong>2</strong> &quot;zero&quot;, relation <strong>3</strong> &quot;one&quot;, relation <strong>4</strong> &quot;two&quot;, relation <strong>5</strong> &quot;three&quot;, relation <strong>6</strong> &quot;zero&quot;<br/>\n"
        "<a href=\"http://www.openstreetmap.org/browse/relation/9\">Browse on openstreetmap.org</a></p>\n"
        "</div>\n";
    }
    std::cout<<
    "\n"
    "</body>\n"
    "</html>\n";
  }
  else if (std::string(args[2]) == "interpreter_42" || std::string(args[2]) == "interpreter_43"
    || std::string(args[2]) == "interpreter_44")
  {
    std::cout<<
    "Content-type: text/html; charset=utf-8\n"
    "\n"
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
    "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n"
    "<head>\n"
    "  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
    "  <title>OSM3S Response</title>\n"
    "</head>\n"
    "<body>\n"
    "\n"
    "<p>The data included in this document is from www.openstreetmap.org. "
    "The data is made available under ODbL.</p>\n"
    "<p>Data included until: mock-up-init</p>\n";
    if (std::string(args[2]) == "interpreter_42")
      std::cout<<"<h1>1 results found</h1>\n";
    else if (std::string(args[2]) == "interpreter_43")
      std::cout<<"<h1>3 results found</h1>\n";
    if (std::string(args[2]) == "interpreter_44")
      std::cout<<"<h1>6 results found</h1>\n";
    std::cout<<
    "\n"
    "\n"
    "\n"
    "<p>Node <strong>1</strong>, lat: "<<std::setprecision(7)<<std::fixed<<51 + 0.5/pattern_size<<", lon: "<<std::setprecision(7)<<std::fixed<<7 + 0.5/pattern_size<<",<br/>\n"
    "node_key = node_few<br/>\n"
    "\n"
    "<a href=\"http://www.openstreetmap.org/browse/node/1\">Browse on openstreetmap.org</a></p>\n"
    "\n";
    if (std::string(args[2]) == "interpreter_44")
      std::cout<<
      "<p>Node <strong>"<<3*pattern_size + 3<<"</strong>, lat: "<<std::setprecision(7)<<std::fixed<<51 + 3.5/pattern_size<<", lon: "<<std::setprecision(7)<<std::fixed<<7 + 2.5/pattern_size<<",<br/>\n"
      "\n"
      "<a href=\"http://www.openstreetmap.org/browse/node/"<<3*pattern_size + 3<<"\">Browse on openstreetmap.org</a></p>\n"
      "\n";
    if (std::string(args[2]) == "interpreter_43" || std::string(args[2]) == "interpreter_44")
      std::cout<<
      "<p>Way <strong>1</strong>, bounding box south: "<<std::setprecision(7)<<std::fixed<<way_1_south(pattern_size)<<", west: "<<std::setprecision(7)<<std::fixed<<way_1_west(pattern_size)<<", north: "<<std::setprecision(7)<<std::fixed<<way_1_north(pattern_size)<<", east: "<<std::setprecision(7)<<std::fixed<<way_1_east(pattern_size)<<",<br/>\n"
      "way_key = way_few<br/>\n"
      "way_key_2/4 = way_value_1<br/>\n"
      " members: "<<pattern_size + 1<<", "<<pattern_size + 2<<"<br/>\n"
      "<a href=\"http://www.openstreetmap.org/browse/way/1\">Browse on openstreetmap.org</a></p>\n"
      "\n";
    if (std::string(args[2]) == "interpreter_44")
      std::cout<<
      "<p>Way <strong>2</strong>, bounding box south: "<<std::setprecision(7)<<std::fixed<<way_2_south(pattern_size)<<", west: "<<std::setprecision(7)<<std::fixed<<way_2_west(pattern_size)<<", north: "<<std::setprecision(7)<<std::fixed<<way_2_north(pattern_size)<<", east: "<<std::setprecision(7)<<std::fixed<<way_2_east(pattern_size)<<",<br/>\n"
      "way_key = way_few<br/>\n"
      "way_key_2/4 = way_value_0<br/>\n"
      " members: "<<pattern_size + 2<<", "<<pattern_size + 3<<"<br/>\n"
      "<a href=\"http://www.openstreetmap.org/browse/way/2\">Browse on openstreetmap.org</a></p>\n"
      "\n";
    if (std::string(args[2]) == "interpreter_43" || std::string(args[2]) == "interpreter_44")
      std::cout<<
      "<p>Relation <strong>1</strong>, bounding box south: "<<std::setprecision(7)<<std::fixed<<relation_1_south(pattern_size)<<", west: "<<std::setprecision(7)<<std::fixed<<relation_1_west(pattern_size)<<", north: "<<std::setprecision(7)<<std::fixed<<relation_1_north(pattern_size)<<", east: "<<std::setprecision(7)<<std::fixed<<relation_1_east(pattern_size)<<",<br/>\n"
      "relation_key = relation_few<br/>\n"
      "relation_key_2/4 = relation_value_1<br/>\n"
      " members: node <strong>1</strong> &quot;one&quot;, node <strong>"<<pattern_size + 2<<"</strong> &quot;two&quot;<br/>\n"
      "<a href=\"http://www.openstreetmap.org/browse/relation/1\">Browse on openstreetmap.org</a></p>\n"
      "\n";
    if (std::string(args[2]) == "interpreter_44")
      std::cout<<
      "<p>Relation <strong>9</strong>, no geographic reference<br/>\n"
      "relation_key = relation_few<br/>\n"
      "relation_key_2/4 = relation_value_1<br/>\n"
      " members: relation <strong>1</strong> &quot;three&quot;, relation <strong>2</strong> &quot;zero&quot;, relation <strong>3</strong> &quot;one&quot;, relation <strong>4</strong> &quot;two&quot;, relation <strong>5</strong> &quot;three&quot;, relation <strong>6</strong> &quot;zero&quot;<br/>\n"
      "<a href=\"http://www.openstreetmap.org/browse/relation/9\">Browse on openstreetmap.org</a></p>\n"
      "\n";
    std::cout<<
    "</body>\n"
    "</html>\n";
  }
  else if (std::string(args[2]) == "interpreter_46")
  {
    std::cout<<
    "Content-type: application/json\n"
    "\n"
    "{\n"
    "  \"version\": 0.6,\n"
    "  \"generator\": \"Overpass API\",\n"
    "  \"osm3s\": {\n"
    "    \"timestamp_osm_base\": \"mock-up-init\",\n"
    "    \"copyright\": \"The data included in this document is from www.openstreetmap.org. "
    "The data is made available under ODbL.\"\n"
    "  },\n"
    "  \"elements\": [\n"
    "\n"
    "{\n"
    "  \"type\": \"node\",\n"
    "  \"id\": 7,\n"
    "  \"lat\": "<<std::setprecision(7)<<std::fixed<<10 + 0.5/pattern_size<<",\n"
    "  \"lon\": "<<std::setprecision(7)<<std::fixed<<1 + 6.5/pattern_size<<",\n"
    "  \"timestamp\": \"2001-01-01T00:00:07Z\",\n"
    "  \"version\": 9,\n"
    "  \"changeset\": 18,\n"
    "  \"user\": \"User_11\",\n"
    "  \"uid\": 11,\n"
    "  \"tags\": {\n"
    "    \"foo\": \"bar\"\n"
    "  }\n"
    "},\n"
    "{\n"
    "  \"type\": \"way\",\n"
    "  \"id\": 1,\n"
    "  \"timestamp\": \"2002-01-01T00:00:01Z\",\n"
    "  \"version\": 4,\n"
    "  \"changeset\": 13,\n"
    "  \"user\": \"User_12\",\n"
    "  \"uid\": 12,\n"
    "  \"nodes\": [\n"
    "    1,\n"
    "    2\n"
    "  ]\n"
    "},\n"
    "{\n"
    "  \"type\": \"relation\",\n"
    "  \"id\": 1,\n"
    "  \"timestamp\": \"2003-01-01T00:00:01Z\",\n"
    "  \"version\": 5,\n"
    "  \"changeset\": 14,\n"
    "  \"user\": \"User_13\",\n"
    "  \"uid\": 13,\n"
    "  \"members\": [\n"
    "    {\n"
    "      \"type\": \"node\",\n"
    "      \"ref\": 1,\n"
    "      \"role\": \"one\"\n"
    "    },\n"
    "    {\n"
    "      \"type\": \"node\",\n"
    "      \"ref\": 2,\n"
    "      \"role\": \"two\"\n"
    "    }\n"
    "  ]\n"
    "}\n"
    "\n"
    "  ]\n"
    "}\n";
  }
  else if (std::string(args[2]) == "interpreter_47")
  {
    std::cout<<
    "Content-type: text/html; charset=utf-8\n"
    "\n"
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
    "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
    "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n"
    "<head>\n"
    "  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
    "  <title>OSM3S Response</title>\n"
    "</head>\n"
    "<body onload=\"init()\">\n"
    "\n"
    "<p>The data included in this document is from www.openstreetmap.org. "
    "The data is made available under ODbL.</p>\n"
    "<p>Data included until: mock-up-init</p>\n"
    "  <script src=\"http://openlayers.org/api/OpenLayers.js\"></script>\n"
    "  <script src=\"http://openstreetmap.org/openlayers/OpenStreetMap.js\"></script>\n"
    "  <script type=\"text/javascript\">\n"
    "\n"
    "      function make_remove_closure(id) {\n"
    "          var elem = document.getElementById(id);\n"
    "          return function(evt) {\n"
    "              elem.parentElement.removeChild(elem);\n"
    "          };\n"
    "      }\n"
    "\n"
    "      function display_map(div_id, query_url, lat, lon, zoom) {\n"
    "\n"
    "          map = new OpenLayers.Map(div_id, {\n"
    "          controls:[\n"
    "              new OpenLayers.Control.Navigation(),\n"
    "              new OpenLayers.Control.PanZoomBar(),\n"
    "              new OpenLayers.Control.LayerSwitcher(),\n"
    "              new OpenLayers.Control.Attribution()],\n"
    "              maxExtent: new OpenLayers.Bounds(-20037508.34,-20037508.34,20037508.34,20037508.34),\n"
    "              maxResolution: 156543.0399,\n"
    "              numZoomLevels: 19,\n"
    "              units: 'm',\n"
    "              projection: new OpenLayers.Projection(\"EPSG:900913\"),\n"
    "              displayProjection: new OpenLayers.Projection(\"EPSG:4326\")\n"
    "          } );\n"
    "\n"
    "          layerMapnik = new OpenLayers.Layer.OSM.Mapnik(\"Mapnik\");\n"
    "          map.addLayer(layerMapnik);\n"
    "\n"
    "          var lonLat = new OpenLayers.LonLat(lon, lat).transform(new OpenLayers.Projection(\"EPSG:4326\"), new OpenLayers.Projection(\"EPSG:900913\"));\n"
    "\n"
    "          map.setCenter (lonLat, zoom);\n"
    "\n"
    "          //Initialise the vector layer using OpenLayers.Format.OSM\n"
    "          var styleMap = new OpenLayers.StyleMap({\n"
    "              strokeColor: \"blue\",\n"
    "              strokeWidth: 10,\n"
    "              pointRadius: 20,\n"
    "              fillColor: \"blue\"\n"
    "          });\n"
    "          var layer = new OpenLayers.Layer.Vector(\"Polygon\", {\n"
    "              strategies: [new OpenLayers.Strategy.Fixed()],\n"
    "              protocol: new OpenLayers.Protocol.HTTP({\n"
    "                  url: query_url,\n"
    "                  format: new OpenLayers.Format.OSM()\n"
    "              }),\n"
    "              styleMap: styleMap,\n"
    "              projection: new OpenLayers.Projection(\"EPSG:4326\")\n"
    "          });\n"
    "\n"
    "          layer.events.register(\"featuresadded\", layer,\n"
    "              make_remove_closure(div_id + \"_progressbar\"));\n"
    "\n"
    "          map.addLayers([layer]);\n"
    "\n"
    "          return map;\n"
    "      }\n"
    "\n"
    "      var maps = new Array();\n"
    "\n"
    "      function init(){\n"
    "\n"
    "          var map_hooks = document.forms;\n"
    "          for (var i in map_hooks)\n"
    "          {\n"
    "              if (map_hooks[i].getAttribute(\"class\") == \"map_descriptor\")\n"
    "                  maps.push(display_map\n"
    "                      (map_hooks[i].getAttribute(\"action\"),\n"
    "                       map_hooks[i].url.value,\n"
    "                       map_hooks[i].lat.value, map_hooks[i].lon.value,\n"
    "                       map_hooks[i].zoom.value));\n"
    "          }\n"
    "\n"
    "      }\n"
    "  </script>\n"
    "\n"
    "<h1>3 results found</h1>\n"
    "\n"
    "\n"
    "\n"
    "<div style=\"min-height:300px;\">\n"
    "<div id=\"map_node_7\" style=\"width:300px;height:300px;float:right;\">\n"
    "<div id=\"map_node_7_progressbar\">Loading data ...</div>\n"
    "<form class=\"map_descriptor\" action=\"map_node_7\">\n"
    "  <input type=\"hidden\" name=\"url\" value=\"interpreter?data=node%287%29%3Bout+skel%3B\"/>\n"
    "  <input type=\"hidden\" name=\"lat\" value=\""<<std::setprecision(7)<<std::fixed<<10 + 0.5/pattern_size<<"\"/>\n"
    "  <input type=\"hidden\" name=\"lon\" value=\""<<std::setprecision(7)<<std::fixed<<1 + 6.5/pattern_size<<"\"/>\n"
    "  <input type=\"hidden\" name=\"zoom\" value=\"17\"/>\n"
    "</form>\n"
    "</div>\n"
    "\n"
    "<p>Node <strong>7</strong>, lat: "<<std::setprecision(7)<<std::fixed<<10 + 0.5/pattern_size<<", lon: "<<std::setprecision(7)<<std::fixed<<1 + 6.5/pattern_size<<",<br/>\n"
    "foo = bar<br/>\n"
    "\n"
    "<a href=\"http://www.openstreetmap.org/browse/node/7\">Browse on openstreetmap.org</a></p>\n"
    "</div>\n"
    "\n"
    "<div style=\"min-height:300px;\">\n"
    "\n"
    "\n"
    "<p>Way <strong>1</strong>, no geographic reference<br/>\n"
    " members: 1, 2<br/>\n"
    "<a href=\"http://www.openstreetmap.org/browse/way/1\">Browse on openstreetmap.org</a></p>\n"
    "</div>\n"
    "\n"
    "<div style=\"min-height:300px;\">\n"
    "\n"
    "\n"
    "<p>Relation <strong>1</strong>, no geographic reference<br/>\n"
    " members: node <strong>1</strong> &quot;one&quot;, node <strong>2</strong> &quot;two&quot;<br/>\n"
    "<a href=\"http://www.openstreetmap.org/browse/relation/1\">Browse on openstreetmap.org</a></p>\n"
    "</div>\n"
    "\n"
    "</body>\n"
    "</html>\n";
  }
}
