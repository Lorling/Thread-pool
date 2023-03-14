#include <iostream>
#include <random>
#include "thread pool.h"

std::random_device rd; // ��ʵ�����������

std::mt19937 mt(rd()); //���ɼ��������mt

std::uniform_int_distribution<int> dist(-1000,1000); //����-1000��1000֮�����ɢ���ȷֲ���

auto rnd = std::bind(dist,mt);

//�����߳�˯��ʱ��
void simulate_hard_computation(){
	std::this_thread::sleep_for(std::chrono::milliseconds(2000+rnd()));
} 

//�����������ֵļ򵥺�������ӡ���
void muliply(const int a,const int b){
	simulate_hard_computation();
	const int res = a*b;
	std::cout<<a<<"*"<<b<<"="<<res<<std::endl;
} 

//���Ӳ�������
void muliply_output(int &out,const int a,const int b){
	simulate_hard_computation();
	out = a*b;
	std::cout<<a<<"*"<<b<<"="<<out<<std::endl;
} 

//�������
int muliply_return (const int a,const int b){
	simulate_hard_computation();
	const int res = a*b;
	std::cout<<a<<"*"<<b<<"="<<res<<std::endl;
	return res;
} 

void example(){
	//���������̵߳��̳߳�
	ThreadPool pool(3);
	
	//��ʼ���̳߳�
	pool.init() ;
	
	//�ύ�˷�����������ʮ��
	for(int i=1;i<=3;i++)
		for(int j=1;j<=10;j++)
			pool.submit(muliply,i,j);
			
	//ʹ��ref���ݵ���������ύ����
	int output_ref;
	auto future1 = pool.submit(muliply_output,std::ref(output_ref),5,6);
	
	//�ȴ��˷�������
	future1.get();
	std::cout<<"Last operation result is equals to " << output_ref << std::endl;
	
	//ʹ��return�����ύ����
	auto future2=pool.submit(muliply_return,5,3);
	
	//�ȴ��˷�������
	int res=future2.get(); 
	std::cout<<"Last operation result is equals to " << res << std::endl;
	
	//�ر��̳߳�
	pool.shutdown(); 
}

int main()
{
	example();
	
	return 0;
}
