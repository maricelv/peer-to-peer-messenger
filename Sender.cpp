#include "Sender.h"
#include "SenderExceptions.h"

namespace CSE384 
{
   Sender::Sender(SocketConfiguration* sc): sockfd(-1),
                                            sc_(sc),
                                            isSending_(false)
   {}

   void Sender::PostMessage(const Message& m)
   {  
      send_bq_.enQ(m);  
   }

   void Sender::Start()
   {
      if(!IsSending())
      {    
         //start the send the send thread
         IsSending(true);
         send_thread_ = std::thread(&Sender::sendProc, this);
      }
   }

   // dedicated thread function that deques messages 
   // from the blocking queue and writes them into the socket
   void Sender::sendProc()
   {
      try
      {
         //IsSending(true);

         while(IsSending())
         {
            // deque the next message
            Message msg = send_bq_.deQ();
          
            // if this is the stop sending message, signal 
            // the send thread to shutdown
            if(msg.GetType() == STOP_SENDING)
            {
              IsSending(false);
            }
            else
            {
               // serialize the message into the socket
               SendSocketMessage(msg);
            }     
         }
      }
      catch(...)
      {
         IsSending(false);
      }
   }

   // serialize the message header and message and write them into the socket
   void Sender::SendSocketMessage(const Message& msg)
   {
       // convert the wire protocol (message header) to big endian (network byte order)
       struct MSGHEADER mhdr =  *(const_cast<Message&>(msg).GetHeader());
       mhdr.ToNetworkByteOrder();

      // send message header
      if(SendAll(sockfd, (const char*) &mhdr, sizeof(struct MSGHEADER)) == -1)
        throw SenderTransmitMessageHeaderException();

      // send message data
      if(SendAll(sockfd, msg.GetData(), msg.Length()) == -1)
         throw SenderTransmitMessageDataException();
   }

   void Sender::Stop()
   {
      if(IsSending())
      {
        //note: only gets deposited into queue if IsSending is true
        Message stopMsg(nullptr,0,STOP_SENDING);
        send_bq_.enQ(stopMsg);
      }
   }
 
   bool Sender::Close()
   {
      Stop();
      if(send_thread_.joinable())
         send_thread_.join();

      bool ret = true;
      if(IsConnected())
      {   
         ret = (close(sockfd) != -1);
         sockfd = -1;
      }

      return ret; 
   }

   Sender::~Sender()
   {
      // call close in case the client forgot
      Close();
   }

   bool Sender::Connect(const EndPoint& ep)
   {
     //---Open socket for streaming---
     if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
     {
       // perror("Socket");
       sockfd=-1;
       return false;
     }

     //---Initialize server address/port struct
     bzero(&dest, sizeof(dest));
     dest.sin_family = AF_INET;
     dest.sin_port = htons(ep.Port());
     dest.sin_addr.s_addr = inet_addr(ep.IP().c_str());

     //---Connect to server---
     if(connect(sockfd, (struct sockaddr*)&dest, sizeof(dest)) != 0 )
     {
        //perror("Connect ");
        close(sockfd);
        sockfd=-1;
        return false;
     }

     return true;
   }

   int Sender::ConnectPersist(const EndPoint& ep,  unsigned retries,
                             unsigned wtime_secs, unsigned vlevel)
   {
     unsigned int runAttempts = 0;
     while(runAttempts++ < retries)
     {
       if(vlevel)
       {
         std::cout << "Connection attempt # "<< runAttempts <<" of "
                   << retries << " to "<< ep << std::endl;
       }

       if(Connect(ep))
         break;

       if(vlevel)
       {
         std::cout << "Failed Attempt: " << runAttempts << std::endl;
       }
       sleep(wtime_secs);
     }
     return runAttempts;
   }
};

// *** SENDER TEST STUB ****
// ** For testing: run against "Receiver test stub **
#ifdef TEST_SENDER
#include <string>
#include <vector>
using namespace CSE384;

void TestSenderAsync(const std::string& name)
{
   // announce the sender
   std::cout << name << " started";
   
   // specify the server Endpoint we wish to connect
   EndPoint serverEp("127.0.0.1", 6060);
   
   // instantiate a sender 
   Sender sender;

   int wait_secs = 1; // wait time between connection attempts
   int verbose_level = 1;  // verbose (display connect attempts)
   int connect_attempts = 10; // make 10 connection attempts
   if(sender.ConnectPersist(serverEp, connect_attempts, wait_secs, verbose_level) < 10)
    //if(sender.Connect(serverEp)) -- mamkes on attempt to connect
   {
      // start the send thread
      sender.Start();
      int num_messages = 30;
      // loop to send 10 messages 
      for(unsigned int j=0; j < num_messages; j++)
      {
         // build an and print each string message 
         std::string strMsg = name + " [ Message #: " + std::to_string(j+1) + " ]";
         Message msg(strMsg.c_str(),strMsg.length(), MessageType::STRING);
         std::cout << "Message is: " << msg << std::endl;

         // post message into the send queue
         sender.PostMessage(msg); 
         
         // sleep for a second so we can see what's going on
         std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      // all done sending, so shutdown the send thread
      sender.Stop();

      // close the socket:  (important to call close: this signals 
      //                    the server to shutdown the client handler)
      //sender.Close();
   }
   else
      perror("Connect:"); // for now, using c-style error printing
}

int main(int argc, char* argv[])
{
    int senderCount = 1;
    std::vector<std::thread> thread_vec;  // store thread is in vector
    // start senderCount (5) concurrent sender threads to test
    for(int i = 1; i <= senderCount; ++i)
    {
        std::string name;
        if(argc == 1)
          name = "Sender#: -> " + std::to_string(i);
        else
          name = "Sender#: -> " + std::string(argv[1]);
          
        // start sender thread, and store it vector
        thread_vec.push_back(std::thread(TestSenderAsync, name));
    }

    // wait for a send threads to complete: using iterator as an example
    for(auto iter = thread_vec.begin();  iter != thread_vec.end(); ++iter)
    {
       if(iter->joinable())
          iter->join();
    }
    
   return 0;
}
#endif


