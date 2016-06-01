// BigPedalboard.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "BigPedalboard.h"
#include "Utilities\SoundClass.h"
#include "Utilities\SoundUtilities.h"
#include "Utilities\ParseUtil.h"
#include "Manager\TimerManager.h"
#include "Manager\MemoryManager.h"

#include "Services\FV1.h"
#include "Services\Lexer.h"

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

struct MemoryAddress {
	Memory* mem = 0;
	signed int displacement = 0;
};

struct Param
{
	Lexer::TOKEN_TYPE type;
	string value;
	FV1::MemoryPosition dir;
	double doubleValue;

	double* regAddress;
	MemoryAddress* memAddress;

	// used for skp instruction
	FV1::SkipCondition condition;
	unsigned int skipLines;

	Param() {
		dir = FV1::Start;
		doubleValue = 0;
		regAddress = 0;
		memAddress = 0;
		condition = FV1::UNKNOWN;
		skipLines = 0;
	}
};

enum InstructionType {
	UNKNOWN,
	RDAX,
	RDA,
	WRAP,
	WRAX,
	WRA,
	MULX,
	RDFX,
	LOG,
	EXP,
	SOF,
	MAXX,
	LDAX,
	SKP,
	ABSA,
	WRLX,
	WRHX,
};

struct Instruction
{
	InstructionType type;
	Param* arg0;
	Param* arg1;
};

struct ExecutionVectorResult
{
	bool success;
	map<string, unsigned int> labels;
	vector<vector<Lexer::Token*>> firstPass;
	vector<vector<Lexer::Token*>> secondPass;
};

struct PassOneResult {
	bool success;
	map<string, Memory*> memoryMap;
	map<string, Param> equatesMap;
};

struct PassTwoResult {
	bool success;
	unsigned int instructionCount;
	Instruction* instructions[128];
};

struct SpinFile {
	PassOneResult passOne;
	PassTwoResult passTwo;
	bool passOneSemanticSucceeded;

	SpinFile() {
		passOneSemanticSucceeded = false;
	}
};

struct SplitStatementInfo
{
	bool shouldSplit;
	string labelDeclared;
	vector<Lexer::Token*> secondPassStatement;
};

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


public:
	SpinSoundDelegate(SpinFile* spinFile) {
		this->spinFile = spinFile;
	}
};

class MinReverbDelegate : public ISoundDelegate {


	void willBeginPlay();
	void getAudioChunk(LPVOID, DWORD, DWORD&);
	void didEndPlay();

	SignalGenerator* generator = 0;
	
	FV1* fv1 = new FV1();

	Timer* timer = 0;
	Memory* api1 = 0;
	Memory* api2 = 0;
	Memory* api3 = 0;
	Memory* api4 = 0;
	Memory* ap1 = 0;
	Memory* ap2 = 0;
	Memory* del1 = 0;
	Memory* del2 = 0;

	const double krt = 0.7; //adjust reverb time
	const double kap = 0.625; //adjust AP coefficients

	double apout = 0;
	double dacl = 0;
	double dacr = 0;


};

