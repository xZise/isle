#include "isleapp.h"

#include "3dmanager/lego3dmanager.h"
#include "decomp.h"
#include "legoanimationmanager.h"
#include "legobuildingmanager.h"
#include "legogamestate.h"
#include "legoinputmanager.h"
#include "legomain.h"
#include "legomodelpresenter.h"
#include "legopartpresenter.h"
#include "legoutils.h"
#include "legovideomanager.h"
#include "legoworldpresenter.h"
#include "misc.h"
#include "mxbackgroundaudiomanager.h"
#include "mxdirectx/mxdirect3d.h"
#include "mxdsaction.h"
#include "mxmisc.h"
#include "mxomnicreateflags.h"
#include "mxomnicreateparam.h"
#include "mxstreamer.h"
#include "mxticklemanager.h"
#include "mxtimer.h"
#include "mxtransitionmanager.h"
#include "mxvariabletable.h"
#include "res/resource.h"
#include "roi/legoroi.h"
#include "viewmanager/viewmanager.h"

#include <dsound.h>

DECOMP_SIZE_ASSERT(IsleApp, 0x8c)

// GLOBAL: ISLE 0x410030
IsleApp* g_isle = NULL;

// GLOBAL: ISLE 0x410034
unsigned char g_mousedown = 0;

// GLOBAL: ISLE 0x410038
unsigned char g_mousemoved = 0;

// GLOBAL: ISLE 0x41003c
BOOL g_closed = FALSE;

// GLOBAL: ISLE 0x410040
RECT g_windowRect = {0, 0, 640, 480};

// GLOBAL: ISLE 0x410050
BOOL g_rmDisabled = FALSE;

// GLOBAL: ISLE 0x410054
BOOL g_waitingForTargetDepth = TRUE;

// GLOBAL: ISLE 0x410058
int g_targetWidth = 640;

// GLOBAL: ISLE 0x41005c
int g_targetHeight = 480;

// GLOBAL: ISLE 0x410060
int g_targetDepth = 16;

// GLOBAL: ISLE 0x410064
BOOL g_reqEnableRMDevice = FALSE;

// STRING: ISLE 0x4101c4
#define WNDCLASS_NAME "Lego Island MainNoM App"

// STRING: ISLE 0x4101dc
#define WINDOW_TITLE "LEGO\xAE"

// Might be static functions of IsleApp
BOOL FindExistingInstance();
BOOL StartDirectSound();

// FUNCTION: ISLE 0x401000
IsleApp::IsleApp()
{
	m_hdPath = NULL;
	m_cdPath = NULL;
	m_deviceId = NULL;
	m_savePath = NULL;
	m_fullScreen = TRUE;
	m_flipSurfaces = FALSE;
	m_backBuffersInVram = TRUE;
	m_using8bit = FALSE;
	m_using16bit = TRUE;
	m_hasLightSupport = FALSE;
	m_drawCursor = FALSE;
	m_use3dSound = TRUE;
	m_useMusic = TRUE;
	m_useJoystick = FALSE;
	m_joystickIndex = 0;
	m_wideViewAngle = TRUE;
	m_islandQuality = 1;
	m_islandTexture = 1;
	m_gameStarted = FALSE;
	m_frameDelta = 10;
	m_windowActive = TRUE;

#ifdef COMPAT_MODE
	{
		MxRect32 r(0, 0, 639, 479);
		MxVideoParamFlags flags;
		m_videoParam = MxVideoParam(r, NULL, 1, flags);
	}
#else
	m_videoParam = MxVideoParam(MxRect32(0, 0, 639, 479), NULL, 1, MxVideoParamFlags());
#endif
	m_videoParam.Flags().Set16Bit(MxDirectDraw::GetPrimaryBitDepth() == 16);

	m_windowHandle = NULL;
	m_cursorArrow = NULL;
	m_cursorBusy = NULL;
	m_cursorNo = NULL;
	m_cursorCurrent = NULL;

	LegoOmni::CreateInstance();
}

