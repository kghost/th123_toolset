#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <imagehlp.h>
#include <shlobj.h>
#include <memory>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include "mima.hpp"
#include "yumemi.hpp"
#include "vivit.hpp"
#include "frandre.hpp"
#include "yuyuko.hpp"
#include "kaguya.hpp"
#include "kanako.hpp"
#include "hinanawi.hpp"
#include "suica.hpp"
#include "pathext.hpp"
#include "resource.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/ sizeof(x)[0])
#endif

#ifndef HANDLE_WM_SIZING
#define HANDLE_WM_SIZING(hwnd, wParam, lParam, fn) \
    (fn)((hwnd), (UINT)(wParam), (LPRECT)(lParam))
#endif

#define WC_MYMAINWND  "pbgmgr_MainWindow"
#define WT_MYMAINWND  "Brightmoon"

enum LV_SORTTYPE {
  LVSORT_STRING,
  LVSORT_NUMBER
};

static struct {
  PSTR pszText;
  int nWidth;
  LV_SORTTYPE nSortType;
} g_ColumnSet[] = {
  { "Name",         100, LVSORT_STRING },
  { "Path",         150, LVSORT_STRING },
  { "Type",         100, LVSORT_STRING },
  { "Size",         100, LVSORT_NUMBER },
  { "Comp. Ratio",   80, LVSORT_NUMBER }
};

struct MYLISTITEM {
  std::string column[ARRAYSIZE(g_ColumnSet)];
  boost::shared_ptr<PBGArchiveEntry> entry;
} ;

HINSTANCE g_hInstance;
HACCEL g_hAccel;
HMENU g_hMenu;
HWND  g_hMainWnd;
HWND  g_hListWnd;

std::vector<MYLISTITEM> g_listItems;
int  g_sortColumn;
bool g_sortAscend;

std::auto_ptr<PBGArchive> g_archive;


bool g_statuscancel;

BOOL CALLBACK StatusDialog_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
  case WM_INITDIALOG: return TRUE;
  case WM_COMMAND:
    if(LOWORD(wParam) == IDCANCEL)
      g_statuscancel = true;
    break;
  }
  return FALSE;
}

bool ListView_Show(HWND hParentWnd, HINSTANCE hInstance)
{
  g_hListWnd = CreateWindowEx(
    WS_EX_CLIENTEDGE,
    WC_LISTVIEW, NULL,
    WS_CHILDWINDOW | WS_OVERLAPPED | WS_VISIBLE | WS_HSCROLL | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_OWNERDATA,
    0, 0,
    CW_USEDEFAULT, CW_USEDEFAULT,
    hParentWnd, NULL, hInstance, NULL);
  if(!g_hListWnd) return false;

  ListView_SetItemCountEx(g_hListWnd, 0, LVSICF_NOINVALIDATEALL);
  //ListView_SetExtendedListViewStyle(g_hListWnd, LVS_EX_FULLROWSELECT);
  
  LVCOLUMN lvCol;
  lvCol.mask  = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
  lvCol.fmt   = LVCFMT_LEFT;
  for(int i = 0; i < ARRAYSIZE(g_ColumnSet); i++) {
    lvCol.cchTextMax  = strlen(g_ColumnSet[i].pszText);
    lvCol.pszText     = g_ColumnSet[i].pszText;
    lvCol.cx          = g_ColumnSet[i].nWidth;
    ListView_InsertColumn(g_hListWnd, i, &lvCol);
  }

  RECT rect;
  GetClientRect(hParentWnd, &rect);
  MoveWindow(g_hListWnd, 0, 0, rect.right - rect.left, rect.bottom - rect.top, TRUE);

  return true;
}

bool ListView_StringDescendSortProc(const MYLISTITEM &item1, const MYLISTITEM &item2)
{
  const std::string &itemText1 = item1.column[g_sortColumn];
  const std::string &itemText2 = item2.column[g_sortColumn];
  return itemText1 < itemText2;
}

bool ListView_StringAscendSortProc(const MYLISTITEM &item1, const MYLISTITEM &item2)
{
  const std::string &itemText1 = item1.column[g_sortColumn];
  const std::string &itemText2 = item2.column[g_sortColumn];
  return itemText1 > itemText2;
}

bool ListView_NumberDescendSortProc(const MYLISTITEM &item1, const MYLISTITEM &item2)
{
  int itemNumber1 = boost::lexical_cast<int>(item1.column[g_sortColumn]);
  int itemNumber2 = boost::lexical_cast<int>(item2.column[g_sortColumn]);
  return itemNumber1 < itemNumber2;
}

