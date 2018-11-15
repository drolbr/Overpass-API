#include "curl_client.h"


#include <curl/curl.h>
#include <cstdio>
#include <cstring>
#include <map>


Curl_Client::Curl_Client()
{
#ifdef HAVE_CURL
  static Global_Curl_Object global_curl;
  handle = curl_easy_init();
  if (!handle)
#endif
    throw Setup_Error();
}


Curl_Client::~Curl_Client()
{
#ifdef HAVE_CURL
  if (handle)
    curl_easy_cleanup(handle);
#endif
}


#include <iostream>
namespace
{
#ifdef HAVE_CURL
  size_t write_data(void* buffer, size_t size, size_t nmemb, void* userp)
  {
    std::string& result = *(std::string*)userp;
    uint old_size = result.size();
    result.resize(old_size + nmemb, ' ');
    memcpy(&result[old_size], buffer, nmemb);
    return nmemb;
  }

  size_t header_callback(char* buffer, size_t size, size_t nitems, void *userp)
  {
    std::string& return_code = *(std::string*)userp;
    if (return_code.empty())
      return_code = std::string(buffer, nitems*size);
    return nitems * size;
  }

  bool return_code_ok(const std::string& return_code)
  {
    std::string::size_type start = return_code.find(' ');
    if (start == std::string::npos)
      return false;
    std::string::size_type end = return_code.find(' ', start+1);
    return (end == std::string::npos ? return_code.substr(start+1)
        : return_code.substr(start+1, end-start-1)) == "200";
  }
#endif
}


std::string Curl_Client::send_request(const std::string& url)
{
  std::string answer;
#ifdef HAVE_CURL
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &write_data);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, &answer);

  std::string return_code;
  curl_easy_setopt(handle, CURLOPT_HEADERFUNCTION, &header_callback);
  curl_easy_setopt(handle, CURLOPT_HEADERDATA, &return_code);

  curl_easy_setopt(handle, CURLOPT_URL, url.c_str());

  CURLcode res = curl_easy_perform(handle);

  if (res != CURLE_OK)
    throw Download_Error(std::string("curl_easy_perform() failed: ") + curl_easy_strerror(res) + "\n", "");

  if (!return_code_ok(return_code))
    throw Download_Error(std::string("Got ") + return_code + "\n", answer);
#endif
  return answer;
}


std::string Curl_Client::send_request(const std::string& url, const std::map< std::string, std::string >& headers)
{
  std::string answer;
#ifdef HAVE_CURL
  curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &write_data);
  curl_easy_setopt(handle, CURLOPT_WRITEDATA, &answer);

  curl_easy_setopt(handle, CURLOPT_URL, url.c_str());

  curl_slist* slist = 0;
  for (std::map< std::string, std::string >::const_iterator it = headers.begin(); it != headers.end(); ++it)
    slist = curl_slist_append(slist, (it->first + ": " + it->second).c_str());
  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, slist);

  CURLcode res = curl_easy_perform(handle);
  curl_slist_free_all(slist);

  if (res != CURLE_OK)
    throw Download_Error(std::string("curl_easy_perform() failed: ") + curl_easy_strerror(res) + "\n", "");
#endif

  return answer;
}