// FUNCTION: ISLE 0x4011a0
IsleApp::~IsleApp()
{
	if (LegoOmni::GetInstance()) {
		Close();
		MxOmni::DestroyInstance();
	}

	if (m_hdPath) {
		delete[] m_hdPath;
	}

	if (m_cdPath) {
		delete[] m_cdPath;
	}

	if (m_deviceId) {
		delete[] m_deviceId;
	}

	if (m_savePath) {
		delete[] m_savePath;
	}
}

// FUNCTION: ISLE 0x401260
void IsleApp::Close()
{
	MxDSAction ds;
	ds.SetUnknown24(-2);

	if (Lego()) {
		GameState()->Save(0);
		if (InputManager()) {
			InputManager()->QueueEvent(c_notificationKeyPress, 0, 0, 0, VK_SPACE);
		}

		VideoManager()->Get3DManager()->GetLego3DView()->GetViewManager()->RemoveAll(NULL);

		Lego()->RemoveWorld(ds.GetAtomId(), ds.GetObjectId());
		Lego()->DeleteObject(ds);
		TransitionManager()->SetWaitIndicator(NULL);
		Lego()->Resume();

		while (Streamer()->Close(NULL) == SUCCESS) {
		}

		while (Lego() && !Lego()->DoesEntityExist(ds)) {
			Timer()->GetRealTime();
			TickleManager()->Tickle();
		}
	}
}

// FUNCTION: ISLE 0x4013b0
BOOL IsleApp::SetupLegoOmni()
{
	BOOL result = FALSE;
	char mediaPath[256];
	GetProfileString("LEGO Island", "MediaPath", "", mediaPath, sizeof(mediaPath));

#ifdef COMPAT_MODE
	BOOL failure;
	{
		MxOmniCreateParam param(mediaPath, (struct HWND__*) m_windowHandle, m_videoParam, MxOmniCreateFlags());
		failure = Lego()->Create(param) == FAILURE;
	}
#else
	BOOL failure =
		Lego()->Create(MxOmniCreateParam(mediaPath, (struct HWND__*) m_windowHandle, m_videoParam, MxOmniCreateFlags())
		) == FAILURE;
#endif

	if (!failure) {
		VariableTable()->SetVariable("ACTOR_01", "");
		TickleManager()->SetClientTickleInterval(VideoManager(), 10);
		result = TRUE;
	}

	return result;
}

// FUNCTION: ISLE 0x401560
void IsleApp::SetupVideoFlags(
	BOOL fullScreen,
	BOOL flipSurfaces,
	BOOL backBuffers,
	BOOL using8bit,
	BOOL using16bit,
	BOOL hasLightSupport,
	BOOL param_7,
	BOOL wideViewAngle,
	char* deviceId
)
{
	m_videoParam.Flags().SetFullScreen(fullScreen);
	m_videoParam.Flags().SetFlipSurfaces(flipSurfaces);
	m_videoParam.Flags().SetBackBuffers(!backBuffers);
	m_videoParam.Flags().SetLacksLightSupport(!hasLightSupport);
	m_videoParam.Flags().SetF1bit7(param_7);
	m_videoParam.Flags().SetWideViewAngle(wideViewAngle);
	m_videoParam.Flags().SetEnabled(TRUE);
	m_videoParam.SetDeviceName(deviceId);
	if (using8bit) {
		m_videoParam.Flags().Set16Bit(0);
	}
	if (using16bit) {
		m_videoParam.Flags().Set16Bit(1);
	}
}

