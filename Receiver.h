/////////////////////////////////////////////////////////////////////////////
// Receiver.h -   multithreaded (concurrent) server framework              //
//                framework protoype                                       //
// ver 1.0                                                                 //
// Language:    Standard C++ (gcc/g++ 7.4)                                 //
// Platform:    Dell Precision M7720, Linux Mint 19.3 (64-bit)             //
// Application: CSE 384, MPL (Message passing Layer)                       //
// Author:      Mike Corley, Syracuse University                           //
//              mwcorley@syr.edu                                           //
/////////////////////////////////////////////////////////////////////////////
/*
 * Package Operations:
 * ===================
 *  This package serves as the basis for providing efficient 
 *  light-weight customizable TCP/IP server framework, allowing
 *  the client (executive) to build custom TCP Servers
 *  This package assumes the target operating system (Linux) is POSIX standard 
 *  compliant. i.e. standard I/O, sockets (IPv4 - old interface) I/O, and 
 *  Threading model. 
 
 *  USAGE:  See Receiver test stub

 * Required Files:
 * ==============
 * Receiver.h, MPUtils.h, Receiver.cpp, Message.cpp, MPUtils.cpp
 *
 * Build Process: (for test stub)
 * =============
 * Linux:  g++ -DTEST_RECEIVER -o ReceiverTest Receiver.cpp Message.cpp MPUtils.cpp ClientHandler.cpp -lpthread 
 *
 * Maintenance:
 * ===========
 *  ver 1.0 : 29 March 2019
 *  -- first release 
*/

#ifndef _Receiver_h_
#define _Receiver_h_

#include <string>
#include <thread>
#include <atomic>

#include "MPUtils.h"
#include "Cpp11-BlockingQueue.h"
#include "Message.h"
#include "ClientHandler.h"

namespace CSE384
{
   class Receiver
   {
      public:
        Receiver(const EndPoint& ep, SocketConfiguration* sc = nullptr);
        ~Receiver(); 
        void RegisterClientHandler(ClientHandler* ch);
        void Start(int backlog=20);
        void Stop();
        bool IsListening();

        // prevent users from making copies of Receiver ojects  
        Receiver(const Receiver&) = delete;
        Receiver& operator=(Receiver&) = delete;

      private:
        virtual void ServiceClient(ClientHandler* ch, int sock_fd);

        void ListenThreadProc(int backlog);
        void Initialize(const char* ip, unsigned int port);
        void IsListening(bool listening);  

        SocketConfiguration* sc_;
        int listensockfd_;
        ClientHandler* ch_;
        std::atomic<bool> islistening_;
        std::thread listenThread_;
        struct sockaddr_in self;
   };

   inline bool Receiver::IsListening()
   {
      return islistening_.load();
   }

   inline void Receiver::IsListening(bool listening)
   {
      islistening_.store(listening); 
   }
}
#endif 