// Forward declarations of functions included in this code module:
ATOM					MyRegisterClass(HINSTANCE hInstance);
BOOL					InitInstance(HINSTANCE, int);
LRESULT CALLBACK		WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK		About(HWND, UINT, WPARAM, LPARAM);
VOID					PromptOpenFile();
VOID					OpenFileForReadAndLoad(LPWSTR filename);
VOID					LoadFile(HANDLE file, FV1*);
PassOneResult			PassOneParse(FV1*, vector<vector<Lexer::Token*>>);
PassTwoResult			PassTwoParse(FV1*, map<string, Param>, map<string, Memory*>, map<string, unsigned int>, vector<vector<Lexer::Token*>>);
ExecutionVectorResult	beginLexicalAnalysis(LPVOID lpBuffer, DWORD size);
ExecutionVectorResult	generateExecutionVector(vector<vector<Lexer::Token*>>);
BOOL					isFirstPassStatement(vector<Lexer::Token*>);
SplitStatementInfo		shouldSplitStatements(vector<Lexer::Token*>);
BOOL					LoadInstructionWithInstructionLine(FV1*, map<string, Param>, map<string, Memory*>, map<string, unsigned int>, vector<Lexer::Token*>, unsigned int, Instruction*);
InstructionType			InstructionTypeWithString(string&);
FV1::MemoryPosition		DirectionSpecificationWithType(Lexer::TOKEN_TYPE&);

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
	LoadString(hInstance, IDC_BIGPEDALBOARD, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BIGPEDALBOARD));

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
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BIGPEDALBOARD));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_BIGPEDALBOARD);
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

	generator = SoundUtilities::createSignalGenerator(SignalType::Triangle, 200.0);
	timer = TimerManager::createTimer(44100);
	
	if (this->spinFile != NULL) {
		// load first pass
		map<string, Memory*> memories = spinFile->passOne.memoryMap;
		unsigned int index = 0;
		for (map<string, Memory*>::iterator it = memories.begin(); it != memories.end(); it++) {
			Memory* mem = (*it).second;
			memoryArray[index] = mem;
			index++;
		}
		memCount = index;

		map<string, Param> equates = spinFile->passOne.equatesMap;
		if (memories.size() == memCount) {
			spinFile->passOneSemanticSucceeded = true;
		}
	}
}

