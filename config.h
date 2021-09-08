#ifndef CONFIG_H
#define CONFIG_H

#include "webserver.h"

using namespace std;

class Config 
{
	public:
	Config();
	~Config(){};

	void parse_arg(int argc, char*argv[]);

        //port number
	int PORT;

        // log write mode
	int LOGWrite;

        //trig mode combination
	int TRIGMode;

        //listenfd trig mode
	int LISTENTrigmode;

        //connfd trig mode
	int CONNTrigmode;

	//close connection
	int OPT_LINGER;

	//database connection pool number
	int sql_num;

        //thread pool thread number
	int thread_num;

        int close_log;

	int actor_model;
};

#endif
