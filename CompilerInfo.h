#if defined(Q_CC_GNU)
	#define COMPILERINFO QString("GCC build: ")+__DATE__+" "+__TIME__
#else
	#if defined(__DATE__) && defined(__TIME__)
		#define COMPILERINFO QString("Unknow compiler: ")+__DATE__+" "+__TIME__
	#else
		#define COMPILERINFO QString("Unknow compiler")
	#endif
#endif 
