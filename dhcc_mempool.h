
/* 	
*	created by xielei on 10 May 2011 
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#ifndef _TYPEDEF_H
#define _TYPEDEF_H
typedef char 	PRInt8;
typedef short 	PRInt16;
typedef int	PRInt32;
typedef long long PRInt64;
typedef unsigned char 	PRUint8;
typedef unsigned short 	PRUint16;
typedef unsigned int 	PRUint32;
typedef unsigned long long PRUint64;
//typedef unsigned int size_t;
#endif


#ifndef _DHCC_POOL_INCLUDED_
#define _DHCC_POOL_INCLUDED_


//定义大块内存和小块内存的分界线
//#define DHCC_POOL_MAX_ALLOC (4096-1)
//这样可能更加合理一些.such as when u need 4094byte memory what will the code above do?
#define DHCC_POOL_MAX_ALLOC (4096-sizeof(dhcc_pool_data_t))
//定义默认的内存池大小
#define DHCC_DEFAULT_POOL_SIZE (16*1024)

#ifndef DHCC_ALIGNMENT
#define DHCC_ALIGNMENT   sizeof(unsigned long)    /*按照多少字节内存对其*/
#endif

#define dhcc_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define dhcc_align_ptr(p, a)                                                   \
    (u_char *) (((PRUint32) (p) + ((PRUint32) a - 1)) & ~((PRUint32) a - 1))

typedef struct dhcc_pool_s dhcc_pool_t;

typedef struct {
	PRUint8	*last;
	PRUint8	*end;
	dhcc_pool_t *next;
	PRInt32	failed;
}dhcc_pool_data_t;

typedef struct dhcc_pool_large_s dhcc_pool_large_t;

struct dhcc_pool_large_s{
	dhcc_pool_large_t *next;
	void * alloc;
};

struct dhcc_pool_s{
	dhcc_pool_data_t	d;
	size_t	max;
	dhcc_pool_t * current;
	dhcc_pool_large_t *large;
};

//创建销毁内存池
dhcc_pool_t* dhcc_create_pool(size_t size);
void* dhcc_destroy_pool(dhcc_pool_t * pool);
void dhcc_reset_pool(dhcc_pool_t *pool);
//从内存池上分配连续指定大小的内存，返回分配内存的地址
void* dhcc_palloc(dhcc_pool_t *pool,size_t size);
void* dhcc_pcalloc(dhcc_pool_t *pool,size_t size);
PRInt32 dhcc_pfree(dhcc_pool_t *pool,void * p);
#endif

