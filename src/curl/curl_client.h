#ifndef DE__OSM3S___CURL__CURL_CLIENT_H
#define DE__OSM3S___CURL__CURL_CLIENT_H


#ifdef HAVE_CONFIG_H
#include <config.h>
#undef VERSION
#endif
#ifdef HAVE_CURL
#include <curl/curl.h>
#endif


#include <map>
#include <stdexcept>
#include <string>


struct Curl_Client
{
  struct Setup_Error : std::runtime_error
  {
#ifdef HAVE_CURL
    Setup_Error() : std::runtime_error("curl_easy_init() returned a nullptr") {}
#else
    Setup_Error() : std::runtime_error("curl library not linked") {}
#endif
    Setup_Error(const std::string& message) : std::runtime_error(message.c_str()) {}
  };
  struct Download_Error
  {
    Download_Error(const std::string& message, const std::string& payload)
        : message_(message), payload_(payload) {}
    const std::string& payload() const { return payload_; }
    const std::string& what() const { return message_; }

  private:
    std::string message_;
    std::string payload_;
  };

  Curl_Client();
  ~Curl_Client();

  std::string send_request(const std::string& url);
  std::string send_request(const std::string& url, const std::map< std::string, std::string >& headers);

  const std::string& get_payload_if_error() const { return payload_if_error; }

private:
#ifdef HAVE_CURL
  CURL* handle;
#endif

  Curl_Client(const Curl_Client&);
  Curl_Client& operator=(const Curl_Client&);

  struct Global_Curl_Object
  {
#ifdef HAVE_CURL
    Global_Curl_Object() { curl_global_init(CURL_GLOBAL_DEFAULT); };
    ~Global_Curl_Object() { curl_global_cleanup(); }
#endif
  };

  std::string payload_if_error;
};


#endif
