#include <windows.h>
#include <locale>

extern "C"
int * __errno() { return _errno(); }

extern bool MainWindow_RegisterClass(HINSTANCE);
extern bool MainWindow_Show(HINSTANCE);
extern void MainWindow_Run();

int APIENTRY WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nCmdShow )
{
  HWND hMainWnd;
  if(
  !MainWindow_RegisterClass(hInstance) ||
  !MainWindow_Show(hInstance)) {
    MessageBox(NULL, "Failed to initialize", "Brightmoon", MB_OK | MB_ICONSTOP);
    return -1;
  }

  MainWindow_Run();
  return 0;
}