void MinReverbDelegate::willBeginPlay()
{
	generator = SoundUtilities::createSignalGenerator(SignalType::Sinusoidal, 200.0);
	timer = TimerManager::createTimer(44100);

	fv1->mem(&api1, 122);
	fv1->mem(&api2, 303);
	fv1->mem(&api3, 553);
	fv1->mem(&api4, 922);

	fv1->mem(&ap1, 3823);
	fv1->mem(&del1, 8500);

	fv1->mem(&ap2, 4732);
	fv1->mem(&del2, 7234);
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
		
		//if (i > 9000) {
			//fv1->adcl = 0;			// cut sound
			//fv1->adcr = 0;
		//}

		if (spinLoaded) {

			for (unsigned int i = 0; i < instructionCount; i++) {
				Instruction* inst = instructions[i];
				unsigned int skipLines = 0;
				bool ok = ExecuteInstruction(inst, i, skipLines);
				if (ok && skipLines != 0) {
					i += skipLines;
				}
				else if (!ok) {
					throw exception("unrecognized exception");
				}
			}

			UpdateDelayMemories();
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
	
	switch (inst->type) {
	case RDAX:
	{
		double* regValue = inst->arg0->regAddress;
		fv1->rdax((*regValue), inst->arg1->doubleValue);
		return true;
	}
	break;
	case RDA:
	{
		fv1->rda(inst->arg0->memAddress->mem, inst->arg0->dir, inst->arg1->doubleValue);
		return true;
	}
	break;
	case WRAP:
	{
		fv1->wrap(inst->arg0->memAddress->mem, inst->arg1->doubleValue);
		return true;
	}
	break;
	case WRAX:
	{
		fv1->wrax(inst->arg0->regAddress, inst->arg1->doubleValue);
		return true;
	}
	break;
	case WRA:
	{
		fv1->wra(inst->arg0->memAddress->mem, inst->arg1->doubleValue);
		return true;
	}
	break;
	case MULX:
	{
		fv1->mulx(inst->arg0->regAddress);
		return true;
	}
	break;
	case RDFX:
	{
		fv1->rdfx(inst->arg0->regAddress, inst->arg1->doubleValue);
		return true;
	}
	break;
	case LOG:
	{
		fv1->fv1log(inst->arg0->doubleValue, inst->arg1->doubleValue);
		return true;
	}
	break;
	case EXP:
	{
		fv1->fv1exp(inst->arg0->doubleValue, inst->arg1->doubleValue);
		return true;
	}
	break;
	case SOF:
	{
		fv1->sof(inst->arg0->doubleValue, inst->arg1->doubleValue);
		return true;
	}
	break;
	case MAXX:
	{
		fv1->maxx(inst->arg0->regAddress, inst->arg1->doubleValue);
		return true;
	}
	break;
	case LDAX:
	{
		fv1->ldax(inst->arg0->regAddress);
		return true;
	}
	break;
	case SKP:
	{
		FV1::SkipCondition condition = inst->arg0->condition;
		bool executeSkip = false;
		switch (condition) {
		case FV1::RUN:
			executeSkip = index > 0;
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

		skipLines = executeSkip ? inst->arg1->skipLines : 0;

		return true;
	}
	break;
	case ABSA:
	{
		fv1->absa();
		return true;
	}
	break;
	}
	return false;
}

void SpinSoundDelegate::UpdateDelayMemories() {
	for (unsigned int i = 0; i < memCount; i++) {
		Memory* mem = memoryArray[i];
		fv1->updm(mem);
	}
}

void MinReverbDelegate::getAudioChunk(LPVOID buffer, DWORD sampleCount, DWORD &dwRetSamples) {

	clock_t begin = clock();

	for (unsigned int i = 0; i < sampleCount; i++) {
		TimerManager::updateTimerWithSampleNumber(timer, i);

		double adcl = 0.5 * SoundUtilities::sampleWithTime(generator, timer);
		double adcr = adcl;

		if (i > 9000) {
			adcl = 0;			// cut sound
			adcr = 0;
		}

		fv1->rdax(adcl, 0.25);					//rdax adcl, 0.25 ; read inputs,
		fv1->rdax(adcr, 0.25);					//attenuate, sum and

		fv1->rda(api1, FV1::End, kap);			//rda api1#, kap; do 4 APs
		fv1->wrap(api1, -kap);					//wrap api1, -kap

		fv1->rda(api2, FV1::End, kap);			//rda api2#, kap
		fv1->wrap(api2, -kap);					//wrap api2, -kap

		fv1->rda(api3, FV1::End, kap);			//rda api3#, kap
		fv1->wrap(api3, -kap);					//wrap api3, -kap

		fv1->rda(api4, FV1::End, kap);			//rda api4#, kap
		fv1->wrap(api4, -kap);					//wrap api4, -kap

		fv1->wrax(&apout, 1.0);					//wrax	apout, 1; write to min, keep in ACC

												//; first loop apd :
												//; AP'd input in ACC
		
		fv1->rda(del2, FV1::End, krt);			//rda del2#, krt; read del2, scale by Krt
		fv1->rda(ap1, FV1::End, -kap);			//rda ap1#, -kap; do loop ap

		fv1->wrap(ap1, kap);					//wrap	ap1, kap
		fv1->wra(del1, 1.99);					//wra	del1, 1.99; write delay, x2 for dac out
		fv1->wrax(&dacl, 0.0);					//wrax	dacl, 0
		
												//; second loop apd :

		fv1->rdax(apout, 1.0);					//; get input signal again
		fv1->rda(del1, FV1::End, krt);			// as above, to other side of loop
		fv1->rda(ap2, FV1::End, kap);
		fv1->wrap(ap2, -kap);
		fv1->wra(del2, 1.99);
		fv1->wrax(&dacr, 0);

		// update memory indexes
		fv1->updm(api1);
		fv1->updm(api2);
		fv1->updm(api3);
		fv1->updm(api4);
		fv1->updm(ap1);
		fv1->updm(ap2);
		fv1->updm(del1);
		fv1->updm(del2);

		((short*)buffer)[i] = (short)(8000.0 * (dacl + dacr));

	}
	clock_t end = clock();
	double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
	printf("Elapsed time: %f seconds", elapsed_secs);

	dwRetSamples = sampleCount;
}

void SpinSoundDelegate::didEndPlay()
{

}

void MinReverbDelegate::didEndPlay()
{

}

void PromptOpenFile() {
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
			// gather all code lines for execution
			ExecutionVectorResult lexicalResult = beginLexicalAnalysis(lpBuffer, size);
			if (lexicalResult.success) {
				// do pass one; gather all delay memory and equates
				PassOneResult passOneResult = PassOneParse(fv1, lexicalResult.firstPass);
				if (passOneResult.success) {

					// pass two, check if all instructions can be interpreted
					// gather all instructions
					PassTwoResult passTwoResult = PassTwoParse(fv1, passOneResult.equatesMap, passOneResult.memoryMap, lexicalResult.labels, lexicalResult.secondPass);
					if (passTwoResult.success) {
						spinResult = new SpinFile();
						spinResult->passOne = passOneResult;
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


ExecutionVectorResult beginLexicalAnalysis(LPVOID lpBuffer, DWORD size) {
	Lexer lex = Lexer(lpBuffer, size);
	vector<vector<Lexer::Token*>> lines;
	while (lex.hasNextLine()) {
		try {
			vector<Lexer::Token*> line = lex.getNextLine();
			if (line.size() > 0) {
				lines.push_back(line);
			}

		}
		catch (std::exception& e) {
			ExecutionVectorResult err = ExecutionVectorResult();
			err.success = false;
			return err;
		}
	}

	if (lines.size() > 0) {
		// ok, the lexical analysis has been successful
		return generateExecutionVector(lines);
	}

	ExecutionVectorResult err = ExecutionVectorResult();
	err.success = false;
	return err;
}

// first pass is to identify memory declarations and equates
// initializes memory and equates dictionaries
PassOneResult PassOneParse(FV1* fv1, vector<vector<Lexer::Token*>> lines) {
	PassOneResult result = PassOneResult();
	result.success = false;


	for (vector<vector<Lexer::Token*>>::iterator it = lines.begin(); it != lines.end(); it++) {
		vector<Lexer::Token*> v = (*it);
		if (v.size() == 3) {
			Lexer::TOKEN_TYPE type = v[0]->type;
			if (type == Lexer::TOKEN_TYPE::IDENTIFIER) {
				string id = v[0]->name;
				if (id.compare("mem") == 0) {
					if (v[1]->type == Lexer::TOKEN_TYPE::IDENTIFIER 
						&& v[2]->type == Lexer::TOKEN_TYPE::NUMBER) {
						unsigned int sizeUInt = 0;
						STR2NUMBER_ERROR err = str2uint(sizeUInt, v[2]->name.c_str());
						if (err == SUCCESS) {
							Memory* memory = MemoryManager::createMemory(sizeUInt);
							if (memory != NULL) {
								result.memoryMap[v[1]->name] = memory;

							}
						}
					}
				}
				else if (id.compare("equ") == 0) {
					if (v[1]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {
						Param equ = Param();
						equ.type = v[2]->type;
						equ.value = v[2]->name;
						if (v[2]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {
							equ.regAddress = fv1->getAddressOfIdentifier(v[2]->name);
						}
						else if (v[2]->type == Lexer::TOKEN_TYPE::NUMBER) {
							double coeff = 0;
							STR2NUMBER_ERROR err = str2dble(coeff, v[2]->name.c_str());
							if (err == SUCCESS) {
								equ.doubleValue = coeff;
							}
						}
						result.equatesMap[v[1]->name] = equ;
					}
				}
			}
		}

	}
	result.success = true;
	return result;
}


PassTwoResult PassTwoParse(FV1* fv1, map<string, Param> equMap, map<string, Memory*> memMap, map<string, unsigned int> labels, vector<vector<Lexer::Token*>> lines) {
	PassTwoResult result = PassTwoResult();
	result.success = false;
	unsigned int index = 0;
	for (vector<vector<Lexer::Token*>>::iterator it = lines.begin(); it != lines.end(); it++) {
		vector<Lexer::Token*> v = (*it);
		Instruction* inst = new Instruction();
		BOOL instructionLoaded = LoadInstructionWithInstructionLine(fv1, equMap, memMap, labels, v, index, inst);
		if (instructionLoaded) {
			result.instructions[index] = inst;
		}
		else {
			return result;
		}
		index++;
	}

	result.instructionCount = index;
	result.success = true;
	return result;
}

BOOL LoadInstructionWithInstructionLine(FV1* fv1, map<string, Param> equMap, map<string, Memory*> memMap, map<string, unsigned int> labels, vector<Lexer::Token*> line, unsigned int currentInstruction, Instruction* instruction) {
	if (line.size() > 0) {
		Lexer::TOKEN_TYPE type = line[0]->type;
		if (type == Lexer::TOKEN_TYPE::IDENTIFIER) {
			string inst = line[0]->name;
			InstructionType type = InstructionTypeWithString(inst);
			if (type != UNKNOWN) {
				line.erase(line.begin() + 0); // remove first element
				instruction->type = type;

				if (line.size() > 2) {

					FV1::MemoryPosition position = FV1::Start;
					Param* arg0 = new Param();

					if (line[0]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {
						arg0->type = line[0]->type;
						arg0->value = line[0]->name;

						line.erase(line.begin());

						// next token is optional, it can be numeral (#) or circumflex (^)
						// if followed by a comma, the direction specification is the default.
						if (line[0]->type == Lexer::TOKEN_TYPE::NUMERAL || line[0]->type == Lexer::TOKEN_TYPE::CIRCUMFLEX) {
							position = DirectionSpecificationWithType(line[0]->type);
							line.erase(line.begin());
						}

						arg0->dir = position;
						arg0->regAddress = fv1->getAddressOfIdentifier(arg0->value);
						if (arg0->regAddress == NULL) {
							map<string, Param>::iterator it = equMap.find(arg0->value);
							if (it != equMap.end()) {
								Param param = (*it).second;
								arg0->regAddress = param.regAddress;
							}
						}

						map<string, Memory*>::iterator it = memMap.find(arg0->value);
						if (it != memMap.end()) {
							MemoryAddress* memAddress = new MemoryAddress();
							memAddress->mem = (*it).second;
							// memory address found, look for any modifier to the memory address
							if (line.size() > 3 && (line[0]->type == Lexer::TOKEN_TYPE::PLUS || line[0]->type == Lexer::TOKEN_TYPE::MINUS)) {
								// there is a displacement here
								bool negative = false;
								if (line[0]->type == Lexer::TOKEN_TYPE::MINUS) {
									// is negative
									negative = true;
								}
								line.erase(line.begin()); // erase plus or minus token
								if (line[0]->type == Lexer::TOKEN_TYPE::NUMBER) {
									unsigned int displacement = 0;
									STR2NUMBER_ERROR err = str2uint(displacement, line[0]->name.c_str());
									if (err == SUCCESS) {
										memAddress->displacement = negative ? displacement * -1 : displacement;
									}

									line.erase(line.begin()); // erase number
								}
								else {
									throw exception("a number was expected to construct a displacement.");
								}
							}

							arg0->memAddress = memAddress;


						}

						if (arg0->regAddress == NULL && arg0->memAddress == NULL) {
							arg0->condition = fv1->conditionWithIdentifier(arg0->value);
						}
					}
					else if (line[0]->type == Lexer::TOKEN_TYPE::NUMBER || line[0]->type == Lexer::TOKEN_TYPE::MINUS) {
						// log and exp should have as first argument a number
						bool negative = false;
						if (line[0]->type == Lexer::TOKEN_TYPE::MINUS) {
							line.erase(line.begin());
							negative = true;
						}

						if (line[0]->type == Lexer::TOKEN_TYPE::NUMBER) {
							arg0->type = line[0]->type;
							arg0->value = line[0]->name;
							double coefficient = 0;
							STR2NUMBER_ERROR err = str2dble(coefficient, line[0]->name.c_str());
							if (err == SUCCESS) {
								arg0->doubleValue = negative ? coefficient * -1.0 : coefficient;
							}
							line.erase(line.begin()); // erase number
						}
					}


					if (instruction->type == RDAX || instruction->type == WRAX || instruction->type == MAXX || instruction->type == WRLX) {
						if (arg0->regAddress == NULL) {
							throw exception("register address is required for first argument.");
						}
					}

					if (instruction->type == RDA || instruction->type == WRA || instruction->type == WRAP) {
						if (arg0->memAddress == NULL) {
							throw exception("memory is required for first argument.");
						}
					}

					if (instruction->type == SKP) {
						if (arg0->condition == 0) {
							throw exception("condition is required for first argument.");
						}
					}

					instruction->arg0 = arg0;

					// next token is a comma (followed by a second argument)
					if (line.size() > 1) {
						if (line[0]->type == Lexer::TOKEN_TYPE::COMMA) {
							line.erase(line.begin()); // consume the comma
							bool negative = false;
							// parse second argument
							// next is optional, it can be a negative flag
							if (line[0]->type == Lexer::TOKEN_TYPE::MINUS) {
								negative = true;
								line.erase(line.begin()); // consume the negative
							}

							if (line.size() > 0) {
								Param* arg1 = new Param();
								double coefficient = 0;
								if (line[0]->type == Lexer::TOKEN_TYPE::NUMBER) {
									STR2NUMBER_ERROR err = str2dble(coefficient, line[0]->name.c_str());
									if (err == SUCCESS) {
										arg1->doubleValue = negative ? coefficient * -1.0 : coefficient;

										arg1->skipLines = unsigned int(arg0->doubleValue);
									}
								}
								else if (line[0]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {
									map<string, Param>::iterator it = equMap.find(line[0]->name);
									if (it != equMap.end()) {
										Param p = (*it).second;
										arg1->doubleValue = negative ? p.doubleValue * -1.0 : p.doubleValue;
									}

									map<string, unsigned int>::iterator it2 = labels.find(line[0]->name);
									if (it2 != labels.end()) {
										unsigned int labelAddress = (*it2).second;
										arg1->skipLines = labelAddress - currentInstruction;
									}
								}
								arg1->type = line[0]->type;
								arg1->value = line[0]->name;

								instruction->arg1 = arg1;
								return true;
							}
						}
					}
				}
				else if(line.size() > 0) {
					// this instruction has one argument, like mulx, ldax
					if (line[0]->type == Lexer::TOKEN_TYPE::IDENTIFIER) {

						Param* arg0 = new Param();
						arg0->type = line[0]->type;
						arg0->value = line[0]->name;
						arg0->dir = FV1::Start;
						arg0->regAddress = fv1->getAddressOfIdentifier(arg0->value);
						if (arg0->regAddress == NULL) {
							map<string, Param>::iterator it = equMap.find(arg0->value);
							if (it != equMap.end()) {
								Param param = (*it).second;
								arg0->regAddress = param.regAddress;
							}
						}

						if (instruction->type == MULX || instruction->type == LDAX) {
							if (arg0->regAddress == NULL) {
								throw exception("register address is required for first argument.");
							}
						}

						instruction->arg0 = arg0;
						instruction->arg1 = NULL;
						return true;
					}
				}
				else {
					// 0 arguments, it must be absa
					instruction->arg0 = NULL;
					instruction->arg1 = NULL;
					return true;
				}
			}
			else {
				throw exception("unknown instruction.");
			}
		}
	}
	else {
		throw exception("instruction too small.");
	}
	return false;
}

FV1::MemoryPosition DirectionSpecificationWithType(Lexer::TOKEN_TYPE& type) {
	switch (type) {
	case Lexer::TOKEN_TYPE::CIRCUMFLEX:
		return FV1::Middle;
	case Lexer::TOKEN_TYPE::NUMERAL:
		return FV1::End;
	default:
		return FV1::Start;
	}
}

InstructionType InstructionTypeWithString(string& instruction) {
	if (instruction.compare("rdax") == 0) {
		return RDAX;
	}
	else if (instruction.compare("rda") == 0) {
		return RDA;
	}
	else if (instruction.compare("wrap") == 0) {
		return WRAP;
	}
	else if (instruction.compare("wrax") == 0) {
		return WRAX;
	}
	else if (instruction.compare("wra") == 0) {
		return WRA;
	}
	else if (instruction.compare("mulx") == 0) {
		return MULX;
	}
	else if (instruction.compare("rdfx") == 0) {
		return RDFX;
	}
	else if (instruction.compare("log") == 0) {
		return LOG;
	}
	else if (instruction.compare("exp") == 0) {
		return EXP;
	}
	else if (instruction.compare("sof") == 0) {
		return SOF;
	}
	else if (instruction.compare("maxx") == 0) {
		return MAXX;
	}
	else if (instruction.compare("ldax") == 0) {
		return LDAX;
	}
	else if (instruction.compare("skp") == 0) {
		return SKP;
	}
	else if (instruction.compare("absa") == 0) {
		return ABSA;
	}
	else if (instruction.compare("wrlx") == 0) {
		return WRLX;
	}
	else if (instruction.compare("wrhx") == 0) {
		return WRHX;
	}

	else {
		return UNKNOWN;
	}
}
// separate statements that should be executed on first pass, and second
// this handles label declarations followed by code
ExecutionVectorResult generateExecutionVector(vector<vector<Lexer::Token*>> lines) {

	ExecutionVectorResult exec = ExecutionVectorResult();

	for (vector<vector<Lexer::Token*>>::iterator it = lines.begin(); it != lines.end(); it++) {
		vector<Lexer::Token*> v = (*it);
		SplitStatementInfo splitInfo = shouldSplitStatements(v);
		if (splitInfo.shouldSplit) {
			string label = splitInfo.labelDeclared;
			unsigned int absoluteCodeLine = exec.secondPass.size() - 1; // zero based line
			exec.labels[label] = absoluteCodeLine;

			if (splitInfo.secondPassStatement.size() > 0) {
				exec.secondPass.push_back(splitInfo.secondPassStatement);
			}
		}
		else {
			if (isFirstPassStatement(v)) {
				exec.firstPass.push_back(v);
			}
			else {
				exec.secondPass.push_back(v);
			}
		}
	}
	exec.success = true;
	return exec;
}

// splits instructions to be parsed in different stages
// label declarations should be executed in first pass,
// the same line can be followed by code.
SplitStatementInfo shouldSplitStatements(vector<Lexer::Token*> line) {
	SplitStatementInfo splitInfo = SplitStatementInfo();
	splitInfo.shouldSplit = false;

	if (line.size() > 1) {
		Lexer::TOKEN_TYPE type = line[0]->type;
		if (type == Lexer::TOKEN_TYPE::IDENTIFIER) {
			// if followed by (:) colon, is a label, should be added to first group
			if (line[1]->type == Lexer::TOKEN_TYPE::COLON) {
				splitInfo.shouldSplit = true;
				splitInfo.labelDeclared = line[0]->name;

				if (line.size() > 2) {
					for (unsigned int i = 2; i < line.size(); i++) {
						splitInfo.secondPassStatement.push_back(line[i]);
					}
				}
			}
		}
	}
	return splitInfo;
}
BOOL isFirstPassStatement(vector<Lexer::Token*> v) {
	if (v.size() == 3) {
		Lexer::TOKEN_TYPE type = v[0]->type;
		if (type == Lexer::TOKEN_TYPE::IDENTIFIER) {
			string id = v[0]->name;
			if (id.compare("mem") == 0) {
				return true;
			}
			else if (id.compare("equ") == 0) {
				return true;
			}
		}
	}

	return false;
}