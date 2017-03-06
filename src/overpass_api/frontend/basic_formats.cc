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


#include "basic_formats.h"


#include <string>


const std::string& iso_string(uint64 timestamp)
{
  uint32 year = (timestamp)>>26;
  uint32 month = ((timestamp)>>22) & 0xf;
  uint32 day = ((timestamp)>>17) & 0x1f;
  uint32 hour = ((timestamp)>>12) & 0x1f;
  uint32 minute = ((timestamp)>>6) & 0x3f;
  uint32 second = timestamp & 0x3f;
  static std::string result("    -  -  T  :  :  Z");
  result[0] = (year / 1000) % 10 + '0';
  result[1] = (year / 100) % 10 + '0';
  result[2] = (year / 10) % 10 + '0';
  result[3] = year % 10 + '0';
  result[5] = (month / 10) % 10 + '0';
  result[6] = month % 10 + '0';
  result[8] = (day / 10) % 10 + '0';
  result[9] = day % 10 + '0';
  result[11] = (hour / 10) % 10 + '0';
  result[12] = hour % 10 + '0';
  result[14] = (minute / 10) % 10 + '0';
  result[15] = minute % 10 + '0';
  result[17] = (second / 10) % 10 + '0';
  result[18] = second % 10 + '0';
  return result;
}


void write_html_header
    (const std::string& timestamp, const std::string& area_timestamp, uint write_mime, bool write_js_init,
     bool write_remarks)
{
  if (write_mime > 0)
  {
    if (write_mime != 200)
    {
      if (write_mime == 504)
        std::cout<<"Status: "<<write_mime<<" Gateway Timeout\n";
      else if (write_mime == 400)
        std::cout<<"Status: "<<write_mime<<" Bad Request\n";
      else if (write_mime == 429)
        std::cout<<"Status: "<<write_mime<<" Too Many Requests\n";
      else
        std::cout<<"Status: "<<write_mime<<"\n";
    }
    std::cout<<"Content-type: text/html; charset=utf-8\n\n";
  }
  std::cout<<
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
  "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\"\n"
  "    \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">\n"
  "<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">\n"
  "<head>\n"
  "  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\" lang=\"en\"/>\n"
  "  <title>OSM3S Response</title>\n"
  "</head>\n";
  std::cout<<(write_js_init ? "<body onload=\"init()\">\n\n" : "<body>\n\n");
  if (write_remarks)
  {
    std::cout<<
    "<p>The data included in this document is from www.openstreetmap.org. "
    "The data is made available under ODbL.</p>\n";
    if (timestamp != "")
    {
      std::cout<<"<p>Data included until: "<<timestamp;
      if (area_timestamp != "")
        std::cout<<"<br/>Areas based on data until: "<<area_timestamp;
      std::cout<<"</p>\n";
    }
  }
}
