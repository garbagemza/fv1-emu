// fv1-emu.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "fv1-emu.h"
#include "Utilities\SoundClass.h"
#include "Utilities\SoundUtilities.h"
#include "Utilities\ParseUtil.h"
#include "Utilities\SCLog.h"
#include "Manager\TimerManager.h"
#include "Manager\MemoryManager.h"

#include "Services\FV1.h"
#include "Services\Lexer.h"
#include "Services\Parser.h"

#include <commdlg.h>
#include <vector>
#include <map>
#include <ctime>
using namespace std;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

SoundClass* m_Sound = 0;
struct SpinFile;
SpinFile* spinResult = 0;
FV1* fv1 = 0;


class SpinSoundDelegate : public ISoundDelegate {
	void willBeginPlay();
	void getAudioChunk(LPVOID, DWORD, DWORD&);
	void didEndPlay();

	BOOL ExecuteInstruction(Instruction* inst, unsigned int index, unsigned int& skipLines);
	void UpdateDelayMemories();

	SignalGenerator* generator = 0;
	SpinFile* spinFile = 0;

	Timer* timer = 0;

	unsigned int memCount = 0;
	Memory* memoryArray[128];

	bool isFirstExecution;

public:
	SpinSoundDelegate(SpinFile* spinFile) {
		this->spinFile = spinFile;
	}
};

// Forward declarations of functions included in this code module:
ATOM					MyRegisterClass(HINSTANCE hInstance);
BOOL					InitInstance(HINSTANCE, int);
LRESULT CALLBACK		WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK		About(HWND, UINT, WPARAM, LPARAM);
VOID					PromptOpenFile();
VOID					OpenFileForReadAndLoad(LPWSTR filename);
VOID					LoadFile(HANDLE file, FV1*);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_FV1_EMU, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}
	SCLogInfo(L"App initialized.");

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FV1_EMU));

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
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FV1_EMU));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_FV1_EMU);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

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
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   // Create the sound object.
   m_Sound = new SoundClass;
   if (!m_Sound)
   {
	   return false;
   }

   // Initialize the sound object.
   bool result = m_Sound->Initialize(hWnd);
   if (!result)
   {
	   MessageBox(hWnd, L"Could not initialize Direct Sound.", L"Error", MB_OK);
	   return false;
   }

   SCLog::mainInstance()->setupLogging();

   // initialize fv1
   fv1 = new FV1();

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
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

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		SCLogInfo(L"MENU: chosen ID: %d", wmId);
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case ID_FILE_OPEN:
			PromptOpenFile();
			break;
		case IDM_GENERATOR:
			{
				bool created = m_Sound->CreateSoundBuffer();
				if (created) {
					m_Sound->SetDelegate(new SpinSoundDelegate(spinResult));
					m_Sound->Play();
				}
			}
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
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

void SpinSoundDelegate::willBeginPlay() {

	generator = SoundUtilities::createSignalGenerator(SignalType::Sinusoidal, 200.0);
	timer = TimerManager::createTimer(44100);
	
	if (this->spinFile != NULL) {
		// load first pass
		map<string, Memory*> memories = spinFile->memMap;
		unsigned int index = 0;
		for (map<string, Memory*>::iterator it = memories.begin(); it != memories.end(); it++) {
			Memory* mem = (*it).second;
			memoryArray[index] = mem;
			index++;
		}
		memCount = index;

		if (memories.size() == memCount) {
			spinFile->passOneSemanticSucceeded = true;
		}

		isFirstExecution = true;
	}
}

void SpinSoundDelegate::getAudioChunk(LPVOID buffer, DWORD sampleCount, DWORD &dwRetSamples) {

	clock_t begin = clock();
	bool spinLoaded = spinFile != NULL && spinFile->passOneSemanticSucceeded;
	unsigned int instructionCount = spinLoaded ? spinFile->passTwo.instructionCount : 0;
	Instruction** instructions = spinLoaded ? spinFile->passTwo.instructions : NULL;

	for (unsigned int i = 0; i < sampleCount; i++) {
		TimerManager::updateTimerWithSampleNumber(timer, i);

		fv1->adcl = 0.5 * SoundUtilities::sampleWithTime(generator, timer);
		fv1->adcr = fv1->adcl;
		
		if (i > 9000) {
			fv1->adcl = 0;			// cut sound
			fv1->adcr = 0;
		}

		if (spinLoaded) {

			for (unsigned int i = 0; i < instructionCount; i++) {
				Instruction* inst = instructions[i];
				unsigned int skipLines = 0;
				BOOL ok = ExecuteInstruction(inst, i, skipLines);
				if (ok && skipLines != 0) {
					i += skipLines;
				}
				else if (!ok) {
					throw exception("unrecognized instruction");
				}
			}

			UpdateDelayMemories();
			if (isFirstExecution) {
				isFirstExecution = false;
			}
		}
		else {
			// if not spin file loaded then copy input registers to output registers
			fv1->dacl = fv1->adcl;
			fv1->dacr = fv1->adcr;
		}

		((short*)buffer)[i] = (short)(8000.0 * (fv1->dacl + fv1->dacr));
	}

	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	printf("Elapsed time: %f seconds", elapsed_secs);

	dwRetSamples = sampleCount;
}

