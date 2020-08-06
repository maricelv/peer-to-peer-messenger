#include <iostream>
#include <string>
#include <mutex>

// include the message passing library header
#include "mpl.h"

using namespace std;
using namespace CSE384;

// these are the message types used by the sender and the MPeers (you can add more if you want to extend your MPeer)
// DISPLAY_MESSAGE a message type that tells a peer to display its messages
// PEER_MESSAGE, a message type that specifies a message is from another peer
using MessengerType = enum { DISPLAY_MESSAGE = 1,
                             PEER_MESSAGE = 2 };

// MessageHander
class MessageHandler : public ClientHandler
{
public:
   // this is a creational function
   // the Receiver creates an instance of this class (it's how the server polymorphism works)
   virtual ClientHandler *Clone()
   {
      return new MessageHandler();
   }

   // *** Project 2:  Complete this function at the points specified ****
   // this is where you define the server processing for the Messenger you must implement it
   virtual void AppProc()
   {
      // while there are messages in the blocking queue, and you have seen the disconnect
      // message, pull messages out and dispaly them.
      Message msg = GetMessage();

      // while the client hasn't disconnected, keep receiving messages
      while (msg.GetType() != MessageType::DISCONNECT)
      {
         switch (msg.GetType())
         {
           // this is a message from another peer
           case MessengerType::PEER_MESSAGE:
           {
             // the vector is static which makes it shared by 
             // all (concurrent) instances of the MessageHandler. 
             //therefore, we need to lock it with a mutex 
             std::lock_guard<std::mutex> l(m);
             // *** project2:  insert the message into vector here *** //
             messages.push_back(msg.ToString());
           }
           break;

           // this is a request message to print all of the received messages
           case MessengerType::DISPLAY_MESSAGE:
           {
              std::lock_guard<std::mutex> l(m);
              // *** Project 2: display all messages received in the vector ****
              // again, we need to lock the vector with a mutex because it shared 
              // by multiple concurrent MessageHandlers
              for (auto mg = messages.begin(); mg != messages.end(); ++mg) {
                // print a pointer of the variable mg
                std:cout << *mg << std::endl;
              }
           }
           break;
         }

         // get the next message of the queue
         msg = GetMessage();
      }
   }
 private:
   // need a static mutex to synchronize multiple clients writing messages
   // into the vector
   static mutex m;

   //need to make the vector static, so it's shared among all connected peers
   static vector<string> messages;
};

//static definitions of the MessageHandler class
mutex MessageHandler::m;
vector<string> MessageHandler::messages;

// *** Project 2:  Complete this function to send messages to MPeers****
bool SendMessage(const Message &message, const EndPoint &dstPeerEP)
{
   // complete the code to send messages to MPeers,
   // using an instance of Sender messages.
   // Look at the test Sender package stub (bottom of file: Sender.cpp)
  
   Sender messages;

   // connect to client
   if (messages.Connect(dstPeerEP))
   {
    messages.Start();
    // check to see if start() was successful and send thread is active
    if (messages.IsSending())
    {
      // send message
      messages.PostMessage(message);

      messages.Stop();
      return true;
    }
   }
   return false;
}

   // this function sends a message to a Peer asking it to display it's messages to the screen
   // usually an MPeer uses this to ask it's server part (Receiver->MessageHandler) to print its 
   // messages  
   void DoDisplayMessages(const EndPoint& dstEP)
   {
      // contruct message to send to another a messenger (at endpoint dstEP) asking it to display 
      // its messages from its peers, note: this message doesn't have content, only a message type.
      // Thats why the message data is set to nullptr, and the length set to 0
      Message displayMsg(nullptr, 0, MessengerType::DISPLAY_MESSAGE);
      
      // send the message to destination peer
      SendMessage(displayMsg, dstEP);
   }


   // handles prompting the user for input, and sending messages to remote Peers 
   void DoSendMessage()
   {
       // lock_guard<mutex> l(ioLock);
       std::string str;
       string destIP = "";
       int destPort;
       
       cout << "Type Message: -> ";
       getline(std::cin, str, '\n');

       cout << "Enter Destination Messenger IP Address: ( Press d - for default localhost - 127.0.0.1)";
       cin >> destIP;
       if(tolower(destIP[0]) == 'd') 
         destIP = "127.0.0.1";

       cout << "Enter Destination Messenger IP Port: ";
       cin >> destPort;

       //guard if user types in a bad port number (like a string or something with no digits)
       //while(cin.fail())
       //{
       //   cin >> destPort;
       //   cin.clear();
       //}
      
       EndPoint destEP(destIP, destPort);
       
       if(SendMessage(Message(str, MessengerType::PEER_MESSAGE), destEP))
          std::cout << "Message Sent to: " << destEP << std::endl;
       else
          std::cout << "Message Sending Failed" << destEP << std::endl;
   }


   char DisplayMenu()
   {
      std::cout << std::endl;
      std::cout << "************************" << std::endl;
      std::cout << "* S - Send a Message   *" << std::endl;
      std::cout << "* D - Display Messages *" << std::endl;
      std::cout << "* Q - Quit             *" << std::endl;
      string str;
      getline(std::cin, str, '\n');
      return tolower(str[0]);
   }

   int main(int argc, char *argv[])
   {
      if (argc != 3)
      {
         std::cerr << "Usage: My-IP-Address My-Service-Port" << std::endl;
         return 0;
      }

      // get the IP address and port for Messenger listening server
      string my_ip_address = std::string(argv[1]);
      int my_port = std::stoi(argv[2]);

      // turn the IP/port into an endpoint use for 
      EndPoint myEndPoint(my_ip_address, my_port);

      // Custom client handler for defining 
      // Messenger client's message handler (this is the server-side of the Messenger)
      MessageHandler th;

      // define instance of the receiver (server host)
      Receiver receiver(myEndPoint);

      // register the Messenger client's message code handler code with receiver host
      receiver.RegisterClientHandler(&th);

      // start the server listening
      receiver.Start();

      while (true)
      {
         char ch = DisplayMenu();
         if (ch == 'q')
            break;

         switch (ch)
         {
          case 's':
            // send a message to another peer
            DoSendMessage(); 
            break;
          case 'd':
            // display all of my messages by sending my endpoint
            DoDisplayMessages(myEndPoint); // display all messages received
            break;
         }
      }

      // user pressed q - quit, so stop the listening server
      receiver.Stop();
      return 0;
   }
