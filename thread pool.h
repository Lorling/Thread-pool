#include <vector>
#include <queue>
#include <mutex>
#include <future>
#include <thread>
#include <utility>
#include <functional>
#include <shared_mutex>

//创建一个安全队列 
template <typename T>
class SafeQueue{
private:
	std::queue<T> m_queue;//构造队列 
	std::shared_mutex m_mutex;
public:
	SafeQueue() {}
	SafeQueue(SafeQueue &&other) {}
	~SafeQueue() {}
	//返回队列是否为空
	bool empty(){
		std::shared_lock<std::shared_mutex> lock(m_mutex);
		return m_queue.empty(); 
	} 
	//返回队列大小
	int size(){ 
		std::shared_lock<std::shared_mutex> lock(m_mutex);
		return m_queue.size();
	}
	//队列添加元素
	void push(T &t){
		std::unique_lock<std::shared_mutex> lock(m_mutex);
		m_queue.emplace(t);
	} 
	//队列取出元素
	bool pop(T &t){
		std::unique_lock<std::shared_mutex> lock(m_mutex);
		if(m_queue.empty()) return false;
		t=std::move(m_queue.front());//取出队首元素，返回队首元素值，并进行右值引用 
		m_queue.pop();//弹出队首元素 
		return true;
	}
};

class ThreadPool{
private:
	class ThreadWorker{
	private:
		ThreadPool *m_pool;//所属线程池
	public:
		ThreadWorker(ThreadPool * pool) : m_pool(pool){
		} 
		//重载()操作
		//让()操作进行从队列中取任务和执行 
		void operator()(){
			std::function<void ()> func;
			bool flag=false;//是否从队列中取出元素 
			while(!m_pool->m_shutdown){
				{
					std::unique_lock<std::mutex> lock(m_pool -> m_mutex);
					//如果队列为空的话，阻塞当前进程 
					if(m_pool -> m_queue.empty()){
						m_pool -> m_lock.wait(lock);
					}
					//取出队列中的元素 
					flag = m_pool -> m_queue.pop(func);
				}
				// 如果成功取出则执行工作函数 
				if(flag) func();
			}
		}
	};
	
	bool m_shutdown;//线程池是否关闭
	SafeQueue<std::function<void()>> m_queue;
	std::vector<std::thread> m_threads;
	std::mutex m_mutex;
	std::condition_variable m_lock;
public:
	ThreadPool(const int n = 4) : m_threads(n),m_shutdown(false){
		for(auto & i : m_threads)
			i = std::thread(ThreadWorker(this));
	}
	ThreadPool(const ThreadPool &) = delete;
	ThreadPool(ThreadPool &&) = delete;
	ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
	
	template <typename F,typename ... Args>
	auto submit(F && f, Args && ... args) -> std::future<decltype(f(args...))>
	{
		std::function<decltype(f(args ...))()> func = std::bind(std::forward<F>(f),std::forward<Args>(args)...);
		auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args ...))()>>(func);
		std::function<void()> warpper_func = [task_ptr](){
			(*task_ptr)();
		};
		m_queue.push(warpper_func);
		m_lock.notify_one();
		return task_ptr->get_future();
	}
	
	~ThreadPool(){
		m_shutdown = true;
		//唤醒所有进程 
		m_lock.notify_all();
		
		for(int i=0;i<m_threads.size();i++){
			if(m_threads[i].joinable())
				m_threads[i].join();
		} 
	}
};
