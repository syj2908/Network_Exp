
/*
*主线程创建子线程（当前子线程为stop停止状态），5秒后主线程唤醒子线程，
*10秒后主线程挂起子线程。15秒后主线程再次唤醒子线程，20秒后主线程执行完毕，
*等待子线程退出。
*/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
 
#define	RUN 	1
#define STOP	0
 
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
 
int status = STOP;
 
void *thread_function(void *arg)
{
    static int i = 0;
	while(1)
	{
		pthread_mutex_lock(&mut);
		while(!status)
		{
			pthread_cond_wait(&cond, &mut);
		}
		pthread_mutex_unlock(&mut);
		printf("child pthread %d\n", i++);
		if (i == 20)
		{
			break;
		}
		sleep(1);
	}
}
 
void thread_resume()
{
	if (status == STOP)
	{
		pthread_mutex_lock(&mut);
		status = RUN;
		pthread_cond_signal(&cond);
		printf("pthread run!\n");
		pthread_mutex_unlock(&mut);
	}
	else
	{
		printf("pthread run already\n");
	}
}
 
void thread_pause()
{
	if (status == RUN)
	{
		pthread_mutex_lock(&mut);
		status = STOP;
		printf("thread stop!\n");
		pthread_mutex_unlock(&mut);
	}
	else
	{
		printf("pthread pause already\n");
	}
}
 
int main()
{
	int err;
	static int i = 0;
	pthread_t child_thread;
	
	if (pthread_mutex_init(&mut, NULL) != 0)
	{
		printf("mutex init error\n");
	}
	if (pthread_cond_init(&cond, NULL) != 0)
	{
		printf("cond init error\n");
	}

    err = pthread_create(&child_thread,NULL,thread_function,NULL);
	if (err != 0)
	{
		printf("can't create thread:%s\n", strerror(err));
	}
	
	while(1)
	{
		printf("father pthread %d\n", i++);
		sleep(1);
		if (i == 5)
		{
			thread_resume();
		}
		if (i == 10)
		{
			thread_pause();
		}
		if (i == 15)
		{
			thread_resume();
		}
		if (i == 20)
		{
			break;
		}
	}
	if (0 == pthread_join(child_thread, NULL))
	{
		printf("child thread is over\n");
	}
	
	return 0;
}