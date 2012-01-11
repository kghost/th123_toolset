#include <shlwapi.h>

void PathRemoveFileSpecEx(LPSTR pszPath)
{
  LPSTR pszLastSlash = NULL;
  LPSTR pszCurrent = pszPath;
  while(*pszCurrent) {
    if(*pszCurrent == '/' || *pszCurrent == '\\')
      pszLastSlash = pszCurrent;
    pszCurrent = CharNext(pszCurrent);
  }
  if(pszLastSlash) *CharNext(pszLastSlash) = 0;
  else *pszPath = 0; 
}

void PathSlashToBackSlash(LPSTR pszPath)
{
  while(*pszPath) {
    if(*pszPath == '/') *pszPath = '\\';
    pszPath = CharNext(pszPath);
  }
}