bool ListView_NumberAscendSortProc(const MYLISTITEM &item1, const MYLISTITEM &item2)
{
  int itemNumber1 = boost::lexical_cast<int>(item1.column[g_sortColumn]);
  int itemNumber2 = boost::lexical_cast<int>(item2.column[g_sortColumn]);
  return itemNumber1 > itemNumber2;
}

void ListView_OnColumnClick(NM_LISTVIEW *pNMLV)
{
  if(pNMLV->iSubItem != g_sortColumn)
    g_sortAscend = false;
  g_sortColumn = pNMLV->iSubItem;

  switch(g_ColumnSet[g_sortColumn].nSortType) {
  case LVSORT_STRING:
    std::stable_sort(g_listItems.begin(), g_listItems.end(), 
      g_sortAscend ? ListView_StringAscendSortProc : ListView_StringDescendSortProc);
    break;
  case LVSORT_NUMBER:
    std::stable_sort(g_listItems.begin(), g_listItems.end(), 
      g_sortAscend ? ListView_NumberAscendSortProc : ListView_NumberDescendSortProc);
    break;
  }
  g_sortAscend = !g_sortAscend;

  InvalidateRect(g_hListWnd, NULL, TRUE);
  UpdateWindow(g_hListWnd);
}

void ListView_OnGetDispInfo(LV_DISPINFO * pLVDI)
{
  if(pLVDI->item.mask & LVIF_TEXT) {
    lstrcpy(pLVDI->item.pszText, 
      g_listItems[pLVDI->item.iItem].column[pLVDI->item.iSubItem].c_str());
  }
}

BOOL MainWindow_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
  InitCommonControls();

  if(!ListView_Show(hwnd, g_hInstance)) return FALSE;
  return TRUE;
}

void MainWindow_OnDestroy(HWND hwnd)
{
  PostQuitMessage(0);
}

BOOL MainWindow_OnSizing(HWND hwnd, UINT fwSide, LPRECT lprc)
{
  RECT rect;
  GetClientRect(hwnd, &rect);
  MoveWindow(g_hListWnd, 0, 0, rect.right - rect.left, rect.bottom - rect.top, TRUE);
  return TRUE;
}

void MainWindow_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
  RECT rect;
  GetClientRect(hwnd, &rect);
  MoveWindow(g_hListWnd, 0, 0, rect.right - rect.left, rect.bottom - rect.top, TRUE);
  return;
}

template<class T>
bool OpenArchive(LPCSTR filename)
{
  g_archive.reset(new(std::nothrow) T);
  if(!g_archive.get()) return false;

  if(!g_archive.get()->Open(filename) ||
     !g_archive.get()->EnumFirst()) {
    g_archive.reset(NULL);
    return false;
  }
  return true;
}

bool TryToOpenArchive(HWND hwnd, const char *filename)
{
  if(OpenArchive<MimaArchive>(filename)) return true;
  if(OpenArchive<YumemiArchive>(filename)) return true;
  if(OpenArchive<VivitArchive>(filename)) return true;
  if(OpenArchive<FrandreArchive>(filename)) return true;
  if(OpenArchive<YuyukoArchive>(filename)) return true;
  if(OpenArchive<KaguyaArchive>(filename)) {
    int result = MessageBox(hwnd, 
      "press 'Yes' to treat as Touhou Eiyashou Archive. Otherwise, press 'No.'",
      WT_MYMAINWND, MB_YESNO | MB_ICONINFORMATION);
    static_cast<KaguyaArchive *>(g_archive.get())
      ->SetArchiveType(result != IDYES);
    return true;
  }
  if(OpenArchive<KanakoArchive>(filename)) return true;
  if(OpenArchive<HinanawiArchive>(filename)) return true;
  if(OpenArchive<SuicaArchive>(filename)) return true;
  return false;
}


