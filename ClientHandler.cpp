#include "ClientHandler.h"

namespace CSE384
{
    // RecvProc is a dedicated thread for servicing the socket
    // by pulling out messasges and enQing in the recv blocking queue
    void ClientHandler::RecvProc()
    {
        try
        {
          IsReceiving(true);

          while(IsReceiving())
          {
             Message msg = RecvSocketMessage();
             recv_queue_.enQ(msg);
             if(msg.GetType() == MessageType::DISCONNECT)
             {
                IsReceiving(false);
             }
          }
        }
        catch(const std::exception& ex)
        {
           std::cerr << ex.what() << std::endl;
           IsReceiving(false);
        }
    }

    // serialize the message header and message and write them into the socket
    Message ClientHandler::RecvSocketMessage()
    {   
        struct MSGHEADER mhdr;
        int recv_bytes;
        // receive fixed size message header (see wire protocol in Message.h)
        if(  (recv_bytes = RecvAll(data_sock_fd_, (const char*) &mhdr, sizeof(MSGHEADER))) == sizeof(MSGHEADER))
        {
          // *** MUST convert message header to host byte order (e.g. Intel CPU == little endian)
          mhdr.ToHostByteOrder();
        
          //construct a Message using the Message header read from the socket channel
          // *** critical that mhdr is host byte order ****
          Message msg(mhdr);
        
          // send message data
          if(RecvAll(data_sock_fd_, msg.GetData(), msg.Length()) == -1)
            throw ReceiverReceiveMessageDataException();
        
          return msg;
        }

        // if read zero bytes, thyen this is the zero length message signaling client shutdown
        if(recv_bytes == 0)
        {
          return Message(NULL,0,DISCONNECT);
        }
        else
        {
          throw ReceiverReceiveMessageHeaderException();
        }
    }   
}