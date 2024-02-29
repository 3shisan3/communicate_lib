// todo 服务器通过端口为key可创建多个
// todo 需要实现对workflow中WFWebSocketServer的继承，来添加函数指针的内容

// 参考moquito代码

/**

typedef int (*MOSQ_FUNC_generic_callback)(int, void *, void *);

struct mosquitto__callback{
	UT_hash_handle hh; //  For callbacks that register for e.g. a specific topic
	struct mosquitto__callback *next, *prev; // For typical callbacks
	MOSQ_FUNC_generic_callback cb;
	void *userdata;
	char *data; //e.g. topic for control event
};

类似此种方式，外部再赋予函数指针指向

**/