// Resist2D.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Resist2D.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
char szTitle[MAX_LOADSTRING];					// The title bar text
char szWindowClass[MAX_LOADSTRING];			// the main window class name
HWND hwndA, hwndStat;
const int StatHeight = 28;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

#define MAINWINDOWSTYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

CImage img;
CImage res;
double AnimationTime;
volatile char ProgressStage;
volatile float ProgressDone;


struct idximageinfo {
	int sizx, sizy, nvar, dum;
	int vidxt[];
};

struct varrectype {
	int nit[4];
	int x,y;
	float v;
	float i;
	float lum;
};


varrectype * VarTab;
idximageinfo * ImgIdx;
int nVar;
float TotalCurrent=-1;

idximageinfo * IndexVariables(void);
void __cdecl Calculate2DResist(void *);
void DrawResultImage(double time);
void PointInfo(HWND hwnd, int x, int y);
void CalcIDensityGraph(void);
void ProgressInfo(HWND hwnd);
void ExtractVariablesAndStartSolver(HWND hwnd);

bool LoadImageToTwoBuffers(HWND hwnd)
{
	char str[MAX_PATH];
	str[0]=0;
    OPENFILENAME   ofn;
    ZeroMemory( &ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hInstance = hInst;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Portable Network Graphics (*.PNG)\0*.png\0Graphics Interchange Format (*.GIF)\0*.gif\0 bitmap image (*.BMP)\0*.bmp\0All Files (*.*)\0*.*\0";
	ofn.lpstrFile = str;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = "Open 2D Resistance Picture";
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
    if(GetOpenFileName( &ofn )==0) return false;
	CImage src;
	src.Load(str);
	if(src.IsNull()) return false;
	img.Destroy();
	res.Destroy();
	img.Create(src.GetWidth(), src.GetHeight(), 24);
	src.BitBlt(img.GetDC(), 0,0);
	img.ReleaseDC();
	res.Create(src.GetWidth(), src.GetHeight(), 24);
	src.BitBlt(res.GetDC(), 0,0);
	res.ReleaseDC();
	return true;
}

bool LoadResourceImageToTwoBuffers(HINSTANCE hInstance, UINT resID)
{
	CImage src;
	src.LoadFromResource(hInstance, resID);
	if(src.IsNull()) return false;
	img.Destroy();
	res.Destroy();
	img.Create(src.GetWidth(), src.GetHeight(), 24);
	src.BitBlt(img.GetDC(), 0,0);
	img.ReleaseDC();
	res.Create(src.GetWidth(), src.GetHeight(), 24);
	src.BitBlt(res.GetDC(), 0,0);
	res.ReleaseDC();
	return true;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_RESIST2D, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_RESIST2D));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_RESIST2D));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= NULL;
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_RESIST2D);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= NULL;

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	InitCommonControls();
   

	hInst = hInstance; // Store instance handle in our global variable

	hwndA = CreateWindow(szWindowClass, szTitle, MAINWINDOWSTYLE,
		CW_USEDEFAULT, 0, 320, 150, NULL, NULL, hInstance, NULL);

	if(!hwndA)
	{
		return FALSE;
	}
	hwndStat = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | ES_READONLY, 0, 10, 320, 40, hwndA, NULL, hInstance, NULL);

	ShowWindow(hwndA, nCmdShow);
	UpdateWindow(hwndA);
	SetTimer(hwndA, 1, 50, NULL);
#ifndef _DEBUG
	if(LoadResourceImageToTwoBuffers(hInstance, IDB_EXAMPLE1)) 
		ExtractVariablesAndStartSolver(hwndA);
#endif
	return TRUE;
}

