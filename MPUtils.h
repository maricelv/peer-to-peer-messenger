////////////////////////////////////////////////////////////////////////
// MPUtils.h -   variety of socket definitions and function utilities //
//               for acheiving wire protocol (MPL)  compatibility     //
// ver 1.0                                                            //
// Language:    Standard C++ (gcc/g++ 7.4)                            //
// Platform:    Dell Precision M7720, Linux Mint 19.3 (64-bit)        //
// Application: CSE 384, MPL (Message passing Layer)                  //
// Author:      Mike Corley, Syracuse University                      //
//              mwcorley@syr.edu                                      //
////////////////////////////////////////////////////////////////////////
/*
 * Package Operations:
 * ===================
 *  Variety of utilties to support message passing over sockets.  Session oriented Sockets 
 *  return asynchronously in response to TCP flow control. i.e. Socket reads and writes
 *  will return with the post condition of having proceessed less than or equal to the 
 *  number of bytes requested.  We must therefore "poll" the socket to pull out (frame)
 *  complete messages.  The two functions below provide this
 * 
 *
 * Required Files:
 * ==============
 * MPUtils.h, MPUtils.cpp
 *
 * For Test Stub Build also requires: (MPUtils.h/.cpp, Thread.h/.cpp
 *
 * Build Process: (for test stub)
 * =============
 * Linux:  g++ -DTEST_SMPUTILS -o MPUtilsTest MPUtils.cpp 
 *
 * Maintenance:
 * ===========
 *  ver 1.0 : 29 March 2019
 *     -- first release
 *
*/

#ifndef MPUTILS_H_
#define MPUTILS_H_

#include <iostream>
#include <string>
#include <sstream>


#include <unistd.h>     // Symbolic Constants 
#include <sys/types.h>  // Primitive System Data Types  
#include <stdlib.h>     // General Utilities 
#include <pthread.h>    // POSIX Threads 

//variety of socket headers
#include <sys/socket.h>
#include <resolv.h>
#include <arpa/inet.h>
#include <errno.h>
#include <strings.h>

namespace CSE384
{
    // interface for deferring socket options to the application
    // (for client and server side)
    class SocketConfiguration
    {
     public:
	      virtual int SetSocketOptions(int sock_fd) = 0;
        virtual ~SocketConfiguration() 
        {}
    };

    class BadEndPoint : public std::exception
    {
      public:
        BadEndPoint(const char* str): str_(str)
        {}

        virtual const char* what() const throw()
        {
      	  return str_;
        }

       ~BadEndPoint() throw() {}
     protected:
        const char* str_;
    };


  class EndPoint
  {
    public:
      EndPoint();
      EndPoint(const std::string& IP, unsigned int port);
      EndPoint(const std::string& serialized_ep);
      void SetEndPoint(const std::string& ip, unsigned int port);
      bool IsValid() const;
      void Parse(const std::string& serialized_ep);
      bool operator == (const EndPoint& ep);
      std::string IP() const;
      unsigned int Port() const;
      std::string ToString() const;
    private:
      std::string _ip;
      unsigned int _port;
  };

  template<typename T>
  inline std::string ToString(const T& t)
  {
    std::ostringstream convert;   // stream used for the conversion
    convert << t;
    return convert.str();
  }

  std::ostream& operator << (std::ostream& os, const EndPoint& ep);


  int SendAll(int sock_fd, const char* block, unsigned int blockLen, int sendRetries=1, unsigned wait_time=1);
  int RecvAll(int sock_fd, const char* block, unsigned int blockLen, int recvRetries=1, unsigned wait_time=1);
}

#endif /* MPUtils_H_ */
