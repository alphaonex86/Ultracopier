/** \file ClientCatchcopy.cpp
\brief Define the catchcopy client
\author alpha_one_x86
\version 0002
\date 2010 */

#include <stdio.h>

#include "ClientCatchcopy.h"
#include <WinSock.h>
//#pragma comment(lib, "Ws2_32.lib")

#undef NONBLOCK_FLAG
ClientCatchcopy::ClientCatchcopy()
{
	m_hpipe=NULL;
	idNextOrder=0;
	const char prefix[]="\\\\.\\pipe\\advanced-copier-";
	char uname[1024];
	DWORD len=1023;
	char *data;
	// false ??
	if(GetUserNameA(uname, &len)!=FALSE)
	{
		// convert into hexa
		data = toHex(uname);
		m_pipename = (char *) malloc(sizeof(prefix)+strlen(data)+2);
	#if defined(_MFC_VER)
		strcpy_s(m_pipename, _countof(prefix) ,prefix);
		strcat_s(m_pipename, sizeof(prefix)+strlen(data)+2,data);
	#else
		strcpy(m_pipename, prefix);
		strcat(m_pipename, data);
	#endif
		free(data);
		m_blk=NULL;
		m_len=0;
		m_tot=0;
		
		canConnect=true;
	}
	else
	{
		#ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		void* lpBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		::GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpBuffer,
		0,
		NULL );
		MessageBox(NULL,(LPCTSTR)lpBuffer, L"GetUserName Failed", MB_OK);
		LocalFree( lpBuffer );
		#endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		canConnect=false;
	}
}

ClientCatchcopy::~ClientCatchcopy()
{
	disconnectFromServer();
}

// Dump UTF16 (little endian)
char * ClientCatchcopy::toHex(const char *str)
{
	char *p, *sz;
	size_t len;
	if (str==NULL)
		return NULL;
	len= strlen(str);
	p = sz = (char *) malloc((len+1)*4);
	// username goes hexa...
	for (size_t i=0; i<len; i++)
	{
		#if defined(_MFC_VER)
			sprintf_s(p, (len+1)*4, "%.2x00", str[i]);
		#else
			sprintf(p, "%.2x00", str[i]);
		#endif
		p+=4;
	}
	*p=0;
	return sz;
}

bool ClientCatchcopy::connectToServer()
{
	if(!canConnect)
	{
		#ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		MessageBox(NULL,L"Can't connect due to previous error",L"Checking", MB_OK);
		#endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		return false;
	}
	if (m_hpipe==NULL)
	{
		#ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		char uname[1024];
		DWORD len=1023;
		GetUserNameA(uname, &len);
		char temp_char_debug[1024];
		sprintf(temp_char_debug, "user name: %s, pipe name: %s",uname , m_pipename);
		MessageBoxA(NULL,temp_char_debug,"Pipe name", MB_OK);
		#endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		// create pipe
		m_hpipe = CreateFileA(m_pipename, GENERIC_READ|GENERIC_WRITE|FILE_FLAG_OVERLAPPED , 0, NULL, OPEN_EXISTING, 0, NULL);
		if (m_hpipe==INVALID_HANDLE_VALUE)
		{
			m_hpipe=NULL;
			if (GetLastError()!= ERROR_PIPE_BUSY)
			{
				#ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
				void* lpBuffer;
				FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				::GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpBuffer,
				0,
				NULL );
				MessageBox(NULL,(LPCTSTR)lpBuffer, L"Can't connect to catchcopy application compatible: GetLastError()!= ERROR_PIPE_BUSY", MB_OK);
				LocalFree( lpBuffer );
				#endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
				return false;
			}
			#ifdef NONBLOCK_FLAG
				return false;
			#else
			if (!WaitNamedPipeA(m_pipename, 10000))
			{
				#ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
				void* lpBuffer;
				FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				::GetLastError(),
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPTSTR) &lpBuffer,
				0,
				NULL );
				MessageBox(NULL,(LPCTSTR)lpBuffer, L"Named pipe too long to responds", MB_OK);
				LocalFree( lpBuffer );
				#endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
				CloseHandle(m_hpipe);
				m_hpipe=NULL;
				return false;
			}
			#endif
		}

		#ifdef NONBLOCK_FLAG
		// The pipe connected; change to pipe_nowait mode. 
		  DWORD dwMode;
		  BOOL  fSuccess = FALSE; 
		  dwMode = PIPE_READMODE_BYTE|PIPE_NOWAIT; 

		   fSuccess = SetNamedPipeHandleState( 
			  m_hpipe,    // pipe handle 
			  &dwMode,  // new pipe mode 
			  NULL,     // don't set maximum bytes 
			  NULL);    // don't set maximum time 
		   if ( ! fSuccess) 
		   {
			  #ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
			  char error_str[1024];
			  sprintf(error_str, "SetNamedPipeHandleState failed. GLE=%d\n", ::GetLastError() ); 
			  MessageBoxA( NULL, error_str,"SetNamedPipeHandleState error", MB_OK);
			  #endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
			  return false;
		   }
		#endif //NONBLOCK_FLAG
	}
	#ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
	MessageBox(NULL,L"The explorer is now connected to the catchcopy compatible application",L"Checking", MB_OK);
	#endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG

	bool f = sendProtocol();
	if(f!=true)
		return false;
	#if defined(_M_X64)
		setClientName(L"Windows Explorer 64Bits");
	#else
		setClientName(L"Windows Explorer 32Bits");
	#endif
	return true;
}