void MemoryErrorMsg(HWND hWnd, unsigned nvar)
{
	char str[256];
	sprintf_s(str, "Not enough memory for %ux%u matrix. (%.2fGB)\r\n"
		"Please reduce conductor surface..", nvar, nvar, (1.0/1024/1024/1024)*nvar*nvar*sizeof(double));
	MessageBox(hWnd, str, "ERROR", MB_ICONERROR);
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rect;


	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case ID_FILE_LOAD:
			if(LoadImageToTwoBuffers(hWnd)) ExtractVariablesAndStartSolver(hWnd);
			break;

		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_MOUSEMOVE:
		PointInfo(hwndStat, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;

	case WM_APP+2:
		MemoryErrorMsg(hWnd, (unsigned)wParam);
		break;

	case WM_APP+1:
		nVar = (int)wParam;
		VarTab = (varrectype*)lParam;
		AnimationTime = 0;
		CalcIDensityGraph();
		PointInfo(hwndStat, -1, -1);
		// no break'
	case WM_TIMER:
		if(VarTab) {
			DrawResultImage(AnimationTime);
			AnimationTime += 0.01;
			rect.left=rect.top = 0;
			rect.bottom=img.GetHeight();
			rect.right=img.GetWidth()*2;
			RedrawWindow(hWnd, &rect , NULL, RDW_ERASE | RDW_INVALIDATE);
		} else 
			ProgressInfo(hwndStat);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		if(!img.IsNull()) img.Draw(hdc, 0, 0);
		if(!res.IsNull()) res.Draw(hdc, img.GetWidth(), 0);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}



void ExtractVariablesAndStartSolver(HWND hwnd)
{
	ProgressStage = 0;
	nVar = 0;
	free(ImgIdx);
	free(VarTab);
	TotalCurrent = -1;
	VarTab = NULL;
	ImgIdx = IndexVariables();
	RECT rect = {0, 0, 2*img.GetWidth(), img.GetHeight()+StatHeight};
	AdjustWindowRect(&rect, MAINWINDOWSTYLE, true);
	SetWindowPos(hwnd, NULL, 0, 0, rect.right-rect.left, rect.bottom-rect.top, SWP_NOMOVE | SWP_NOZORDER);
	MoveWindow(hwndStat, 0, img.GetHeight(), rect.right-rect.left, StatHeight, false);
	RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
	if(ImgIdx) _beginthread(Calculate2DResist, 0, hwnd);
}

struct matrixstn {
	double * row;
	int firstNZ;
};


void ScalarMulAndSubst(double a, double * src, double * dst, int i, int nvar)
{
	for(i=i; i<=nvar; i++) dst[i] -= a*src[i];
}

const int Isolator      = -1;
const int ConstVoltRed  = -2; // value 2V
const int ConstVoltBlue = -3; // value 1V
const int VarVoltMax    = -3;
const double UnknownVolt    = -1;


idximageinfo * IndexVariables(void)
{
	int x,y, sizx=img.GetWidth(), sizy=img.GetHeight();
	idximageinfo * ii = (idximageinfo*)malloc(sizx*sizy*(sizeof(int)+sizeof(float))+sizeof(idximageinfo));
	float * lumtab = (float*)&ii->vidxt[sizx*sizy];
	int nvar=0, addr;
	for(addr=y=0; y<sizy; y++)
		for(x=0; x<sizx; x++, addr++)
		{
			COLORREF pix = img.GetPixel(x, y);
			BYTE r = GetRValue(pix);
			BYTE g = GetGValue(pix);
			BYTE b = GetBValue(pix);
			int bw = r+g+b;
			lumtab[addr] = 0.2126f*r + 0.7152f*g +  0.0722f*b;
			if(bw>50)
			{
				if(r*3>bw) {
					ii->vidxt[addr] = ConstVoltRed;  // voltage in this point is RED (constant)
				}
				else if(b*3>bw) {
					ii->vidxt[addr] = ConstVoltBlue;  // voltage in this point is BLUE (constant)
				} else {
					ii->vidxt[addr] = nvar++; // voltage in this point is unknown (variable)
				}
			} else {
				ii->vidxt[addr] = Isolator;  // voltage in this point is not relevant, infinite resistance
			}
		}
	// we have nvar unknown voltages (variables)
	ii->sizx = sizx;
	ii->sizy = sizy;
	ii->nvar = nvar;
	if(nvar>0) return ii; 
	free(ii);
	return NULL;
}


void __cdecl Calculate2DResist(void * par)
{
	ProgressStage=0;
	ProgressDone=0;
	int addr, x,y, nvar=ImgIdx->nvar;
	float * lumtab = (float*)&ImgIdx->vidxt[ImgIdx->sizx*ImgIdx->sizy];
	varrectype * vartab = (varrectype*)malloc(nvar*sizeof(varrectype));
	for(addr=y=0; y<ImgIdx->sizy; y++)
	{
		for(x=0; x<ImgIdx->sizx; x++, addr++)
		{
			int j = ImgIdx->vidxt[addr];
			if(j<0) continue;
			vartab[j].x = x;
			vartab[j].y = y;
			vartab[j].lum = lumtab[addr]*1.0f/255;
			vartab[j].nit[0] = y>0?              ImgIdx->vidxt[addr-ImgIdx->sizx] : Isolator;
			vartab[j].nit[1] = y+1<ImgIdx->sizy? ImgIdx->vidxt[addr+ImgIdx->sizx] : Isolator;
			vartab[j].nit[2] = x>0?              ImgIdx->vidxt[addr-1]            : Isolator;
			vartab[j].nit[3] = x+1<ImgIdx->sizx? ImgIdx->vidxt[addr+1]            : Isolator;
			
		}
	}
	// every variable is dependent on up to 4 connections to other variables or constants

	// we build a matrix nvar lines, nvar+1 columns, representing the equation system to solve
	// i0=(uo-u0)/R, i1=(uo-u1)/R, i2=(uo-u2)/R, i3=(uo-u3)/R
	// we assume R is constant, will be solved globally
	// i0+i1+i2+i3=0
	// uo-u0  +  uo-u1  +  uo-u2  +  uo-u3  = 0
	// 4*uo -1*u0 -1*u1 -1*u2 -1*u3 = 0
	// we set u(RED)=0, u(BLUE)=1, to be scaled later
	// u0,u1,u2,u3 might be constant, sum of all constants goes to the last column
	// coefficient 4 is actually a number of conductive neighburs

	int l,j,i,k,vn, imin;
	matrixstn* matrix = (matrixstn*)malloc(nvar*sizeof(matrixstn));
	for(j=0; j<nvar; j++)
	{
		double * row = (double*)malloc((nvar+1)*sizeof(double));
		if(row==NULL) {
			while(j>0) free(matrix[--j].row);
			free(matrix);
			free(vartab);
			PostMessage((HWND)par, WM_APP+2, nvar, 0);
			_heapmin();
			return;
		}
		matrix[j].row = row;
		ZeroMemory(row, (nvar+1)*sizeof(double));
		vn = 0;
		imin = nvar+1;
		for(k=0; k<4; k++)
		{
			i = vartab[j].nit[k];
			if(i>=0) {
				row[i] = -1;
				if(i<imin) imin=i;
				vn++;
			}
			else if(i==ConstVoltRed) {
				vn++;
				row[nvar] += 2;
			}
			else if(i==ConstVoltBlue) {
				vn++;
				row[nvar] += 1;
			}
		}
		row[j] = vn;
		if(vn!=0 && j<imin) imin=j;
		matrix[j].firstNZ = imin;
	}

	// now we need to solve the eq-system, a sparse system, at the moment reasonably pivoted
	// we do no tricks or fast algorithms
	// Gaussian Elimination
	ProgressStage=1;
	matrixstn tmp;
	for(l=0; l<nvar; l++)
	{
		ProgressDone = float(l)/nvar;
		double d = abs(matrix[l].row[l]);
		for(j=l+1; j<nvar; j++)
			if(abs(matrix[j].row[l]) > d) 
			{
				tmp = matrix[j];
				matrix[j] = matrix[l];
				matrix[l] = tmp;
				d = abs(matrix[l].row[l]);
			}
		
		for(j=l+1; j<nvar; j++)
		{
			int firstNZ = matrix[j].firstNZ;
			if(firstNZ <= l) 
			{
				ScalarMulAndSubst(
					matrix[j].row[l] / matrix[l].row[l], 
					matrix[l].row, 
					matrix[j].row, 
					firstNZ+1, nvar);
				matrix[j].row[l] = 0;
				firstNZ++;
				while( matrix[j].row[firstNZ]==0) firstNZ++;
				matrix[j].firstNZ = firstNZ;
			} 
		}	
	}
	// backward substitution
	ProgressStage=2;
	k=l=nvar-1;
	while(l>=0 && k>=0)
	{
		ProgressDone = float(nvar-l)/nvar;
		if(matrix[l].firstNZ>k) {
			l--;
		} 
		else if(matrix[l].firstNZ==k)
		{
			double d = matrix[l].row[nvar] / matrix[l].row[k];
			vartab[k].v = float(d);
			for(j=l-1; j>=0; j--) 
			{
				matrix[j].row[nvar] -= matrix[j].row[k]*d;
				matrix[j].row[k] = 0;
			}
			k--;
			l--;
		} else {
			double d = UnknownVolt;
			vartab[k].v = float(UnknownVolt);
			for(j=l; j>=0; j--) 
			{
				matrix[j].row[nvar] -= matrix[j].row[k]*d;
				matrix[j].row[k] = 0;
			}
			k--;
		}
	}
	while(k>=0) vartab[k--].v = float(UnknownVolt);
	for(j=0; j<nvar; j++) {
		free(matrix[j].row);
		if(vartab[j].v<0.95 || vartab[j].v>2.05) vartab[j].v = float(UnknownVolt);
	}
	free(matrix);
	PostMessage((HWND)par, WM_APP+1, nvar, LPARAM(vartab));
	_heapmin();

	// total current: say, from the red terminal
	double iRed=0;
	for(i=0; i<nvar; i++) 
	{
		if(vartab[i].v>=0.25)
		{
			for(k=0; k<4; k++) 
				if(vartab[i].nit[k]==ConstVoltRed) 
					iRed += 2-vartab[i].v;
		}
	}
	TotalCurrent = float(iRed);
	ProgressStage=3;
}

void DrawResultImage(double time)
{
	if(res.IsNull()) return;
	for(int idx=0; idx<nVar; idx++)
	{
		double v = VarTab[idx].v;
		float r,g,b;
		if(v<0) r=g=b=100; else 
		{
			v += time;
			double u;
			float sql = (float)modf(v*7, &u);
			sql = sql<0.125f ? (sql*16-1) : sql<0.5f ? 1 : sql<0.625f ? 1-(sql-0.5f)*16 : -1;
			b = 127+63*(float)sin(v*7)+50*sql;
			r = 127+53*(float)sin(v*9)+60*sql;
			g = 127+63*(float)sin(v*11)+40*sql;
		}
		float lum = VarTab[idx].lum;
		res.SetPixel(VarTab[idx].x, VarTab[idx].y, (BYTE(floor(b*lum))<<16) | (BYTE(floor(g*lum))<<8) | BYTE(floor(r*lum)));
	}
}

int calcDP(double val)
{
	val=abs(val); // if might be more clear than log10
	if(val>=100) return 0; 
	if(val>=10) return 1; 
	if(val>=1) return 2; 
	if(val>=0.1) return 3; 
	if(val>=0.01) return 4; 
	if(val>=0.001) return 5; 
	return 6;
}

void PointInfo(HWND hwnd, int x, int y)
{
	char str[256];
	str[0]=0;
	if(VarTab==NULL) return;
	if(ImgIdx && x>=0 && x<2*ImgIdx->sizx && y>=0 && y<ImgIdx->sizy)
	{
		if(x >= ImgIdx->sizx) x -= ImgIdx->sizx;
		int i = ImgIdx->vidxt[x + y*ImgIdx->sizy];
		if(i>=0) {
			float uo = VarTab[i].v;
			if(uo>-0.05)
			{
				float nu, io=0;
				for(int k=0; k<4; k++) 
				{
					int ni = VarTab[i].nit[k];
					if(ni>=0) nu = VarTab[ni].v;
					else if(ni==ConstVoltRed) nu=2;
					else if(ni==ConstVoltBlue) nu=1;
					else nu=uo;
					nu -= uo;
					if(nu>0) io += nu; // R=1
				}
				sprintf_s(str, "U=%.3f I=%.*f at (%i,%i)", uo-1.0, calcDP(io), io, x, y);
			} else
				sprintf_s(str, "Separate Conductor at (%i,%i)", x, y);
		} 
		else if(i==ConstVoltRed) sprintf_s(str, "Red Terminal U=1 I=%.*f at (%i,%i)", calcDP(TotalCurrent), TotalCurrent, x, y);
		else if(i==ConstVoltBlue) sprintf_s(str, "Blue Terminal U=0 I=%.*f at (%i,%i)", calcDP(TotalCurrent), TotalCurrent, x, y);
	} 
	if(str[0]==0) sprintf_s(str, "Resistance: %.*f", calcDP(1/TotalCurrent), 1/TotalCurrent);
	SetWindowText(hwnd, str);
}


void CalcIDensityGraph(void)
{
	float maxio = 0;
	int i;
	for(i=0; i<nVar; i++)
	{
		float uo = VarTab[i].v;
		if(uo<-0.05) VarTab[i].i = 0; else
		{
			float nu, io=0;
			for(int k=0; k<4; k++) 
			{
				int ni = VarTab[i].nit[k];
				if(ni>=0) nu = VarTab[ni].v;
				else if(ni==ConstVoltRed) nu=2;
				else if(ni==ConstVoltBlue) nu=1;
				else nu=uo;
				nu -= uo;
				if(nu>0) io += nu; // R=1
			}
			VarTab[i].i = io;
			if(io>maxio) maxio=io;
		}
	}

	for(i=0; i<nVar; i++)
	{
		float io = VarTab[i].i;
		if(io>0) 
		{
			io = sqrt(io/maxio);
			float g = io<0.5? 0 : (io-0.5f)*(2*255);
			float r = sin(io*(3.141592f/2))*255;
			float b = (sin(io*(3.141592f*2))/2 + io*io)*255;
			float lum = VarTab[i].lum;
			img.SetPixel(VarTab[i].x, VarTab[i].y, (BYTE(floor(b*lum))<<16) | (BYTE(floor(g*lum))<<8) | BYTE(floor(r*lum)));
		} else
			img.SetPixel(VarTab[i].x, VarTab[i].y, 0);
	}
}

void ProgressInfo(HWND hwnd)
{
	char str[128];
	switch(ProgressStage) {
	case 0: sprintf_s(str, "Create Matrix"); break;
	case 1: sprintf_s(str, "Gaussian elimination, Progress %.1f%% of rows", ProgressDone*100); break;
	case 2: sprintf_s(str, "backward substitution, Progress %.1f%% of rows", ProgressDone*100); break;
	default: sprintf_s(str, "Done"); break;
	}
	SetWindowText(hwnd, str);
}
