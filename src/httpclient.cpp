#include "httpclient.h"
#include "applutil.h"

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

bool httpclient::sync(const std::string &hostnameport, const std::string &url, std::string &result, const std::string& info)
{
   try
   {
      //boost::asio::ip::tcp::resolver::query query( _host, mylib::to_string( _port ) );
      std::vector<std::string> vec = mylib::split(hostnameport,':');
      if (vec.size() < 1 || vec.size() > 2 )
         throw std::runtime_error("invalid parameter: " + hostnameport);
      
      std::string hostname = vec[0];
      std::string port = "http";
      if (vec.size() > 1)
            port = vec[1];

      boost::asio::io_service io_service;

      // Get a list of endpoints corresponding to the server name.
      tcp::resolver resolver(io_service);
      tcp::resolver::query query(hostname, port);
      tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

      // Try each endpoint until we successfully establish a connection.
      tcp::socket socket(io_service);
      boost::asio::connect(socket, endpoint_iterator);

      // Form the request. We specify the "Connection: close" header so that the
      // server will close the socket after transmitting the response. This will
      // allow us to treat all data up until the EOF as the content.
      boost::asio::streambuf request;
      std::ostream request_stream(&request);
      request_stream << "GET " << url << " HTTP/1.0\r\n";
      request_stream << "Host: " << hostname << "\r\n";
      request_stream << "Accept: */*\r\n";
      request_stream << "Connection: close\r\n\r\n";

      // Send the request.
      boost::asio::write(socket, request);

      // Read the response status line. The response streambuf will automatically
      // grow to accommodate the entire line. The growth may be limited by passing
      // a maximum size to the streambuf constructor.
      boost::asio::streambuf response;
      boost::asio::read_until(socket, response, "\r\n");

      // Check that response is OK.
      std::istream response_stream(&response);
      std::string http_version;
      response_stream >> http_version;
      unsigned int status_code;
      response_stream >> status_code;
      std::string status_message;
      std::getline(response_stream, status_message);
      if (!response_stream || http_version.substr(0, 5) != "HTTP/")
      {
         DOUT(info << "Invalid response");
         return false;
      }
      if (status_code != 200)
      {
         DOUT(info << "Response returned with status code " << status_code );
         return false;
      }

      // Read the response headers, which are terminated by a blank line.
      boost::asio::read_until(socket, response, "\r\n\r\n");

       // Process the response headers.
      std::string header;
      while (std::getline(response_stream, header) && header != "\r")
         ;
      
      std::ostringstream oss;

       // Write whatever content we already have to output.
      if (response.size() > 0)
         oss << &response;

      // Read until EOF, writing data to output as we go.
      boost::system::error_code error;
      while (boost::asio::read(socket, response,
             boost::asio::transfer_at_least(1), error))
         oss << &response;
      if (error != boost::asio::error::eof)
         throw boost::system::system_error(error);
      result = oss.str();
      return true;
   }
   catch (std::exception& e)
   {
      DOUT(info << "Exception: " << e.what());
   }
   return false;
}