void MainMenu_OnFileOpen(HWND hwnd)
{
  OPENFILENAME ofn;
  char filename[1024];
  ZeroMemory(&ofn, sizeof(OPENFILENAME));
  ZeroMemory(filename, sizeof(filename));
  ofn.lStructSize = sizeof(OPENFILENAME);
  ofn.hwndOwner   = hwnd;
  ofn.lpstrFilter = "Brightmoon Archive (*.dat)\0*.dat\0";
  ofn.lpstrFile   = filename;
  ofn.nMaxFile    = sizeof filename;
  ofn.Flags       = OFN_EXPLORER | OFN_FILEMUSTEXIST;
  if(!GetOpenFileName(&ofn)) return;

  ListView_SetItemCountEx(g_hListWnd, 0, LVSICF_NOINVALIDATEALL);
  g_listItems.clear();

  if(!TryToOpenArchive(hwnd, filename)) {
    MessageBox(hwnd, "failed to open archive.", "error", MB_OK | MB_ICONSTOP);
    return;
  }

  do {
    MYLISTITEM item;
    PBGArchiveEntry *entry = g_archive.get()->GetEntry();
    item.entry.reset(entry);

    std::string entryname(entry->GetEntryName());
    std::vector<char> filename(entryname.begin(), entryname.end());
    filename.push_back(0);
    PathStripPath(&filename[0]);
    item.column[0] = std::string(&filename[0]);

    std::vector<char> filepath(entryname.begin(), entryname.end());
    filepath.push_back(0);
    PathRemoveFileSpecEx(&filepath[0]);
    item.column[1] = std::string(&filepath[0]);

    SHFILEINFO sfi;
    SHGetFileInfo(&filename[0], 0, &sfi, sizeof sfi,
            SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES);
    item.column[2] = std::string(sfi.szTypeName);

    char buffer[64];
    wsprintf(buffer, "%d", entry->GetOriginalSize());
    item.column[3] = std::string(buffer);
   
    long comp_ratio = MulDiv(entry->GetCompressedSize(), 100, entry->GetOriginalSize());
    wsprintf(buffer, "%d", comp_ratio);
    item.column[4] = std::string(buffer);

    g_listItems.push_back(item);
  } while(g_archive.get()->EnumNext());
  ListView_SetItemCountEx(g_hListWnd, g_listItems.size(), LVSICF_NOINVALIDATEALL);

  EnableMenuItem(g_hMenu, ID_FILE_CLOSE  , MF_ENABLED);
  EnableMenuItem(g_hMenu, ID_FILE_EXTRACT, MF_ENABLED);
  EnableMenuItem(g_hMenu, 1, MF_BYPOSITION | MF_ENABLED);
  DrawMenuBar(hwnd);
}

bool ExtractCallback(const char *message, void *user)
{
  MSG msg;
  while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  HWND hStatusDlg = (HWND)user;
  SendDlgItemMessage(hStatusDlg, IDC_EDIT_STATUS, EM_REPLACESEL, 1, (LPARAM)message);
  UpdateWindow(hStatusDlg);

  return !g_statuscancel;
}

void MainMenu_OnExtract(HWND hwnd)
{
  BROWSEINFO bi;
  ZeroMemory(&bi, sizeof(BROWSEINFO));
  bi.hwndOwner      = hwnd;
  bi.lpszTitle      = "Please choose a folder for extracted files.";
  bi.pidlRoot       = CSIDL_DESKTOP;
  bi.ulFlags        = BIF_RETURNONLYFSDIRS;
  LPITEMIDLIST lpidl = SHBrowseForFolder(&bi);
  if(!lpidl) return;

  char extractpath[1024];
  SHGetPathFromIDList(lpidl, extractpath);
  CoTaskMemFree(lpidl);

  SetCurrentDirectory(extractpath);

  HWND hStatusDlg = CreateDialog(g_hInstance, MAKEINTRESOURCE(IDD_STATUS), hwnd, StatusDialog_WindowProc);
  SendDlgItemMessage(hStatusDlg, IDC_EDIT_STATUS, EM_SETSEL, 0, 0);
  ShowWindow(hStatusDlg, SW_SHOW);

  g_statuscancel = false;
  int index = -1;
  while((index = ListView_GetNextItem(g_hListWnd, index, LVNI_SELECTED)) != -1 &&
        !g_statuscancel)
  {
    PBGArchiveEntry *entry = g_listItems[index].entry.get();
    try {
      std::string entryname(entry->GetEntryName());
      std::vector<char> outname(entryname.begin(), entryname.end());
      outname.push_back(0);
      PathSlashToBackSlash(&outname[0]);

      std::vector<char> filepath(outname);
      PathRemoveFileSpecEx(&filepath[0]);
      if(filepath[0])
        MakeSureDirectoryPathExists(&filepath[0]);
	  int wlen = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, &outname[0], outname.size(), NULL, 0);
	  std::vector<WCHAR> ws(wlen);
	  MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, &outname[0], outname.size(), &ws[0], ws.size());
	  ws.push_back(0);
      std::ofstream os(&ws[0], std::ios_base::binary);
      if(os.fail()) {
        throw std::exception();
      } else {
        entry->Extract(os, ExtractCallback, (void *)hStatusDlg);
      }
    } catch(std::exception &e) {
      ExtractCallback("failed to create", (void *)hStatusDlg);
      ExtractCallback(entry->GetEntryName(), (void *)hStatusDlg);
      ExtractCallback("\r\n", (void *)hStatusDlg);
    }
  }
  DestroyWindow(hStatusDlg);
}

