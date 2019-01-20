#include "HttpManager.h"
#include "HttpRequest.h"
#include <iostream>
#ifdef TRACE_CLASS_MEMORY_ENABLED
#include "ClassMemoryTracer.h"
#endif


CURLSH* HttpManager::s_share_handle_ = nullptr;

HttpManager::HttpManager()
	: m_lock(new TPLock)
{
#ifdef TRACE_CLASS_MEMORY_ENABLED
	TRACE_CLASS_CONSTRUCTOR(HttpManager);
#endif

	curl_global_init(CURL_GLOBAL_DEFAULT);
	s_share_handle_ = curl_share_init();
	curl_share_setopt(s_share_handle_, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);

	ThreadPool::globalInstance()->init();
}

HttpManager::~HttpManager()
{
	std::cout << __FUNCTION__ << "(B)" << std::endl;
#ifdef TRACE_CLASS_MEMORY_ENABLED
	TRACE_CLASS_DESTRUCTOR(HttpManager);
#endif

	globalCleanup();
	m_lock.reset();

	curl_share_cleanup(s_share_handle_);
	curl_global_cleanup();

	std::cout << __FUNCTION__ << "(E)" << std::endl;
}

HttpManager* HttpManager::globalInstance()
{
	static HttpManager _instance;
	return &_instance;
}

void HttpManager::globalInit()
{
	HttpManager::globalInstance();
}

void HttpManager::globalCleanup()
{
	HttpManager::globalInstance()->clearReply();
	ThreadPool::globalInstance()->waitForDone();
}

bool HttpManager::addTask(std::shared_ptr<TaskBase> t, ThreadPool::Priority priority)
{
	if (!t.get())
	{
		return false;
	}
	return ThreadPool::globalInstance()->addTask(t, priority);
}

bool HttpManager::abortTask(int task_id)
{
	return ThreadPool::globalInstance()->abortTask(task_id);
}

bool HttpManager::abortAllTask()
{
	HttpManager::globalInstance()->clearReply();
	return ThreadPool::globalInstance()->abortAllTask();
}

void HttpManager::clearReply()
{
	TPLocker locker(m_lock);
	m_map_replys.clear();
}

void HttpManager::set_share_handle(CURL* curl_handle)
{
	if (curl_handle && s_share_handle_)
	{
		curl_easy_setopt(curl_handle, CURLOPT_SHARE, s_share_handle_);
		curl_easy_setopt(curl_handle, CURLOPT_DNS_CACHE_TIMEOUT, 60 * 5);
	}
}

void HttpManager::addReply(std::shared_ptr<HttpReply> reply)
{
	TPLocker locker(m_lock);
	m_map_replys[reply->id()] = reply;
}

void HttpManager::removeReply(int id)
{
	TPLocker locker(m_lock);

	auto iter = m_map_replys.find(id);
	if (iter != m_map_replys.end())
	{
		m_map_replys.erase(iter);
	}
}

std::shared_ptr<HttpReply> HttpManager::takeReply(int id)
{
	std::shared_ptr<HttpReply> reply = nullptr;
	TPLocker locker(m_lock);

	auto iter = m_map_replys.find(id);
	if (iter != m_map_replys.end())
	{
		reply = iter->second;
		m_map_replys.erase(iter);
	}
	return reply;
}

std::shared_ptr<HttpReply> HttpManager::getReply(int id)
{
	std::shared_ptr<HttpReply> reply = nullptr;
	TPLocker locker(m_lock);

	auto iter = m_map_replys.find(id);
	if (iter != m_map_replys.end())
	{
		reply = iter->second;
	}
	return reply;
}