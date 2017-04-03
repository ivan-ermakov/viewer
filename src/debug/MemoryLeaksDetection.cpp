#if defined(_MSC_VER) && defined(_DEBUG) && defined(_CRTDBG_MAP_ALLOC)

#include <windows.h>

#include <algorithm>
#include <map>
#include <vector>
#include <string>

struct leak
{
  int count;
  size_t size;
};
std::map<std::string, leak> m;

std::string lastmessage;
bool bhavelastmessage = false;

int ReportHook( int , char *message, int * )
{   
  //Collect only in project memory leak messages 
  long long a,b;

  if (bhavelastmessage && 
      sscanf(message,"normal block at 0x%I64x, %I64d bytes long.\n",&a,&b) == 2)
  {
    std::map<std::string, leak>::iterator i = m.find(lastmessage);
    if(i != m.end())
      i->second.size += (std::size_t) b;
    bhavelastmessage = false;            
  }
  else if(strncmp(message, " Data:", 6) != 0 && strstr(message,") :"))
  {
//    if ( strstr(message, "Data: ") )
//      __debugbreak();
    bhavelastmessage = true;
    lastmessage = message;
    std::map<std::string, leak>::iterator i = m.find(message);
    if(i == m.end())
    {
      leak l = {1,0};
      m[message] = l;
    }
    else
      i->second.count++;
  }
  return TRUE;
}

void MemoryLeakReport()
{
  _CrtDumpMemoryLeaks();

  //Convert to vector
  std::vector<std::pair<std::string, leak> > v(m.begin(), m.end());

  struct op
  {
    bool operator()(const std::pair<std::string, leak> &l, const std::pair<std::string, leak> &r)
    {
      return l.second.size < r.second.size;
    }
  };
  //Sort by leak size
  std::sort(v.rbegin(), v.rend(),op());

  if(v.size())
    OutputDebugStringA("WARNING: Memory leaks detected.\n");
  for(std::vector<
      std::pair<std::string, leak> >::iterator i=v.begin(); i!=v.end(); ++i)
  {
    char buf[1024];
    sprintf(buf,"%s %zd bytes, %d times\n",i->first.c_str(),i->second.size,i->second.count);
    //Out to console
    OutputDebugStringA(buf);
  }
}

#endif

void installMemoryLeaksFilter()
{
#if defined(_MSC_VER) && defined(_DEBUG) && defined(_CRTDBG_MAP_ALLOC)
    _CrtSetReportHook(ReportHook);
    atexit(MemoryLeakReport);
    //_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF /*| _CRTDBG_LEAK_CHECK_DF*/ );
#endif
}
