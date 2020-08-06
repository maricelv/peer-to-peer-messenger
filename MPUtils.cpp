////////////////////////////////////////////////////////////////////////
// MPUtils.cpp - socket definition and function utility module        //
//               capture new (IP agnostic) socket interface for       //
//               MPL                                                  //
// ver 1.0                                                            //
// Language:    Standard C++ (gcc/g++ 7.4)                            //
// Platform:    Dell Precision M7720, Linux Mint 19.3 (64-bit)        //
// Application: CSE 384, MPL (Message passing Layer)                  //
// Author:      Mike Corley, Syracuse University                      //
//              mwcorley@syr.edu                                      //
////////////////////////////////////////////////////////////////////////

#include "MPUtils.h"

namespace CSE384 
{

   EndPoint::EndPoint():  _ip("NIP"), _port(0)
   {}

   EndPoint::EndPoint(const std::string& IP, unsigned int port): _ip(IP), _port(port)
   {}

   EndPoint::EndPoint(const std::string& serialized_ep)
   {
      Parse(serialized_ep);
   }

   bool EndPoint::IsValid() const
   {
       return (_ip != "NIP");
   }

   void EndPoint::Parse(const std::string& serialized_ep)
   {
     try
     {
       //first, get the indices to extract the IP address part
       std::size_t first_ch_index_ip  = serialized_ep.find('(');
       if (first_ch_index_ip == std::string::npos)
         throw BadEndPoint("Can't deserialize EndPoint string");

       std::size_t last_ch_index_ip= serialized_ep.find(',');
       if(last_ch_index_ip == std::string::npos)
	    throw BadEndPoint("Can't deserialize EndPoint string");

       //second, get the indices to extract the port part
       std::size_t last_ch_index_port = serialized_ep.find(')');
       if(last_ch_index_port == std::string::npos)
  	   throw BadEndPoint("Can't deserialize EndPoint string");

       first_ch_index_ip += 1;
       //finally, extract the ip and port
       _ip = serialized_ep.substr(first_ch_index_ip , (last_ch_index_ip-first_ch_index_ip));

       last_ch_index_port -= 1;
       last_ch_index_ip += 1;
       std::istringstream port_str(serialized_ep.substr(last_ch_index_ip, last_ch_index_port));
       port_str >> _port;
     }
     catch(std::exception& ex)
     {
	  _ip = "NIP";
	  _port = 0;
	  throw BadEndPoint(ex.what());
     }
   }


   void EndPoint::SetEndPoint(const std::string& ip, unsigned int port)
   {
	_ip = ip;
	_port = port;
   }


   bool EndPoint::operator == (const EndPoint& ep)  
   {
     if(this->_ip == ep._ip  && this->_port == ep._port)
	 return true;
     return false;
   }


   std::string EndPoint::IP() const
   {
     return _ip;  
   }


   unsigned int EndPoint::Port() const
   {
     return _port;  
   }


   std::string EndPoint::ToString() const  
   { 
     std::ostringstream out;
     out.str("");
     out << (*this);
     return out.str();
   }


   std::ostream& operator << (std::ostream& os, const EndPoint& ep)
   {
	     os << "EndPoint("<<ep.IP()<<","<<ep.Port()<<")";
       return os;
   }


   int SendAll(int sock, const char *block, unsigned int blockLen, int sendRetries, unsigned int wait_time)
   {
     int bytesSent;
     int bytesLeft = blockLen;
     int blockIndx = 0;
     int count = 0;

     while(bytesLeft > 0)
     {
       bytesSent = send(sock,&block[blockIndx],bytesLeft,0);
       if(bytesSent == -1 )
       {
         usleep(wait_time);
         ++count;
       }
       else
       {
         bytesLeft -= bytesSent;
         blockIndx += bytesSent;
       }
       if(count==sendRetries)
         return -1;
      } 
      return blockLen;
   }


   int RecvAll(int sock, const char *block, unsigned int blockLen, int recvRetries, unsigned int wait_time)  
   {
     int bytesRecvd, bytesLeft = blockLen;
     int blockIndx = 0;
     int count = 0;

     while(bytesLeft > 0)
     {
       bytesRecvd = recv(sock,(void*) &block[blockIndx],bytesLeft,0);
       if(bytesRecvd == 0)
	   return 0;

       if(bytesRecvd == -1 )
       {
         usleep(wait_time);
         ++count;
       }
       else
       {
    	   bytesLeft -= bytesRecvd;
    	   blockIndx += bytesRecvd;
       }
      if(count==recvRetries)
        return -1;
     }
     return blockLen;
   }
};

#ifdef TEST_MPUTILS
int main()
{
    std::cout<<"*** Testing MPUtils ***"<<std::endl;

    std::cout<<"Testing EndPoint class"<<std::endl;

    CSE384::EndPoint ep("10.10.25.2", 5050);
    CSE384::EndPoint ep2 = ep;

    std::cout<<"Test == operator"<<std::endl;
    if(ep == ep2)
      std::cout<<"Correct ep is equal to ep2"<<std::endl;
    else
     std::cout<<"Wrong!! ep and ep2 are indeed equal"<<std::endl;

    QSI::EndPoint ep3("10.10.25.2", 5051);
    if(ep == ep3)
       std::cout<<"Wrong ep is not equal to ep3"<<std::endl;
    else
      std::cout<<"Correct!! ep and ep3 really are not equal"<<std::endl;

    std::cout<<"Test accessors"<<std::endl;
    std::cout<<"IP="<<ep.IP()<<" "<<ep.Port()<<std::endl;
    std::cout<<"Test ToString(): "<<ep.ToString()<<std::endl;
    std::cout<<"Test insertion operator: "<<ep<<std::endl;

    try
    {

       CSE384::EndPoint deserialized_ep1(ep3.ToString());
       std::cout << "Deserialized EP IP address: " << deserialized_ep1.IP()   << std::endl;
       std::cout << "Deserialized EP Port      : " << deserialized_ep1.Port() << std::endl;

    	CSE384::EndPoint deserialized_ep("EndPoint(123.456.789.000,2020)");
       std::cout << "Deserialized EP IP address: " << deserialized_ep.IP()   << std::endl;
       std::cout << "Deserialized EP Port      : " << deserialized_ep.Port() << std::endl;

       deserialized_ep.Parse("EndPoint(1.2.3.4,9999)");
       std::cout << "Deserialized EP IP address: " << deserialized_ep.IP()   << std::endl;
       std::cout << "Deserialized EP Port      : " << deserialized_ep.Port() << std::endl;

       deserialized_ep.Parse("EndPoint(1.2.3.4,9999");
       std::cout << "Deserialized EP IP address: " << deserialized_ep.IP()   << std::endl;
       std::cout << "Deserialized EP Port      : " << deserialized_ep.Port() << std::endl;

       deserialized_ep.Parse("EndPoint(1.2.3.4,99!9)");
       std::cout << "Deserialized EP IP address: " << deserialized_ep.IP()   << std::endl;
       std::cout << "Deserialized EP Port      : " << deserialized_ep.Port() << std::endl;
    }
    catch(CSE384::BadEndPoint& bex)
    {
    	std::cerr << "Caught BadEndPoint:" << std::endl;
    	std::cerr<< bex.what() << std::endl;
    }


    std::cout<<"ToString():  "<<QSI::ToString(ep3.Port())<<std::endl;
    return 0;
}
#endif





