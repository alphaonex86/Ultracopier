#include "Deque.h"
#include <windows.h>

CDeque::CDeque(void)
{
	top1=0;
	top2=0;
	head=NULL;
	tail=NULL;
}

CDeque::~CDeque(void)
{
}

void CDeque::push_front(wchar_t *str)
{
	CNode *temp;
	if(top1+top2 >= MAX_DEQUE)
	{
	  return ;
	}

	if(top1+top2 == 0)
	  {
	    head = new CNode;
	    wcscpy(head->str, str);
	    head->next=NULL;
	    head->prev=NULL;
	    tail=head;
	    top1++;
	  }
	 else
	 {
	     top1++;
	     temp=new CNode;
	     wcscpy(temp->str, str);
	     temp->next=head;
	     temp->prev=NULL;
	     head->prev=temp;
	     head=temp;
	 }
}

void CDeque::push_back(wchar_t *str)
{
	CNode *temp;
	if(top1+top2 >= MAX_DEQUE)
	{
	  return ;
	}
	if(top1+top2 == 0)
	  {
	    head = new CNode;
	    wcscpy(head->str, str);
	    head->next=NULL;
	    head->prev=NULL;
	    tail=head;
	    top1++;
	  }
	 else
	 {
	     top2++;
	     temp=new CNode;
	     wcscpy(temp->str, str);
	     temp->next=NULL;
	     temp->prev=tail;
	     tail->next=temp;
	     tail=temp;
	 }
}

int CDeque::size()
{
	return top1 + top2;
}

wchar_t *CDeque::at(int pos)
{
	int i=0;
	CNode *temp;

	if(top1+top2 <= 0)
	{
		return NULL;
	}

	temp=head;
	while(temp!=NULL)
	{
		if(i==pos)
			return temp->str;

		temp=temp->next;

		i++;
	}

	return NULL;
}