void ClientCatchcopy::disconnectFromServer()
{
	clear();
	if(m_hpipe!=NULL)
	{
		/* needed to not crash the server when have data to read */
		FlushFileBuffers(m_hpipe);
		CloseHandle(m_hpipe);
		m_hpipe=NULL;
		#ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		MessageBox(NULL, L"The explorer is now disconnected of the catchcopy compatible application",L"Checking", MB_OK);
		#endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
	}
}

/// \brief to send order
bool ClientCatchcopy::sendProtocol()
{
	CDeque data;
	data.push_back(L"protocol");
	data.push_back(L"0002");
	return sendRawOrderList(data);
}

bool ClientCatchcopy::setClientName(wchar_t *name)
{
	CDeque data;
	data.push_back(L"client");
	data.push_back(name);
	return sendRawOrderList(data);
}

bool ClientCatchcopy::addCopyWithDestination(CDeque sources,wchar_t *destination)
{
    sources.push_front(L"cp");
	sources.push_back(destination);
    #ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
    MessageBox(NULL, "sources size: "+sources.size()+", first: "+sources.at(0),L"Checking", MB_OK);
    #endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
	return sendRawOrderList(sources);
}

bool ClientCatchcopy::addCopyWithoutDestination(CDeque sources)
{
	sources.push_front(L"cp-?");
    #ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
    MessageBox(NULL, "sources size: "+sources.size()+", first: "+sources.at(0),L"Checking", MB_OK);
    #endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
	return sendRawOrderList(sources);
}

bool ClientCatchcopy::addMoveWithDestination(CDeque sources, wchar_t *destination)
{
	sources.push_front(L"mv");
	sources.push_back(destination);
    #ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
    MessageBox(NULL, "sources size: "+sources.size()+", first: "+sources.at(0),L"Checking", MB_OK);
    #endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
	return sendRawOrderList(sources);
}

bool ClientCatchcopy::addMoveWithoutDestination(CDeque sources)
{
	sources.push_front(L"mv-?");
    #ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
    MessageBox(NULL, "sources size: "+sources.size()+", first: "+sources.at(0),L"Checking", MB_OK);
    #endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
	return sendRawOrderList(sources);
}

/// \brief to send stream of string list
bool ClientCatchcopy::sendRawOrderList(CDeque order, bool first_try)
{
	if(m_hpipe!=NULL)
	{
		int data_size=sizeof(int)*3;
		for(int i=0;i<order.size();i++)
		{
			if (order.at(i) == NULL)
				continue;

			data_size+=(int)sizeof(int);
			data_size+=(int)sizeof(wchar_t)*(int)wcslen(order.at(i));
		}
		addInt32(data_size);
		addInt32(idNextOrder++);
		addInt32((int)order.size());
		for(int i=0;i<order.size();i++)
		{
			if (order.at(i) == NULL)
				continue;

			//set string contenant
			addStr(order.at(i));
		}
		if(dataToPipe()<0)
		{
			if(first_try)
			{
				disconnectFromServer();
				connectToServer();
				return sendRawOrderList(order,false);
			}
			else
					clear();
		}
		return true;
	}
	else
	{
		clear();
		return false;
	}
}

