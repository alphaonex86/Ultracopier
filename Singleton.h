/** \file Singleton.h
\brief Define the singleton for class which should have unique object
\author alpha_one_x86
\note Big thanks to all people in the channel #qt-fr of freenode of irc
\version 0.3
\date 2010
\licence GPL3, see the file COPYING */

#include <QMutex>
#include <QMutexLocker>

#ifndef SINGLETON_H
#define SINGLETON_H

/** \class Singleton
\brief Define the singleton for class which should have unique object
\note Big thanks to all people in the channel #qt-fr of freenode of irc */
template <typename T>
class Singleton
{
	public:
		/// \brief Public interface
		static T *getInstance()
		{
			number_load++;
			if(_singleton==NULL)
				_singleton = new T;
			return (static_cast<T*> (_singleton));
		}
		/// \brief For destroy only when the call to this function call count is the same as the getInstance() count call
		static void destroyInstanceAtTheLastCall()
		{
			number_load--;
			if(number_load==0)
			{
				delete _singleton;
				_singleton=NULL;
			}
		}
	private:
		/// \brief Unique instance
		static T *_singleton;
		/// \brief To count the getInstance() call count
		static int number_load;
};

template <typename T>
T *Singleton<T>::_singleton = NULL;
template <typename T>
int Singleton<T>::number_load = 0;

#endif // SINGLETON_H