BOOL SpinSoundDelegate::ExecuteInstruction(Instruction* inst, unsigned int index, unsigned int & skipLines) {

	switch (inst->opcode) {
	case RDAX:
	{
		double* regValue = inst->args[0]->regAddress;
		fv1->rdax((*regValue), inst->args[1]->doubleValue);
		return true;
	}
	break;
	case RDA:
	{
		fv1->rda(inst->args[0]->memAddress, inst->args[0]->dir, inst->args[1]->doubleValue);
		return true;
	}
	break;
	case WRAP:
	{
		fv1->wrap(inst->args[0]->memAddress->mem, inst->args[1]->doubleValue);
		return true;
	}
	break;
	case WRAX:
	{
		fv1->wrax(inst->args[0]->regAddress, inst->args[1]->doubleValue);
		return true;
	}
	break;
	case WRA:
	{
		fv1->wra(inst->args[0]->memAddress->mem, inst->args[1]->doubleValue);
		return true;
	}
	break;
	case MULX:
	{
		fv1->mulx(inst->args[0]->regAddress);
		return true;
	}
	break;
	case RDFX:
	{
		fv1->rdfx(inst->args[0]->regAddress, inst->args[1]->doubleValue);
		return true;
	}
	break;
	case LOG:
	{
		fv1->fv1log(inst->args[0]->doubleValue, inst->args[1]->doubleValue);
		return true;
	}
	break;
	case EXP:
	{
		fv1->fv1exp(inst->args[0]->doubleValue, inst->args[1]->doubleValue);
		return true;
	}
	break;
	case SOF:
	{
		fv1->sof(inst->args[0]->doubleValue, inst->args[1]->doubleValue);
		return true;
	}
	break;
	case MAXX:
	{
		fv1->maxx(inst->args[0]->regAddress, inst->args[1]->doubleValue);
		return true;
	}
	break;
	case LDAX:
	{
		fv1->ldax(inst->args[0]->regAddress);
		return true;
	}
	break;
	case SKP:
	{
		FV1::SkipCondition condition = inst->args[0]->condition;
		bool executeSkip = false;
		switch (condition) {
		case FV1::RUN:
			executeSkip = !isFirstExecution;
			break;
		case FV1::ZRC:
			executeSkip = fv1->zrc();
			break;
		case FV1::ZRO:
			executeSkip = fv1->zro();
			break;
		case FV1::GEZ:
			executeSkip = fv1->gez();
			break;
		case FV1::NEG:
			executeSkip = fv1->neg();
			break;
		}

		skipLines = executeSkip ? (unsigned int)inst->args[1]->doubleValue : 0;

		return true;
	}
	break;
	case ABSA:
	{
		fv1->absa();
		return true;
	}
	break;
	case WRLX:
	{
		fv1->wrlx(inst->args[0]->regAddress, inst->args[1]->doubleValue);
		return true;
	}
	break;
	case WRHX:
	{
		fv1->wrhx(inst->args[0]->regAddress, inst->args[1]->doubleValue);
		return true;
	}
	break;
	case WLDS:
	{
		FV1::SineOscillator osc = inst->args[0]->osc;
		fv1->wlds(osc, inst->args[1]->doubleValue, inst->args[2]->doubleValue);
		return true;
	}
	}
	return false;
}

void SpinSoundDelegate::UpdateDelayMemories() {
	for (unsigned int i = 0; i < memCount; i++) {
		Memory* mem = memoryArray[i];
		fv1->updm(mem);
	}
}

void SpinSoundDelegate::didEndPlay()
{

}

void PromptOpenFile() {
	SCLogFunction();

	OPENFILENAME ofn;
	LPWSTR filename = new TCHAR[MAX_PATH];
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = 0;
	ofn.lpstrDefExt = 0;
	ofn.lpstrFile = filename;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = L"Spin Files(*.spn)\0*.spn\0\0";
	ofn.nFilterIndex = 0;
	ofn.lpstrInitialDir = 0;
	ofn.lpstrTitle = 0;
	
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	GetOpenFileName(&ofn);

	if (_tcslen(filename) > 0) {
		OpenFileForReadAndLoad(filename);
	}

}

void OpenFileForReadAndLoad(LPWSTR filename) {
	SCLogInfo(L"Prepare to open file %w", filename);

	HANDLE file = CreateFile(filename,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
		);
	if (file != 0) {
		LoadFile(file, fv1);
		CloseHandle(file);
	}
}

void LoadFile(HANDLE file, FV1* fv1) {
	
	DWORD fileSizeHigh;
	DWORD numberOfBytesRead;

	DWORD size = GetFileSize(file, &fileSizeHigh);

	LPVOID lpBuffer = (LPVOID)new byte[size];

	// ensure the file is bigger than zero and small enough for a typical program
	if (size > 0 && fileSizeHigh == 0) {
	
		BOOL read = ReadFile(file, lpBuffer, size, &numberOfBytesRead, NULL);
		if (read && numberOfBytesRead == size) {
			Parser parser = Parser(fv1);
			// gather all code lines for execution
			ExecutionVectorResult lexicalResult = parser.beginLexicalAnalysis(lpBuffer, size);
			if (lexicalResult.success) {
				// do pass one; gather all delay memory and equates
				BOOL success = parser.PassOneParse(lexicalResult.firstPass);
				if (success) {

					// pass two, check if all instructions can be interpreted
					// gather all instructions
					PassTwoResult passTwoResult = parser.PassTwoParse(lexicalResult.secondPass, lexicalResult.secondPassRawLines);
					if (passTwoResult.success) {

						spinResult = new SpinFile();
						spinResult->memMap = parser.memMap;
						spinResult->equMap = parser.equMap;
						spinResult->passTwo = passTwoResult;
						MessageBeep(0);
					}

				}
			}
		}
	}

	// free memory
	delete[] lpBuffer;
}