// Send data block to named pipe
int ClientCatchcopy::dataToPipe()
{
	byte_t *ptr;
	int ret=0, max;
	if (m_hpipe!=NULL)
	{
			ptr = m_blk;
			while (!ret && m_len)
			{
					max=(m_len>BUFFER_PIPE) ? BUFFER_PIPE:m_len;
				#ifdef NONBLOCK_FLAG
					writePipe_nonBlock(m_hpipe, ptr, max);
					break;
				#else
					if(writePipe(m_hpipe, ptr, max)!=0)
					{
							ret=-2;
							break;
					}
					m_len-=max;
					ptr+=max;
				#endif
			}
	}
        return ret;
}

int ClientCatchcopy::writePipe(HANDLE hPipe, byte_t *ptr, int len)
{
	DWORD cbWritten;
	if (!WriteFile(hPipe, ptr, len, &cbWritten, NULL))
		return -4;
	return 0;
}

#ifdef NONBLOCK_FLAG
int ClientCatchcopy::writePipe_nonBlock(HANDLE hPipe, byte_t *ptr, int len)
{
	DWORD cbWritten;
	WriteFile(hPipe, ptr, len, &cbWritten, NULL);
	return 0;
}
#endif

// Add int32 (big-endian) into binary block
int ClientCatchcopy::addInt32(int value)
{
	blkGrowing(sizeof(int));
	// add value
	setInt32(m_len, value);
	m_len+=sizeof(int);
	return m_len-sizeof(int);
}

void ClientCatchcopy::setInt32(int offset, int value)
{
	C_INT(m_blk+offset)=htonl(value);
}

// Add unicode string into binary block from ASCIIZ
int ClientCatchcopy::addStr(WCHAR *data)
{
	int ret=-1, len;
	WCHAR *x;
	if (data!=NULL && *data)
	{
		// le => be
		x = _wcsdup(data);
		len = toBigEndian(x);
		// set size of string
		ret = addInt32(len);
		// and add it to block
		blkGrowing(len);
		memmove(m_blk+m_len, x, len);
		m_len+=len;
		free(x);
	}
	return ret;
}

// resize binary block (if needed)
byte_t *ClientCatchcopy::blkGrowing(int added)
{
	if (m_len+added>m_tot)
	{
		// check if added isn't bigger than buffer itself...
		m_tot+= (added>BLOCK_SIZ) ? added:BLOCK_SIZ;
		m_blk = (byte_t *) realloc(m_blk, m_tot);
	}
	return m_blk+m_len;
}

int ClientCatchcopy::toBigEndian(WCHAR *p)
{
	WCHAR tmp;
	int ret=0;
	while(*p)
	{
		tmp = htons(*p);
		*p++=tmp;
		ret+=2;
	}
	return ret;
}

void ClientCatchcopy::clear()
{
	m_tot=0;
	m_len=0;
	idNextOrder=0;
	if (m_blk!=NULL)
	{
		free(m_blk);
		m_blk=NULL;
	}
}

bool ClientCatchcopy::isConnected()
{
	if(m_hpipe==NULL)
		return false;
		
		bool fSuccess = PeekNamedPipe(
	  m_hpipe,
	  NULL,
	  0,
	  0,
	  0,
	  0
		);

	if(!fSuccess && GetLastError() != ERROR_MORE_DATA)
	{
		#ifdef CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		void* lpBuffer;
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		::GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpBuffer,
		0,
		NULL );
		MessageBox(NULL,(LPCTSTR)lpBuffer, L"Error detected with the connexion", MB_OK);
		LocalFree( lpBuffer );
		#endif // CATCHCOPY_EXPLORER_PLUGIN_DEBUG
		disconnectFromServer();
		return false;
	}
	else
		return true;
}
