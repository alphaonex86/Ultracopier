#pragma once

#define MAX_STR_LEN 512
#define MAX_DEQUE   512

class CNode
{
public:
	wchar_t str[MAX_STR_LEN];
	class CNode *next;
	class CNode *prev;
};

class CDeque : public CNode
{
public:
	CDeque(void);
	~CDeque(void);

private:
	CNode *head,*tail;
	int top1,top2;

public:
	void push_front(wchar_t *str);
	void push_back(wchar_t *str);
	int size();
	wchar_t *at(int pos);
};
