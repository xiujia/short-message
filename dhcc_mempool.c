
/* 	
*	created by xielei on 10 May 2011 	
*/
#include"dhcc_mempool.h"
static void* dhcc_palloc_block(dhcc_pool_t * pool,size_t size);
static void* dhcc_palloc_large(dhcc_pool_t* pool,size_t size);

dhcc_pool_t * dhcc_create_pool(size_t size)
{
	dhcc_pool_t * p;
	if(!(p=(dhcc_pool_t*)malloc(size))){
		return NULL;
	}
	p->d.last=(char*)p+sizeof(dhcc_pool_t);
	p->d.end=(char*)p+size;
	p->d.failed=0;
	p->d.next=NULL;

	//分配的内存块最大值不能超过当前内存池申请的内存大小
	size-=sizeof(dhcc_pool_t);
	p->max=(size<DHCC_POOL_MAX_ALLOC)?size:(DHCC_POOL_MAX_ALLOC);
	p->large=NULL;
	p->current=p;
	return p;
}

void* dhcc_destroy_pool(dhcc_pool_t * pool)
{
	dhcc_pool_t *p,*n;
	dhcc_pool_large_t *l;
	for(l=pool->large;l;l=l->next){
		if(l->alloc)
			free(l->alloc);
	}
	for(p=pool,n=pool->d.next;;p=n,n=n->d.next){
		free(p);
		if(n==NULL)
			break;
	}
}
void dhcc_reset_pool(dhcc_pool_t *pool)
{
	dhcc_pool_t* p;
	dhcc_pool_large_t *l;
	for(l=pool->large;l;l=l->next){
		if(l->alloc!=NULL){
			free(l->alloc);
		//l->alloc=NULL;/*可要可不要*/
			}
		}
	pool->large=NULL;
	
	for(p=pool;p;p=p->d.next){
		p->d.last=(char *)p+sizeof(dhcc_pool_t);
	}
}
void* dhcc_palloc(dhcc_pool_t * pool,size_t size){
	u_char	*m;
	dhcc_pool_t *p;
	if(size<=pool->max){
		//如果为小块内存则
		p=pool->current;
		do{
			m=p->d.last;
			if((size_t)size<=p->d.end-p->d.last){
				p->d.last=m+size;
				return m;
				}
			p=p->d.next;
		}
		while(p);
		return dhcc_palloc_block(pool,size);
		}
	return dhcc_palloc_large(pool,size);
}

void* dhcc_pcalloc(dhcc_pool_t * pool,size_t size){
	void *p;
	p=dhcc_palloc(pool,size);
	if(p){
		memset((char*)p,0,size);}
		return p;
}
//只能被本文件内的函数调用
static void* dhcc_palloc_block(dhcc_pool_t *pool,size_t size){
	u_char* m;
	size_t psize;
	dhcc_pool_t *newp,*p,*current;
	psize=(size_t)((PRUint8*)pool->d.end-(PRUint8*)pool);
	//按照原来dhcc_pool_t的大小分配一块同样大小的内存。
	m=malloc(psize);
	if(!m){
		return NULL;
		}
	newp=(dhcc_pool_t*)m;
	newp->d.end=m+psize;
	newp->d.failed=0;
	newp->d.next=NULL;

	m+=sizeof(dhcc_pool_data_t);
	newp->d.last=m+size;

	//内存池中的current开始遍历 给新建立的节点找到合适的位置。并且挂在链表上
	current=pool->current;

	for(p=current;p->d.next;p=p->d.next)
		if(p->d.failed++>4)
			current=p->d.next;
	p->d.next=newp;

	pool->current=current?current:newp;
	return m;
}
//只能被本文件内的函数调用声明为静态函数
static void* dhcc_palloc_large(dhcc_pool_t *pool,size_t size)
{
	void *p;
	PRUint32 n=0;
	dhcc_pool_large_t * large;
	p=(void*)malloc(size);
	if(p==NULL)
		return NULL;
	//首先在dhcc_pool_t中的large链表中查找有没有空的位置提供给
	for(large=pool->large;large;large=large->next){
		if(large->alloc==NULL){
			large->alloc=p;
			return p;
			}
		if(n++>3)
			break;
		}

	large=(dhcc_pool_large_t*)dhcc_palloc(pool,sizeof(dhcc_pool_large_t));
	if(large==NULL){
		free(p);
		return NULL;
		}
	//将large节点添加到链表头
	large->alloc=p;
	large->next=pool->large;
	pool->large=large;
	return p;
}
PRInt32 dhcc_pfree(dhcc_pool_t *pool,void * p)
{
	dhcc_pool_large_t* l;
	for(l=pool->large;l;l=l->next){
		if(l->alloc==p){
			free(l->alloc);
			l->alloc=NULL;
			return 0;
			}
		}
	return -1;
}