// FUNCTION: ISLE 0x401610
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	// Look for another instance, if we find one, bring it to the foreground instead
	if (!FindExistingInstance()) {
		return 0;
	}

	// Attempt to create DirectSound instance
	BOOL soundReady = FALSE;
	for (int i = 0; i < 20; i++) {
		if (StartDirectSound()) {
			soundReady = TRUE;
			break;
		}
		Sleep(500);
	}

	// Throw error if sound unavailable
	if (!soundReady) {
		MessageBox(
			NULL,
			"\"LEGO\xAE Island\" is not detecting a DirectSound compatible sound card.  Please quit all other "
			"applications and try again.",
			"Lego Island Error",
			MB_OK
		);
		return 0;
	}

	// Create global app instance
	g_isle = new IsleApp();

	// Create window
	if (g_isle->SetupWindow(hInstance, lpCmdLine) != SUCCESS) {
		MessageBox(
			NULL,
			"\"LEGO\xAE Island\" failed to start.  Please quit all other applications and try again.",
			"LEGO\xAE Island Error",
			MB_OK
		);
		return 0;
	}

	// Get reference to window
	HWND window;
	if (g_isle->GetWindowHandle()) {
		window = g_isle->GetWindowHandle();
	}

	// Load accelerators (this call actually achieves nothing - there is no "AppAccel" resource in the original - but
	// we'll keep this for authenticity) This line may actually be here because it's in DFVIEW, an example project that
	// ships with MSVC420, and was such a clean example of a Win32 app, that it was later adapted into an "ExeSkeleton"
	// sample for MSVC600. It's quite possible Mindscape derived this app from that example since they no longer had the
	// luxury of the MFC AppWizard which we know they used for the frontend used during development (ISLEMFC.EXE,
	// MAIN.EXE, et al.)
	LoadAccelerators(hInstance, "AppAccel");

	MSG msg;

	while (!g_closed) {
		while (!PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
			if (g_isle) {
				g_isle->Tick(1);
			}
		}

		if (g_isle) {
			g_isle->Tick(0);
		}

		while (!g_closed) {
			if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				break;
			}

			MSG nextMsg;
			if (!g_isle || !g_isle->GetWindowHandle() || msg.message != WM_MOUSEMOVE ||
				!PeekMessage(&nextMsg, NULL, 0, 0, PM_NOREMOVE) || nextMsg.message != WM_MOUSEMOVE) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			if (g_reqEnableRMDevice) {
				g_reqEnableRMDevice = FALSE;
				VideoManager()->EnableRMDevice();
				g_rmDisabled = FALSE;
				Lego()->Resume();
			}

			if (g_closed) {
				break;
			}

			if (g_mousedown && g_mousemoved && g_isle) {
				g_isle->Tick(0);
			}

			if (g_mousemoved) {
				g_mousemoved = FALSE;
			}
		}
	}

	DestroyWindow(window);

	return msg.wParam;
}

// FUNCTION: ISLE 0x401ca0
BOOL FindExistingInstance()
{
	HWND hWnd = FindWindow(WNDCLASS_NAME, WINDOW_TITLE);
	if (hWnd) {
		if (SetForegroundWindow(hWnd)) {
			ShowWindow(hWnd, SW_RESTORE);
		}
		return 0;
	}
	return 1;
}

// FUNCTION: ISLE 0x401ce0
BOOL StartDirectSound()
{
	LPDIRECTSOUND lpDS = NULL;
	HRESULT ret = DirectSoundCreate(NULL, &lpDS, NULL);
	if (ret == DS_OK && lpDS != NULL) {
		lpDS->Release();
		return TRUE;
	}

	return FALSE;
}

