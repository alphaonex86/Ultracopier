#if defined(PLATFORM_MAC) || defined(PLATFORM_HAIKU)
  #define fseeko64  stat
  #define ftruncate64 ftruncate
  #define off64_t off_t
  #define O_LARGEFILE 0
#endif 