void MainMenu_OnFileClose(HWND hwnd)
{
  ListView_SetItemCountEx(g_hListWnd, 0, LVSICF_NOINVALIDATEALL);
  g_listItems.clear();
  g_archive.reset(NULL);
  EnableMenuItem(g_hMenu, ID_FILE_CLOSE  , MF_GRAYED);
  EnableMenuItem(g_hMenu, ID_FILE_EXTRACT, MF_GRAYED);
  EnableMenuItem(g_hMenu, 1, MF_BYPOSITION | MF_GRAYED);
  DrawMenuBar(hwnd);
}

void MainMenu_OnQuit(HWND hwnd)
{
  PostMessage(hwnd, WM_CLOSE, 0, 0);
}

void MainMenu_OnSelectAll(HWND hwnd)
{
  ListView_SetItemState(g_hListWnd, (UINT)-1, LVIS_SELECTED, LVIS_SELECTED);
  SetFocus(g_hListWnd);
}

void MainMenu_OnSelectInv(HWND hwnd)
{
  for(int i = 0; i < ListView_GetItemCount(g_hListWnd); ++i) {
    int oldstate = ListView_GetItemState(g_hListWnd, i, LVIS_SELECTED);
    int newstate = (oldstate == LVIS_SELECTED ? 0 : LVIS_SELECTED);
    ListView_SetItemState(g_hListWnd, i, newstate, LVIS_SELECTED);
  }
  SetFocus(g_hListWnd);
}

void MainWindow_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
  switch(id) {
  case ID_FILE_OPEN:    MainMenu_OnFileOpen(hwnd);  break;
  case ID_FILE_EXTRACT: MainMenu_OnExtract(hwnd);   break;
  case ID_FILE_CLOSE:   MainMenu_OnFileClose(hwnd); break;
  case ID_FILE_QUIT:    MainMenu_OnQuit(hwnd);      break;
  case ID_LIST_SELALL:  MainMenu_OnSelectAll(hwnd); break;
  case ID_LIST_SELINV:  MainMenu_OnSelectInv(hwnd); break;
  }
  return;
}

LRESULT MainWindow_OnNotify(HWND hwnd, int idCtrl, LPNMHDR pNMHdr)
{
  if(pNMHdr->hwndFrom != g_hListWnd) return FALSE;
  switch(pNMHdr->code) {
  case LVN_COLUMNCLICK: ListView_OnColumnClick(reinterpret_cast<NM_LISTVIEW *>(pNMHdr)); break;
  case LVN_GETDISPINFO: ListView_OnGetDispInfo(reinterpret_cast<LV_DISPINFO *>(pNMHdr)); break;
  }
  return TRUE;
}

LRESULT CALLBACK MainWindow_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
    HANDLE_MSG(hwnd, WM_CREATE,   MainWindow_OnCreate);
    HANDLE_MSG(hwnd, WM_DESTROY,  MainWindow_OnDestroy);
    HANDLE_MSG(hwnd, WM_SIZING,   MainWindow_OnSizing);
    HANDLE_MSG(hwnd, WM_SIZE,     MainWindow_OnSize);
    HANDLE_MSG(hwnd, WM_COMMAND,  MainWindow_OnCommand);
    HANDLE_MSG(hwnd, WM_NOTIFY,   MainWindow_OnNotify);
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool MainWindow_RegisterClass(HINSTANCE hInstance)
{
  WNDCLASSEX wcex;
  wcex.cbSize         = sizeof(WNDCLASSEX);
  wcex.cbClsExtra     = 0;
  wcex.cbWndExtra     = 0;
  wcex.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
  wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
  wcex.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
  wcex.hIconSm        = NULL;
  wcex.hInstance      = hInstance;
  wcex.lpfnWndProc    = MainWindow_WndProc;
  wcex.lpszClassName  = WC_MYMAINWND;
  wcex.lpszMenuName   = NULL;
  wcex.style          = CS_VREDRAW | CS_HREDRAW;

  return RegisterClassEx(&wcex) != 0;
}

bool MainWindow_Show(HINSTANCE hInstance)
{
  g_hInstance = hInstance;
  g_hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDM_MENU));
  if(!g_hMenu) return false;
  g_hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL));
  if(!g_hAccel) return false;

  g_hMainWnd = CreateWindowEx(
    0,
    WC_MYMAINWND, WT_MYMAINWND,
    WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,
    CW_USEDEFAULT, CW_USEDEFAULT,
    640, 480,
    NULL, g_hMenu, hInstance, NULL);
  if(!g_hMainWnd) return false;

  ShowWindow(g_hMainWnd, SW_SHOW);
  return true;
}

void MainWindow_Run()
{
  MSG msg;
  while(GetMessage(&msg, NULL, 0, 0)) {
    if(!TranslateAccelerator(g_hMainWnd, g_hAccel, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}
