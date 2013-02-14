#include "tcpsocket.hh"
#include "threadclass.h"

#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include <assert.h>
#include <sstream>
#include <list>

using namespace std;



class CompileThread: public MyThreadClass
{
public:
  CompileThread( Network::TcpSocket *pcClientSocket ) : pcClientSocket(pcClientSocket) {
  }

  
  void recvString(std::string& str) {
    *pcClientSocket >> str;
    std::cout << "received: " << str << "\n";
  }

  void sendAck() {
    *pcClientSocket << "ok\n";
  }

  virtual void InternalThreadEntry() {

    std::string fname_dest;
    std::string fname_src;
    std::string path_qdp;
    std::string jit_options;
    std::string cuda_dir;
    std::string qdp_gpuarch;

    recvString(fname_dest); sendAck();
    recvString(fname_src); sendAck();
    recvString(path_qdp); sendAck();
    recvString(jit_options); sendAck();
    recvString(cuda_dir); sendAck();
    recvString(qdp_gpuarch); // last Ack sent when build has finished!

    std::ostringstream jit_command;

    jit_command << string(cuda_dir) << "/bin/nvcc " << jit_options;
    jit_command << " -arch=" << string(qdp_gpuarch);
    jit_command << " -m64 --compiler-options -fPIC,-shared -link ";
    jit_command << fname_src << " -I" << path_qdp << "/include -o " << fname_dest;
    
    string gen = jit_command.str();
    printf("%s",gen.c_str());

    char* cmd = new(nothrow) char[gen.size()+1];
    if (!cmd) {
      printf("no memory for jitter command"); exit(1);
    }
    strcpy(cmd,gen.c_str());

    int ret;

    if (ret=system(cmd)) {

      cout << "failed cmd = " << (void*)cmd << endl;

      printf("Problems calling nvcc jitter:");

      if (WIFEXITED(ret)) {
	printf("exited, status=%d\n", WEXITSTATUS(ret));
      } else if (WIFSIGNALED(ret)) {
	printf("killed by signal %d\n", WTERMSIG(ret));
      } else if (WIFSTOPPED(ret)) {
	printf("stopped by signal %d\n", WSTOPSIG(ret));
      } else if (WIFCONTINUED(ret)) {
	printf("continued\n");
      } else {
	printf("not recognized\n");
      }

      cout << "Checking shell.. ";
      if(system(NULL))
	cout << "ok!\n";
      else
	cout << "nope!\n";

      if (WEXITSTATUS(ret) != 127) {
	printf("This error is not the hw/kernel bug that we have a workaround. giving up");
	exit(1);
      }

      printf("This error is the hw/kernel bug");

      list<char*> listPtr;
      int maxtry=1000;
      do{
	cout << maxtry << " failed cmd = " << (void*)cmd << endl;

	cout << "Problems calling nvcc jitter: ";

	if (WIFEXITED(ret)) {
	  printf("exited, status=%d\n", WEXITSTATUS(ret));
	} else if (WIFSIGNALED(ret)) {
	  printf("killed by signal %d\n", WTERMSIG(ret));
	} else if (WIFSTOPPED(ret)) {
	  printf("stopped by signal %d\n", WSTOPSIG(ret));
	} else if (WIFCONTINUED(ret)) {
	  printf("continued\n");
	} else {
	  printf("not recognized\n");
	}

	cout << "alloc " << gen.size()+1 << endl;
	char* tmp = new(nothrow) char[gen.size()+1];
	if (!tmp) {
	  printf("no memory for jitter command");
	  exit(1);
	}
	strcpy(tmp,gen.c_str());
	listPtr.push_back( tmp );

	printf("%s\n",listPtr.back());
	  
      } while ((ret=system(listPtr.back())) && (--maxtry>0));

      int ii=0;
      while(listPtr.size()) {
	cout << "deleting " << ii++ << endl;
	delete[] listPtr.back();
	listPtr.pop_back();
      }

      if (ret) {
	printf("Nvcc error\n"); exit(1);
      }
      else
	cout << "system call kernel/hardware bug: workaround applied !" << endl;

    }
    delete[] cmd;

    sendAck();

    printf("\nClosing Client Socket...\n");
    delete pcClientSocket;
  }


private:
  Network::TcpSocket *pcClientSocket;
};




void
TCPCompileServer(int iPortNumber)
{
  Network::TcpSocket  server;
  Network::TcpSocket *client;

  std::vector<CompileThread*> vecThreads;

  try {
    server.connect(iPortNumber);
    server.add_delim("\n");
    server.add_delim("\r\n");
  }
  catch (Network::Exception &e)
    {
      std::cerr << e;
      exit (1);
    }
  std::cout << "Binded on port [" << iPortNumber << "]" << std::endl;

  for (;;)
    {
      printf("Waiting for Connection...\n");

      client = server.accept();
      std::cout << "Ip [" << server.get_ip(client) << "]" << std::endl;
      client->add_delim("\n");
      client->add_delim("\r\n");

      vecThreads.push_back( new CompileThread( client ) );

      vecThreads.back()->StartInternalThread();
    }

}





int main(int argc, char **argv)
{
  if (argc != 1 && argc != 2)
    {
      std::cerr << "Usage : " << argv[0] << " [port]" << std::endl;
      exit (1);
    }

  int port = 1500;
  if (argc == 2 )
    port = strtol(argv[1], 0, 10);

  std::cout << "COMPILESERVER_PID " << getpid() << "\n";

  TCPCompileServer(port);
}
