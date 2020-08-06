#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <string>
#include <thread>
#include <atomic>

#include "MPUtils.h"
#include "Cpp11-BlockingQueue.h"
#include "Message.h"
#include "ReceiverExceptions.h"

////////////////////////////////////////////////////////////////////////////
// ClientHandler.h - Defines customizable server side processing          //
// Language:    Standard C++ 11 (g++ 7.4),                                //
// Platform:    Dell Precision m7720, MINT Linux 19.3 (64-bit)            //
// Application: Project 2 helpers                                         //
// Author:      Mike Corley, Syracuse University                          //
//              mwcorley@syr.edu                                          //
////////////////////////////////////////////////////////////////////////////

/*
 * Module Operations
 * =================
 * This module provides the ClientHandler protocol used define custom server side
 * processing.    Receiver (server) hosts the client handler by registering the 
 * it through a base class pointer.  This a allows to easily change the processing.
 * Client users must implement a class that derives from ClientHandler get application 
 * specific server behavior. The client must (though public inheritance), derive 
 * from ClientHandler and provide an override for "Clone()" and "AppProc()". 
 * "Clone()" provides the creation function for creating the ClientHandler instances 
 * used to service client requests.  The clients implements AppProc()" 
 * to define the application specific processing code.
 * 
 * See Receiver test stub for build information and testing
 */

namespace CSE384
{
    // ClientHandler Interface:  subclass this to get custom behavior
    class ClientHandler
    {
       public: 
         friend class Receiver; // let Receiver access private state
         ClientHandler();
         void SetSocket(int socket_fd);
         int Close();
         bool IsReceiving();
         
         Message GetMessage();
         
         // pure virtual function: must implement 
         virtual void AppProc() = 0;
         virtual ClientHandler* Clone() = 0;
         virtual ~ClientHandler();

         // prevent users from making explicit copies of ClientHandler ojects  
        ClientHandler(const ClientHandler&) = delete;
        ClientHandler& operator=(ClientHandler&) = delete;

       private:
         void IsReceiving(bool receiving);  
         virtual void RecvProc();
         virtual Message RecvSocketMessage();
         std::atomic<bool> isReceiving_;
         std::thread recvThread;
         int data_sock_fd_;
         BlockingQueue<Message> recv_queue_;
    };

    inline ClientHandler::ClientHandler(): data_sock_fd_(-1)
    {}

    inline int ClientHandler::Close()
    {
       return close(data_sock_fd_);
    }

    inline ClientHandler::~ClientHandler()
    {}

    inline Message ClientHandler::GetMessage()
    {
       return this->recv_queue_.deQ();
    }

    inline void ClientHandler::SetSocket(int socket_fd)
    {
      data_sock_fd_ = socket_fd;
    }

    inline bool ClientHandler::IsReceiving()
    {
       return isReceiving_.load();
    }

    inline void ClientHandler::IsReceiving(bool receiving)
    {
       isReceiving_.store(receiving);
    }
}

#endif 