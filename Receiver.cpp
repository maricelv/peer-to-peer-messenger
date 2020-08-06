#include "Receiver.h"
#include "ReceiverExceptions.h"

namespace CSE384
{ 
     Receiver::Receiver(const EndPoint& ep, SocketConfiguration* sc): sc_(sc), 
                                                                     listensockfd_(-1), 
                                                                     ch_(nullptr),
                                                                     islistening_(false)
     {               
        //note: old socket interface (no IPv6 support) 
        if ((listensockfd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
           throw ReceiverCreateSocketException();
           
        // Initialize address/port structure
        bzero(&self, sizeof(self));
        self.sin_family = AF_INET;
        self.sin_port = htons(ep.Port());
        self.sin_addr.s_addr =  inet_addr(ep.IP().c_str());
        //self.sin_addr.s_addr = INADDR_ANY;

        // Assign a port number to the socket
        if(bind(listensockfd_, (struct sockaddr*)&self, sizeof(self)) != 0 )
          throw ReceiverBindException();

        //set sockt socket listening 
        //if(listen(listensockfd_, 20) != 0 )
        //   ReceiverListenException();
     }

     void Receiver::Start(int backlog)
     {
        if(!IsListening())
           listenThread_ = std::thread(&Receiver::ListenThreadProc, this, backlog);
     }

     void Receiver::ListenThreadProc(int backlog)
     {
        struct sockaddr_in client_addr;
        socklen_t addrlen=sizeof(client_addr);
        try
        {
           // set sockt socket listening 
           if(listen(listensockfd_, backlog) != 0)
             ReceiverListenException(); 
       
           IsListening(true);

           // loop around accepting
           while (IsListening())
           {
              // ---accept a connection (creating a data pipe)---
              int clientfd = accept(listensockfd_, (struct sockaddr *)&client_addr, &addrlen);
              if (clientfd > 0)
              {
                 // std::cout << "Hello New Client From: " << inet_ntoa(client_addr.sin_addr) << " : "
                 //           << ntohs(client_addr.sin_port) << std::endl;

                 if (ch_ != nullptr)
                 {
                    //clone the registered client hander, and hand the socket to the client handler
                    ClientHandler *ch = ch_->Clone();

                    // spawn a thread to service the current client request
                    std::thread clientThread = std::thread(&Receiver::ServiceClient, this, ch, clientfd);
                    clientThread.detach();
                 }
                 else
                 {
                    throw ReceiverNoRegisteredClientHandlerException();
                 }
              }
           }
        }
        catch (const std::exception &e)
        {
           IsListening(false);
        }
     }

      void Receiver::ServiceClient(ClientHandler* ch, int sock_fd)
      {
         try
         {
            ch->SetSocket(sock_fd);

            //start the client processing thread
            std::thread clientThread = std::thread(&ClientHandler::RecvProc, ch);

            // start the user defined AppProc() on a new thread
            // std::thread app_thread_ = std::thread(&ClientHandler::AppProc, ch);
            ch->AppProc();

            if(clientThread.joinable())
               clientThread.join();
         }
         catch (std::exception &ex)
         {
            std::cerr << ex.what() << std::endl;
         }

         // make sure the client handler resources are freed 
         if(ch != nullptr)
         { 
            ch->Close();
            delete ch;
         }
      }

      void Receiver::RegisterClientHandler(ClientHandler* ch)
      {
         ch_ = ch;
      }
 
      void Receiver::Stop()
      {
         // if user started the listener, then shut it all down
         if(IsListening())
         {
            try
            {
               IsListening(false);
               close(listensockfd_);
               listenThread_.detach();
            }
            catch (...)
            {
               IsListening(false);
            }
         }
      }

      Receiver::~Receiver()
      {
         // call Stop on the listening thread in case the client forgot
         Stop();
      }
 };

// ***  RECEIVER TEST STUB ***
 #ifdef TEST_RECEIVER
  /*  Note: use either "SenderTest" for test */
   using namespace CSE384;
  
   // this is an example of custom server processing class
   class TestClientHandler : public ClientHandler
   {
       public:

       // this is a creational function: you must implement it
       // the Receiver creates an instance of this class (it's how the server polymorphism works) 
       virtual ClientHandler* Clone()
       {
          return new TestClientHandler();
       }

       // this is where you define the custom server processing: you must implement it
       virtual void AppProc()
       {
          // while there are messages in the blocking queue, and you have seen the disconnect
          // message, pull messages out and dispaly them.
          Message msg;
          while( (msg = GetMessage()).GetType() != MessageType::DISCONNECT)
          {
             std::cout << "Got a message from: " << msg << std::endl;
          }
       }

       ~TestClientHandler()
       {
          std::cout << "Handler destroyed" << std::endl;
       }
   };


   int main(int argc, char* argv[])
   {   
      // define server endpoint ip address and port for listening
      EndPoint serverEP("127.0.0.1", 6060);

      // define instance of custom server processing (ccustom client handler) 
      TestClientHandler th;

      // define instance of the receiver (server host)
      Receiver receiver(serverEP);

      // register the custom client handler with receiver instance 
      receiver.RegisterClientHandler(&th);

      // start the server listening thread 
      receiver.Start();
      std::cout << "Server listening on: " << serverEP << std::endl;
      std::cout << "Press any key to quit" << std::endl;
      std::cin.get();

      //shut it listener and quit
      receiver.Stop();
      
      return 0;
   }
 #endif
