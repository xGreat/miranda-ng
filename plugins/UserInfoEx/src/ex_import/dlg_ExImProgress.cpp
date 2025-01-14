/*
UserinfoEx plugin for Miranda IM

Copyright:
© 2006-2010 DeathAxe, Yasnovidyashii, Merlin, K. Romanov, Kreol

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "../stdafx.h"

/***********************************************************************************************************
 * windows procedure
 ***********************************************************************************************************/

/**
 * name:	DlgProcProgress
 * desc:	dialog procedure for the progress dialog
 * params:	none
 * return:	nothing
 **/

static const ICONCTRL idIcon[] = {
	{ IDI_IMPORT,    WM_SETICON,   NULL        },
	{ IDI_IMPORT,    STM_SETIMAGE, ICO_DLGLOGO },
	{ IDI_BTN_CLOSE, BM_SETIMAGE,  IDCANCEL    }
};

INT_PTR CALLBACK DlgProcProgress(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		IcoLib_SetCtrlIcons(hDlg, idIcon, g_plugin.getByte(SET_ICONS_BUTTONS, 1) ? _countof(idIcon) : 2);

		TranslateDialogDefault(hDlg);
		SendDlgItemMessage(hDlg, IDCANCEL, BUTTONTRANSLATE, NULL, NULL);
		SendDlgItemMessage(hDlg, IDC_PROGRESS, PBM_SETPOS, 0, 0);
		SendDlgItemMessage(hDlg, IDC_PROGRESS2, PBM_SETPOS, 0, 0);
		SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
		UpdateWindow(hDlg);
		break;

	case WM_CTLCOLORSTATIC:
		switch (GetWindowLongPtr((HWND)lParam, GWLP_ID)) {
		case STATIC_WHITERECT:
		case TXT_SETTING:
		case IDC_PROGRESS:
		case TXT_CONTACT:
		case IDC_PROGRESS2:
			SetBkColor((HDC)wParam, RGB(255, 255, 255));
			return (INT_PTR)GetStockObject(WHITE_BRUSH);
		}
		return FALSE;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED) {
			switch (LOWORD(wParam)) {
			case IDCANCEL:
				// in the progress dialog, use the user data to indicate that the user has pressed cancel
				ShowWindow(hDlg, SW_HIDE);
				SetWindowLongPtr(hDlg, GWLP_USERDATA, 1);
				return TRUE;
			}
		}
		break;
	}
	return FALSE;
}

/**
 * name:	CProgress
 * class:	CProgress
 * desc:	create the progress dialog and return a handle as pointer to the datastructure
 * params:	none
 * return:	nothing
 **/

CProgress::CProgress()
{
	_dwStartTime = GetTickCount();
	_hDlg = CreateDialog(g_plugin.getInst(), MAKEINTRESOURCE(IDD_COPYPROGRESS), nullptr, DlgProcProgress);
}

/**
 * name:	~CProgress
 * class:	CProgress
 * desc:	destroy the progress dialog and its data structure
 * params:	none
 * return:	nothing
 **/

CProgress::~CProgress()
{
	if (IsWindow(_hDlg)) DestroyWindow(_hDlg);
}

/**
 * name:	SetContactCount
 * class:	CProgress
 * desc:	number of contacts to show 100% for
 * params:	numContacts	- the number of contacts
 * return:	nothing
 **/

void CProgress::SetContactCount(uint32_t numContacts)
{
	if (_hDlg) {
		HWND hProgress = GetDlgItem(_hDlg, IDC_PROGRESS2);
		SendMessage(hProgress, PBM_SETRANGE32, 0, numContacts);
		SendMessage(hProgress, PBM_SETPOS, 0, 0);
	}
}

/**
 * name:	SetSettingsCount
 * class:	CProgress
 * desc:	number of settings & events to show 100% for
 * params:	numSettings	- the number of settings & events
 * return:	nothing
 **/

void CProgress::SetSettingsCount(uint32_t numSettings)
{
	if (_hDlg) {
		HWND hProgress = GetDlgItem(_hDlg, IDC_PROGRESS);
		SendMessage(hProgress, PBM_SETRANGE32, 0, numSettings);
		SendMessage(hProgress, PBM_SETPOS, 0, 0);
	}
}

/**
 * name:	Hide
 * class:	CProgress
 * desc:	hides the dialog
 * params:	none
 * return:	nothing
 **/

void CProgress::Hide()
{
	ShowWindow(_hDlg, SW_HIDE);
}

/**
 * name:	Update
 * class:	CProgress
 * desc:	update the progress dialog
 * params:	nothing
 * return:	FALSE if user pressed cancel, TRUE otherwise
 **/

uint8_t CProgress::Update()
{
	MSG msg;

	// show dialog after one second
	if (GetTickCount() > _dwStartTime + 1000) {
		ShowWindow(_hDlg, SW_SHOW);
	}

	UpdateWindow(_hDlg);

	while (PeekMessage(&msg, _hDlg, 0, 0, PM_REMOVE) != 0) {
		if (_hDlg == nullptr || !IsDialogMessage(_hDlg, &msg)) { /* Wine fix. */
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return GetWindowLongPtr(_hDlg, GWLP_USERDATA) == 0;
}

/**
 * name:	UpdateContact
 * class:	CProgress
 * desc:	increase contact's progressbar by one and set new text
 * params:	pszFormat	- the text to display for the contact
 * return:	FALSE if user pressed cancel, TRUE otherwise
 **/

uint8_t CProgress::UpdateContact(LPCTSTR pszFormat, ...)
{
	if (_hDlg != nullptr) {
		HWND hProg = GetDlgItem(_hDlg, IDC_PROGRESS2);
		if (pszFormat) {
			wchar_t buf[MAX_PATH];
			va_list vl;

			va_start(vl, pszFormat);
			mir_vsnwprintf(buf, _countof(buf), TranslateW(pszFormat), vl);
			va_end(vl);
			SetDlgItemText(_hDlg, TXT_CONTACT, buf);
		}
		SendMessage(hProg, PBM_SETPOS, (int)SendMessage(hProg, PBM_GETPOS, 0, 0) + 1, 0);
		return Update();
	}
	return TRUE;
}

/**
 * name:	UpdateContact
 * class:	CProgress
 * desc:	increase setting's progressbar by one and set new text
 * params:	pszFormat	- the text to display for the setting
 * return:	FALSE if user pressed cancel, TRUE otherwise
 **/

uint8_t CProgress::UpdateSetting(LPCTSTR pszFormat, ...)
{
	if (_hDlg != nullptr) {
		HWND hProg = GetDlgItem(_hDlg, IDC_PROGRESS);
		if (pszFormat) {
			wchar_t buf[MAX_PATH];
			wchar_t tmp[MAX_PATH];
			va_list vl;

			va_start(vl, pszFormat);
			mir_vsnwprintf(buf, _countof(buf), TranslateW(pszFormat), vl);
			va_end(vl);
			GetDlgItemText(_hDlg, TXT_SETTING, tmp, _countof(tmp));
			if (mir_wstrcmpi(tmp, buf))
				SetDlgItemText(_hDlg, TXT_SETTING, buf);
		}
		SendMessage(hProg, PBM_SETPOS, (int)SendMessage(hProg, PBM_GETPOS, 0, 0) + 1, 0);
		return Update();
	}
	return TRUE;
}
