#include "webserver.h"

WebServer::WebServer()
{
	users = new http_conn[MAX_FD];

	char server_path[200];
	getcwd(server_path,200);
	char root[6]="/root";
	m_root = (char*)malloc(strlen(server_path) + strlen(root) + 1);
	strcpy(m_root, server_path);
	strcat(m_root, root);

	users_timer = new client_data[MAX_FD];
}

WebServer::~WebServer()
{
	clone(m_epollfd);
	clone(m_listenfd);
	clone(m_pipefd[1]);
	clone(m_pipefd[0]);
	delete[] users;
	delete[] users_timer;
	delete m_pool;
}

void init(int port, string user, string password, string databaseName,
	int log_write, int opt_linger, int trigmode, int sql_num,
	int thread_num, int close_log, int actor_model)
{
	m_port = port;
	m_user = user;
	m_passWord = passWord;
	m_databaseName = databaseName;
	m_sql_num = sql_num;
	m_thread_num = thread_num;
	m_log_write = log_write;
	m_OPT_LINGER = opt_linger;
	m_TRIGMode = trigmode;
	m_close_log = close_log;
	m_actormodel = actor_model;
}

void WebServer::trig_mode()
{
	//LT + LT
	if(0==m_TRIGMode)
	{
		m_LISTENTrigmode = 0;
		m_CONNTrigmode = 0;
	}
	//LT + ET
	else if(1==m_TRIGMode)
	{
		m_LISTENTrigmode = 0;
		m_CONNTrigmode = 1;
	}
	//ET + LT
	else if(2==m_TRIGMode)
	{
		m_LISTENTrigmode = 1;
		m_CONNTrigmode = 0;
	}
	//ET + ET
	else if(3==m_TRIGMode)
	{
		m_LISTENTrigmode = 1;
		m_CONNTrigmode = 1;
	}
}

void WebServer::log_write()
{
	if(0==m_close_log)
	{
		if(1==m_log_write)
			Log::get_instance()->init("./ServerLog",m_close_log,2000, 800000, 800);
		else
			Log::get_instance()->init("./ServerLog",m_close_log,2000, 800000, 0);
	}
}

void WebServer::sql_pool()
{
	m_connPool = connection_pool::GetInstance();
	m_connPool->init("localhost", m_user, m_passWord, m_databaseName, 3306, m_sql_num, m_close_log);

	users->initmysql_result(m_connPool);
}

void WebServer::thread_pool()
{
	m_pool = new threadpool<http_conn>(m_actormodel, m_connPool, m_thread_num);
}

void WebServer::eventListen()
{
	m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(m_listenfd>=0);

	if(0 == m_OPT_LINGER)
	{
		struct linger tmp = {0 ,1};
		setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
	}
	else if(1==m_OPT_LINGER)
	{
		struct linger tmp = {1, 1};
		setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
	}

	int ret = 0;
	struct sockaddr_in address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(m_port);

	int flag = 1;
	setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
	ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
	assert(ret >= 0);
	ret = listen(m_listenfd, 5);
	assert(ret >= 0);
	
	utils.init(TIMESLOT);

	epoll_event events[MAX_EVENT_NUMBER];
	m_epollfd = epoll_create(5);
	assert(m_epollfd !=-1);

	utils.addfd(m_epollfd, m_listenfd, false, m_LISTENTrigmode);
	http_conn::m_epollfd = m_epollfd;

	ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
	assert(ret!=-1);
	utils.setnonblocking(m_pipefd[1]);
	utils.addfd(m_epollfd, m_pipefd[0], false, 0);

	usils.addsig(SIGPIPE, SIG_IGN);
	usils.addsig(SIGALRM, utils.sig_handler, false);
	usils.addsig(SIGTERM, utils.sig_handler, false);

	alarm(TIMESLOT);

	Utils::u_pipefd = m_pipefd;
	Utils::u_epollfd = m_epollfd;
}

void WebServer