// FUNCTION: ISLE 0x401d20
LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	NotificationId type;
	unsigned char keyCode = 0;

	if (!g_isle) {
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
	case WM_PAINT:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	case WM_ACTIVATE:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	case WM_ACTIVATEAPP:
		if (g_isle) {
			if ((wParam != 0) && (g_isle->GetFullScreen())) {
				MoveWindow(
					hWnd,
					g_windowRect.left,
					g_windowRect.top,
					(g_windowRect.right - g_windowRect.left) + 1,
					(g_windowRect.bottom - g_windowRect.top) + 1,
					TRUE
				);
			}
			g_isle->SetWindowActive(wParam);
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	case WM_CLOSE:
		if (!g_closed && g_isle) {
			delete g_isle;
			g_isle = NULL;
			g_closed = TRUE;
			return 0;
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	case WM_GETMINMAXINFO:
		((MINMAXINFO*) lParam)->ptMaxTrackSize.x = (g_windowRect.right - g_windowRect.left) + 1;
		((MINMAXINFO*) lParam)->ptMaxTrackSize.y = (g_windowRect.bottom - g_windowRect.top) + 1;
		((MINMAXINFO*) lParam)->ptMinTrackSize.x = (g_windowRect.right - g_windowRect.left) + 1;
		((MINMAXINFO*) lParam)->ptMinTrackSize.y = (g_windowRect.bottom - g_windowRect.top) + 1;
		return 0;
	case WM_ENTERMENULOOP:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	case WM_SYSCOMMAND:
		if (wParam == SC_SCREENSAVE) {
			return 0;
		}
		if (wParam == SC_CLOSE && g_closed == FALSE) {
			if (g_isle) {
				if (g_rmDisabled) {
					ShowWindow(g_isle->GetWindowHandle(), SW_RESTORE);
				}
				PostMessage(g_isle->GetWindowHandle(), WM_CLOSE, 0, 0);
				return 0;
			}
		}
		else if (g_isle && g_isle->GetFullScreen() && (wParam == SC_MOVE || wParam == SC_KEYMENU)) {
			return 0;
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	case WM_EXITMENULOOP:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	case WM_MOVING:
		if (g_isle && g_isle->GetFullScreen()) {
			GetWindowRect(hWnd, (LPRECT) lParam);
			return 0;
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	case WM_NCPAINT:
		if (g_isle && g_isle->GetFullScreen()) {
			return 0;
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	case WM_DISPLAYCHANGE:
		if (g_isle && VideoManager() && g_isle->GetFullScreen() && VideoManager()->GetDirect3D()) {
			if (VideoManager()->GetDirect3D()->AssignedDevice()) {
				int targetDepth = wParam;
				int targetWidth = LOWORD(lParam);
				int targetHeight = HIWORD(lParam);

				if (g_waitingForTargetDepth) {
					g_waitingForTargetDepth = FALSE;
					g_targetDepth = targetDepth;
				}
				else {
					BOOL valid = FALSE;

					if (g_targetWidth == targetWidth && g_targetHeight == targetHeight &&
						g_targetDepth == targetDepth) {
						valid = TRUE;
					}

					if (g_rmDisabled) {
						if (valid) {
							g_reqEnableRMDevice = TRUE;
						}
					}
					else if (!valid) {
						g_rmDisabled = TRUE;
						Lego()->Pause();
						VideoManager()->DisableRMDevice();
					}
				}
			}
		}
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	case WM_KEYDOWN:
		// While this probably should be (HIWORD(lParam) & KF_REPEAT), this seems
		// to be what the assembly is actually doing
		if (lParam & (KF_REPEAT << 16)) {
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
		type = c_notificationKeyPress;
		keyCode = wParam;
		break;
	case WM_MOUSEMOVE:
		g_mousemoved = 1;
		type = c_notificationMouseMove;
		break;
	case WM_TIMER:
		type = c_notificationTimer;
		break;
	case WM_LBUTTONDOWN:
		g_mousedown = 1;
		type = c_notificationButtonDown;
		break;
	case WM_LBUTTONUP:
		g_mousedown = 0;
		type = c_notificationButtonUp;
		break;
	case WM_ISLE_SETCURSOR:
		if (g_isle) {
			g_isle->SetupCursor(wParam);
			return 0;
		}
		break;
	case WM_SETCURSOR:
		if (g_isle && (g_isle->GetCursorCurrent() == g_isle->GetCursorBusy() ||
					   g_isle->GetCursorCurrent() == g_isle->GetCursorNo() || !g_isle->GetCursorCurrent())) {
			SetCursor(g_isle->GetCursorCurrent());
			return 0;
		}
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	if (g_isle) {
		if (InputManager()) {
			InputManager()->QueueEvent(type, wParam, LOWORD(lParam), HIWORD(lParam), keyCode);
		}
		if (g_isle && g_isle->GetDrawCursor() && type == c_notificationMouseMove) {
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
			if (x >= 640) {
				x = 639;
			}
			if (y >= 480) {
				y = 479;
			}
			VideoManager()->MoveCursor(x, y);
		}
	}

	return 0;
}

// FUNCTION: ISLE 0x4023e0
MxResult IsleApp::SetupWindow(HINSTANCE hInstance, LPSTR lpCmdLine)
{
	WNDCLASSA wndclass;
	ZeroMemory(&wndclass, sizeof(WNDCLASSA));

	LoadConfig();

	SetupVideoFlags(
		m_fullScreen,
		m_flipSurfaces,
		m_backBuffersInVram,
		m_using8bit,
		m_using16bit,
		m_hasLightSupport,
		FALSE,
		m_wideViewAngle,
		m_deviceId
	);

	MxOmni::SetSound3D(m_use3dSound);

	srand(timeGetTime() / 1000);
	SystemParametersInfo(SPI_SETMOUSETRAILS, 0, NULL, 0);

	ZeroMemory(&wndclass, sizeof(WNDCLASSA));

	wndclass.cbClsExtra = 0;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbWndExtra = 0;
	wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(APP_ICON));
	wndclass.hCursor = m_cursorArrow = m_cursorCurrent = LoadCursor(hInstance, MAKEINTRESOURCE(ISLE_ARROW));
	m_cursorBusy = LoadCursor(hInstance, MAKEINTRESOURCE(ISLE_BUSY));
	m_cursorNo = LoadCursor(hInstance, MAKEINTRESOURCE(ISLE_NO));
	wndclass.hInstance = hInstance;
	wndclass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wndclass.lpszClassName = WNDCLASS_NAME;

	if (!RegisterClass(&wndclass)) {
		return FAILURE;
	}

	if (m_fullScreen) {
		AdjustWindowRectEx(&g_windowRect, WS_CAPTION | WS_SYSMENU, 0, WS_EX_APPWINDOW);

		m_windowHandle = CreateWindowEx(
			WS_EX_APPWINDOW,
			WNDCLASS_NAME,
			WINDOW_TITLE,
			WS_CAPTION | WS_SYSMENU,
			g_windowRect.left,
			g_windowRect.top,
			g_windowRect.right - g_windowRect.left + 1,
			g_windowRect.bottom - g_windowRect.top + 1,
			NULL,
			NULL,
			hInstance,
			NULL
		);
	}
	else {
		AdjustWindowRectEx(&g_windowRect, WS_CAPTION | WS_SYSMENU, 0, WS_EX_APPWINDOW);

		m_windowHandle = CreateWindowEx(
			WS_EX_APPWINDOW,
			WNDCLASS_NAME,
			WINDOW_TITLE,
			WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			g_windowRect.right - g_windowRect.left + 1,
			g_windowRect.bottom - g_windowRect.top + 1,
			NULL,
			NULL,
			hInstance,
			NULL
		);
	}

	if (!m_windowHandle) {
		return FAILURE;
	}

	if (m_fullScreen) {
		MoveWindow(
			m_windowHandle,
			g_windowRect.left,
			g_windowRect.top,
			(g_windowRect.right - g_windowRect.left) + 1,
			(g_windowRect.bottom - g_windowRect.top) + 1,
			TRUE
		);
	}

	ShowWindow(m_windowHandle, SW_SHOWNORMAL);
	UpdateWindow(m_windowHandle);
	if (!SetupLegoOmni()) {
		return FAILURE;
	}

	GameState()->SetSavePath(m_savePath);
	GameState()->SerializePlayersInfo(LegoStorage::c_read);
	GameState()->SerializeScoreHistory(LegoStorage::c_read);

	int iVar10;
	switch (m_islandQuality) {
	case 0:
		iVar10 = 1;
		break;
	case 1:
		iVar10 = 2;
		break;
	default:
		iVar10 = 100;
	}

	int uVar1 = (m_islandTexture == 0);
	LegoModelPresenter::configureLegoModelPresenter(uVar1);
	LegoPartPresenter::configureLegoPartPresenter(uVar1, iVar10);
	LegoWorldPresenter::configureLegoWorldPresenter(m_islandQuality);
	LegoBuildingManager::configureLegoBuildingManager(m_islandQuality);
	LegoROI::configureLegoROI(iVar10);
	LegoAnimationManager::configureLegoAnimationManager(m_islandQuality);
	if (LegoOmni::GetInstance()) {
		if (LegoOmni::GetInstance()->GetInputManager()) {
			LegoOmni::GetInstance()->GetInputManager()->SetUseJoystick(m_useJoystick);
			LegoOmni::GetInstance()->GetInputManager()->SetJoystickIndex(m_joystickIndex);
		}
	}
	if (m_fullScreen) {
		MoveWindow(
			m_windowHandle,
			g_windowRect.left,
			g_windowRect.top,
			(g_windowRect.right - g_windowRect.left) + 1,
			(g_windowRect.bottom - g_windowRect.top) + 1,
			TRUE
		);
	}
	ShowWindow(m_windowHandle, SW_SHOWNORMAL);
	UpdateWindow(m_windowHandle);

	return SUCCESS;
}

// FUNCTION: ISLE 0x402740
BOOL IsleApp::ReadReg(LPCSTR name, LPSTR outValue, DWORD outSize)
{
	HKEY hKey;
	DWORD valueType;

	BOOL out = FALSE;
	DWORD size = outSize;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\Mindscape\\LEGO Island", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		if (RegQueryValueEx(hKey, name, NULL, &valueType, (LPBYTE) outValue, &size) == ERROR_SUCCESS) {
			if (RegCloseKey(hKey) == ERROR_SUCCESS) {
				out = TRUE;
			}
		}
	}

	return out;
}

// FUNCTION: ISLE 0x4027b0
BOOL IsleApp::ReadRegBool(LPCSTR name, BOOL* out)
{
	char buffer[256];

	BOOL read = ReadReg(name, buffer, sizeof(buffer));
	if (read) {
		if (strcmp("YES", buffer) == 0) {
			*out = TRUE;
			return read;
		}

		if (strcmp("NO", buffer) == 0) {
			*out = FALSE;
			return read;
		}

		read = FALSE;
	}
	return read;
}

// FUNCTION: ISLE 0x402880
BOOL IsleApp::ReadRegInt(LPCSTR name, int* out)
{
	char buffer[256];

	BOOL read = ReadReg(name, buffer, sizeof(buffer));
	if (read) {
		*out = atoi(buffer);
	}

	return read;
}

// FUNCTION: ISLE 0x4028d0
void IsleApp::LoadConfig()
{
	char buffer[1024];

	if (!ReadReg("diskpath", buffer, sizeof(buffer))) {
		strcpy(buffer, MxOmni::GetHD());
	}

	m_hdPath = new char[strlen(buffer) + 1];
	strcpy(m_hdPath, buffer);
	MxOmni::SetHD(m_hdPath);

	if (!ReadReg("cdpath", buffer, sizeof(buffer))) {
		strcpy(buffer, MxOmni::GetCD());
	}

	m_cdPath = new char[strlen(buffer) + 1];
	strcpy(m_cdPath, buffer);
	MxOmni::SetCD(m_cdPath);

	ReadRegBool("Flip Surfaces", &m_flipSurfaces);
	ReadRegBool("Full Screen", &m_fullScreen);
	ReadRegBool("Wide View Angle", &m_wideViewAngle);
	ReadRegBool("3DSound", &m_use3dSound);
	ReadRegBool("Music", &m_useMusic);
	ReadRegBool("UseJoystick", &m_useJoystick);
	ReadRegInt("JoystickIndex", &m_joystickIndex);
	ReadRegBool("Draw Cursor", &m_drawCursor);

	int backBuffersInVRAM;
	if (ReadRegBool("Back Buffers in Video RAM", &backBuffersInVRAM)) {
		m_backBuffersInVram = !backBuffersInVRAM;
	}

	int bitDepth;
	if (ReadRegInt("Display Bit Depth", &bitDepth)) {
		if (bitDepth == 8) {
			m_using8bit = TRUE;
		}
		else if (bitDepth == 16) {
			m_using16bit = TRUE;
		}
	}

	if (!ReadReg("Island Quality", buffer, sizeof(buffer))) {
		strcpy(buffer, "1");
	}
	m_islandQuality = atoi(buffer);

	if (!ReadReg("Island Texture", buffer, sizeof(buffer))) {
		strcpy(buffer, "1");
	}
	m_islandTexture = atoi(buffer);

	if (ReadReg("3D Device ID", buffer, sizeof(buffer))) {
		m_deviceId = new char[strlen(buffer) + 1];
		strcpy(m_deviceId, buffer);
	}

	if (ReadReg("savepath", buffer, sizeof(buffer))) {
		m_savePath = new char[strlen(buffer) + 1];
		strcpy(m_savePath, buffer);
	}
}

// FUNCTION: ISLE 0x402c20
inline void IsleApp::Tick(BOOL sleepIfNotNextFrame)
{
	// GLOBAL: ISLE 0x4101c0
	static MxLong g_lastFrameTime = 0;

	// GLOBAL: ISLE 0x4101bc
	static int g_startupDelay = 200;

	if (!m_windowActive) {
		Sleep(0);
		return;
	}

	if (!Lego()) {
		return;
	}
	if (!TickleManager()) {
		return;
	}
	if (!Timer()) {
		return;
	}

	MxLong currentTime = Timer()->GetRealTime();
	if (currentTime < g_lastFrameTime) {
		g_lastFrameTime = -m_frameDelta;
	}

	if (m_frameDelta + g_lastFrameTime < currentTime) {
		if (!Lego()->IsPaused()) {
			TickleManager()->Tickle();
		}
		g_lastFrameTime = currentTime;

		if (g_startupDelay == 0) {
			return;
		}

		g_startupDelay--;
		if (g_startupDelay != 0) {
			return;
		}

		LegoOmni::GetInstance()->CreateBackgroundAudio();
		BackgroundAudioManager()->Enable(this->m_useMusic);

		MxStreamController* stream = Streamer()->Open("\\lego\\scripts\\isle\\isle", MxStreamer::e_diskStream);
		MxDSAction ds;

		if (!stream) {
			stream = Streamer()->Open("\\lego\\scripts\\nocd", MxStreamer::e_diskStream);
			if (!stream) {
				return;
			}

			ds.SetAtomId(stream->GetAtom());
			ds.SetUnknown24(-1);
			ds.SetObjectId(0);
			VideoManager()->EnableFullScreenMovie(TRUE, TRUE);

			if (Start(&ds) != SUCCESS) {
				return;
			}
		}
		else {
			ds.SetAtomId(stream->GetAtom());
			ds.SetUnknown24(-1);
			ds.SetObjectId(0);
			if (Start(&ds) != SUCCESS) {
				return;
			}
			m_gameStarted = TRUE;
		}
	}
	else if (sleepIfNotNextFrame != 0) {
		Sleep(0);
	}
}

// FUNCTION: ISLE 0x402e80
void IsleApp::SetupCursor(WPARAM wParam)
{
	switch (wParam) {
	case e_cursorArrow:
		m_cursorCurrent = m_cursorArrow;
		break;
	case e_cursorBusy:
		m_cursorCurrent = m_cursorBusy;
		break;
	case e_cursorNo:
		m_cursorCurrent = m_cursorNo;
		break;
	case e_cursorNone:
		m_cursorCurrent = NULL;
	case e_cursorUnused3:
	case e_cursorUnused4:
	case e_cursorUnused5:
	case e_cursorUnused6:
	case e_cursorUnused7:
	case e_cursorUnused8:
	case e_cursorUnused9:
	case e_cursorUnused10:
		break;
	}

	SetCursor(m_cursorCurrent);
}
