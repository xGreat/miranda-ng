/*

Jabber Protocol Plugin for Miranda NG

Copyright (c) 2002-04  Santithorn Bunchua
Copyright (c) 2005-12  George Hazan
Copyright (c) 2007     Maxim Mluhov
Copyright (c) 2007     Victor Pavlychko
Copyright (C) 2012-22 Miranda NG team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "stdafx.h"

#ifndef SES_EXTENDBACKCOLOR
#define	SES_EXTENDBACKCOLOR 4
#endif

/* increment buffer with 1K steps */
#define STRINGBUF_INCREMENT		1024

struct StringBuf
{
	char *buf;
	size_t size, offset, streamOffset;
};

static void sttAppendBufRaw(StringBuf *buf, const char *str);
static void sttAppendBufW(StringBuf *buf, const wchar_t *str);
#define sttAppendBufT(a,b)		(sttAppendBufW((a),(b)))
static void sttEmptyBuf(StringBuf *buf);

#define RTF_HEADER	\
			"{\\rtf1\\ansi{\\colortbl;"	\
				"\\red128\\green0\\blue0;"	\
				"\\red0\\green0\\blue128;"	\
				"\\red245\\green255\\blue245;"	\
				"\\red245\\green245\\blue255;"	\
				"\\red128\\green128\\blue128;"	\
				"\\red255\\green235\\blue235;"	\
			"}"
#define RTF_FOOTER			"}"
#define RTF_BEGINTAG		"\\pard "
#define RTF_INDENT_FMT		"\\fi-100\\li%d "
#define RTF_ENDTAG			"\\par"
#define RTF_BEGINTAGNAME	"\\cf1\\b "
#define RTF_ENDTAGNAME		"\\cf0\\b0 "
#define RTF_BEGINATTRNAME	"\\cf2\\b "
#define RTF_ENDATTRNAME		"\\cf0\\b0 "
#define RTF_BEGINATTRVAL	"\\b0 "
#define RTF_ENDATTRVAL		""
#define RTF_BEGINTEXT		"\\pard "
#define RTF_TEXTINDENT_FMT	"\\fi0\\li%d "
#define RTF_ENDTEXT			"\\par"
#define RTF_BEGINPLAINXML	"\\pard\\fi0\\li100\\highlight6\\cf0 "
#define RTF_ENDPLAINXML		"\\par"
#define RTF_SEPARATOR		"\\sl-1\\slmult0\\highlight5\\cf5\\-\\par\\sl0"

static void sttRtfAppendXml(StringBuf *buf, const TiXmlElement *node, uint32_t flags, int indent);

void CJabberProto::OnConsoleProcessXml(const TiXmlElement *node, uint32_t flags)
{
	if (node && m_pDlgConsole) {
		if (node->Name()) {
			if (FilterXml(node, flags)) {
				StringBuf buf = {};
				sttAppendBufRaw(&buf, RTF_HEADER);
				sttRtfAppendXml(&buf, node, flags, 1);
				sttAppendBufRaw(&buf, RTF_SEPARATOR);
				sttAppendBufRaw(&buf, RTF_FOOTER);
				m_pDlgConsole->OnProtoRefresh(0, (LPARAM)&buf);
				sttEmptyBuf(&buf);
			}
		}
		else {
			for (auto *it : TiXmlEnum(node))
				OnConsoleProcessXml(it, flags);
		}
	}
}

bool CJabberProto::RecursiveCheckFilter(const TiXmlElement *node, uint32_t flags)
{
	for (auto *p = node->FirstAttribute(); p; p = p->Next())
		if (JabberStrIStr(Utf2T(p->Value()), m_filterInfo.pattern))
			return true;

	for (auto *it : TiXmlEnum(node))
		if (RecursiveCheckFilter(it, flags))
			return true;

	return false;
}

bool CJabberProto::FilterXml(const TiXmlElement *node, uint32_t flags)
{
	if (!m_filterInfo.msg && !mir_strcmp(node->Name(), "message")) return false;
	if (!m_filterInfo.presence && !mir_strcmp(node->Name(), "presence")) return false;
	if (!m_filterInfo.iq && !mir_strcmp(node->Name(), "iq")) return false;
	if (m_filterInfo.type == TFilterInfo::T_OFF) return true;

	mir_cslock lck(m_filterInfo.csPatternLock);

	const char *attrValue;
	switch (m_filterInfo.type) {
	case TFilterInfo::T_JID:
		attrValue = XmlGetAttr(node, (flags & JCPF_OUT) ? "to" : "from");
		if (attrValue)
			return JabberStrIStr(Utf2T(attrValue), m_filterInfo.pattern) != nullptr;
		break;

	case TFilterInfo::T_XMLNS:
		attrValue = XmlGetAttr(XmlFirstChild(node), "xmlns");
		if (attrValue)
			return JabberStrIStr(Utf2T(attrValue), m_filterInfo.pattern) != nullptr;
		break;

	case TFilterInfo::T_ANY:
		return RecursiveCheckFilter(node, flags);
	}

	return false;
}

static void sttAppendBufRaw(StringBuf *buf, const char *str)
{
	if (!str) return;

	size_t length = mir_strlen(str);
	if (buf->size - buf->offset < length + 1) {
		buf->size += (length + STRINGBUF_INCREMENT);
		buf->buf = (char *)mir_realloc(buf->buf, buf->size);
	}
	mir_strcpy(buf->buf + buf->offset, str);
	buf->offset += length;
}

static void sttAppendBufW(StringBuf *buf, const wchar_t *str)
{
	char tmp[32];

	if (!str) return;

	sttAppendBufRaw(buf, "{\\uc1 ");
	for (const wchar_t *p = str; *p; ++p) {
		if ((*p == '\\') || (*p == '{') || (*p == '}')) {
			tmp[0] = '\\';
			tmp[1] = (char)*p;
			tmp[2] = 0;
		}
		else if (*p < 128) {
			tmp[0] = (char)*p;
			tmp[1] = 0;
		}
		else mir_snprintf(tmp, "\\u%d ?", (int)*p);

		sttAppendBufRaw(buf, tmp);
	}
	sttAppendBufRaw(buf, "}");
}

static void sttEmptyBuf(StringBuf *buf)
{
	if (buf->buf) mir_free(buf->buf);
	buf->buf = nullptr;
	buf->size = 0;
	buf->offset = 0;
}

static void sttRtfAppendXml(StringBuf *buf, const TiXmlElement *node, uint32_t flags, int indent)
{
	char indentLevel[128];
	mir_snprintf(indentLevel, RTF_INDENT_FMT, (int)(indent * 200));

	sttAppendBufRaw(buf, RTF_BEGINTAG);
	sttAppendBufRaw(buf, indentLevel);
	if (flags & JCPF_IN)
		sttAppendBufRaw(buf, "\\highlight3 ");
	if (flags & JCPF_OUT)
		sttAppendBufRaw(buf, "\\highlight4 ");
	sttAppendBufRaw(buf, "<");
	sttAppendBufRaw(buf, RTF_BEGINTAGNAME);
	sttAppendBufRaw(buf, node->Name());
	sttAppendBufRaw(buf, RTF_ENDTAGNAME);

	for (auto *p = node->FirstAttribute(); p; p = p->Next()) {
		sttAppendBufRaw(buf, " ");
		sttAppendBufRaw(buf, RTF_BEGINATTRNAME);
		sttAppendBufW(buf, Utf2T(p->Name()));
		sttAppendBufRaw(buf, RTF_ENDATTRNAME);
		sttAppendBufRaw(buf, "=\"");
		sttAppendBufRaw(buf, RTF_BEGINATTRVAL);
		sttAppendBufT(buf, Utf2T(p->Value()));
		sttAppendBufRaw(buf, "\"");
		sttAppendBufRaw(buf, RTF_ENDATTRVAL);
	}

	if (!node->NoChildren() || node->GetText()) {
		sttAppendBufRaw(buf, ">");
		if (!node->NoChildren())
			sttAppendBufRaw(buf, RTF_ENDTAG);
	}

	if (node->GetText()) {
		if (!node->NoChildren()) {
			sttAppendBufRaw(buf, RTF_BEGINTEXT);
			char indentTextLevel[128];
			mir_snprintf(indentTextLevel, RTF_TEXTINDENT_FMT, (int)((indent + 1) * 200));
			sttAppendBufRaw(buf, indentTextLevel);
		}

		sttAppendBufT(buf, Utf2T(node->GetText()));
		if (!node->NoChildren())
			sttAppendBufRaw(buf, RTF_ENDTEXT);
	}

	for (auto *it : TiXmlEnum(node))
		sttRtfAppendXml(buf, it, flags & ~(JCPF_IN | JCPF_OUT), indent + 1);

	if (!node->NoChildren() || node->GetText()) {
		sttAppendBufRaw(buf, RTF_BEGINTAG);
		sttAppendBufRaw(buf, indentLevel);
		sttAppendBufRaw(buf, "</");
		sttAppendBufRaw(buf, RTF_BEGINTAGNAME);
		sttAppendBufRaw(buf, node->Name());
		sttAppendBufRaw(buf, RTF_ENDTAGNAME);
		sttAppendBufRaw(buf, ">");
	}
	else sttAppendBufRaw(buf, " />");

	sttAppendBufRaw(buf, RTF_ENDTAG);
}

DWORD CALLBACK sttStreamInCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	StringBuf *buf = (StringBuf *)dwCookie;

	if (buf->streamOffset < buf->offset) {
		size_t cbLen = min(size_t(cb), buf->offset - buf->streamOffset);
		memcpy(pbBuff, buf->buf + buf->streamOffset, cbLen);
		buf->streamOffset += cbLen;
		*pcb = (LONG)cbLen;
	}
	else *pcb = 0;

	return 0;
}

static void sttJabberConsoleRebuildStrings(CJabberProto *ppro, HWND hwndCombo)
{
	JABBER_LIST_ITEM *item = nullptr;

	int len = GetWindowTextLength(hwndCombo) + 1;
	wchar_t *buf = (wchar_t *)_alloca(len * sizeof(wchar_t));
	GetWindowText(hwndCombo, buf, len);

	SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);

	for (int i = 0; i < g_cJabberFeatCapPairs; i++)
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)g_JabberFeatCapPairs[i].szFeature);
	for (int i = 0; i < g_cJabberFeatCapPairsExt; i++)
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)g_JabberFeatCapPairsExt[i].szFeature);

	LISTFOREACH(i, ppro, LIST_ROSTER)
		if (item = ppro->ListGetItemPtrFromIndex(i))
			SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)item->jid);
	LISTFOREACH(i, ppro, LIST_CHATROOM)
		if (item = ppro->ListGetItemPtrFromIndex(i))
			SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)item->jid);

	SetWindowText(hwndCombo, buf);
}

///////////////////////////////////////////////////////////////////////////////
// CJabberDlgConsole class

struct
{
	int type;
	wchar_t *title;
	int icon;
}
static filter_modes[] =
{
	{ TFilterInfo::T_JID,   L"JID",            IDI_JABBER },
	{ TFilterInfo::T_XMLNS, L"xmlns",          IDI_CONSOLE },
	{ TFilterInfo::T_ANY,   L"all attributes", IDI_FILTER_APPLY },
	{ TFilterInfo::T_OFF,   L"disabled",       IDI_FILTER_RESET },
};

class CJabberDlgConsole : public CJabberDlgBase
{
	typedef CJabberDlgBase CSuper;

public:
	CJabberDlgConsole(CJabberProto *proto):
		CJabberDlgBase(proto, IDD_CONSOLE)
	{
		SetMinSize(300, 400);
	}

	bool OnInitDialog() override
	{
		CSuper::OnInitDialog();

		Window_SetIcon_IcoLib(m_hwnd, g_plugin.getIconHandle(IDI_CONSOLE));
		SendDlgItemMessage(m_hwnd, IDC_CONSOLE, EM_SETEDITSTYLE, SES_EXTENDBACKCOLOR, SES_EXTENDBACKCOLOR);
		SendDlgItemMessage(m_hwnd, IDC_CONSOLE, EM_EXLIMITTEXT, 0, 0x80000000);

		m_proto->m_filterInfo.msg = m_proto->getByte("consoleWnd_msg", TRUE);
		m_proto->m_filterInfo.presence = m_proto->getByte("consoleWnd_presence", TRUE);
		m_proto->m_filterInfo.iq = m_proto->getByte("consoleWnd_iq", TRUE);
		m_proto->m_filterInfo.type = (TFilterInfo::Type)m_proto->getByte("consoleWnd_ftype", TFilterInfo::T_OFF);

		*m_proto->m_filterInfo.pattern = 0;
		ptrW tszPattern(m_proto->getWStringA("consoleWnd_fpattern"));
		if (tszPattern != nullptr)
			mir_wstrncpy(m_proto->m_filterInfo.pattern, tszPattern, _countof(m_proto->m_filterInfo.pattern));

		sttJabberConsoleRebuildStrings(m_proto, GetDlgItem(m_hwnd, IDC_CB_FILTER));
		SetDlgItemText(m_hwnd, IDC_CB_FILTER, m_proto->m_filterInfo.pattern);

		struct
		{
			int idc;
			char *title;
			int icon;
			bool push;
			BOOL pushed;
		}
		static buttons[] =
		{
			{ IDC_BTN_MSG,            "Messages",     IDI_PL_MSG_ALLOW,   true,  m_proto->m_filterInfo.msg },
			{ IDC_BTN_PRESENCE,       "Presences",    IDI_PL_PRIN_ALLOW,  true,  m_proto->m_filterInfo.presence },
			{ IDC_BTN_IQ,             "Queries",      IDI_PL_QUERY_ALLOW, true,  m_proto->m_filterInfo.iq },
			{ IDC_BTN_FILTER,         "Filter mode",  IDI_FILTER_APPLY,   true,  false },
			{ IDC_BTN_FILTER_REFRESH, "Refresh list", IDI_NAV_REFRESH,    false, false },
		};

		for (auto &it : buttons) {
			SendDlgItemMessage(m_hwnd, it.idc, BM_SETIMAGE, IMAGE_ICON, (LPARAM)g_plugin.getIcon(it.icon));
			SendDlgItemMessage(m_hwnd, it.idc, BUTTONSETASFLATBTN, TRUE, 0);
			SendDlgItemMessage(m_hwnd, it.idc, BUTTONADDTOOLTIP, (WPARAM)it.title, 0);
			if (it.push)
				SendDlgItemMessage(m_hwnd, it.idc, BUTTONSETASPUSHBTN, TRUE, 0);
			if (it.pushed)
				CheckDlgButton(m_hwnd, it.idc, BST_CHECKED);
		}

		for (auto &it : filter_modes)
			if (it.type == m_proto->m_filterInfo.type) {
				IcoLib_ReleaseIcon((HICON)SendDlgItemMessage(m_hwnd, IDC_BTN_FILTER, BM_SETIMAGE, IMAGE_ICON, (LPARAM)g_plugin.getIcon(it.icon)));
				SendDlgItemMessage(m_hwnd, IDC_BTN_FILTER, BM_SETIMAGE, IMAGE_ICON, (LPARAM)g_plugin.getIcon(it.icon));
				break;
			}

		EnableWindow(GetDlgItem(m_hwnd, IDC_CB_FILTER), (m_proto->m_filterInfo.type == TFilterInfo::T_OFF) ? FALSE : TRUE);
		EnableWindow(GetDlgItem(m_hwnd, IDC_BTN_FILTER_REFRESH), (m_proto->m_filterInfo.type == TFilterInfo::T_OFF) ? FALSE : TRUE);

		Utils_RestoreWindowPosition(m_hwnd, 0, m_proto->m_szModuleName, "consoleWnd_");
		return true;
	}

	bool OnApply() override
	{
		m_proto->setByte("consoleWnd_msg", m_proto->m_filterInfo.msg);
		m_proto->setByte("consoleWnd_presence", m_proto->m_filterInfo.presence);
		m_proto->setByte("consoleWnd_iq", m_proto->m_filterInfo.iq);
		m_proto->setByte("consoleWnd_ftype", m_proto->m_filterInfo.type);
		m_proto->setWString("consoleWnd_fpattern", m_proto->m_filterInfo.pattern);
		return true;
	}

	void OnDestroy() override
	{
		Utils_SaveWindowPosition(m_hwnd, 0, m_proto->m_szModuleName, "consoleWnd_");

		IcoLib_ReleaseIcon((HICON)SendDlgItemMessage(m_hwnd, IDC_BTN_MSG, BM_SETIMAGE, IMAGE_ICON, 0));
		IcoLib_ReleaseIcon((HICON)SendDlgItemMessage(m_hwnd, IDC_BTN_PRESENCE, BM_SETIMAGE, IMAGE_ICON, 0));
		IcoLib_ReleaseIcon((HICON)SendDlgItemMessage(m_hwnd, IDC_BTN_IQ, BM_SETIMAGE, IMAGE_ICON, 0));
		IcoLib_ReleaseIcon((HICON)SendDlgItemMessage(m_hwnd, IDC_BTN_FILTER, BM_SETIMAGE, IMAGE_ICON, 0));
		IcoLib_ReleaseIcon((HICON)SendDlgItemMessage(m_hwnd, IDC_BTN_FILTER_REFRESH, BM_SETIMAGE, IMAGE_ICON, 0));

		m_proto->m_pDlgConsole = nullptr;
		CSuper::OnDestroy();

		PostThreadMessage(m_proto->m_dwConsoleThreadId, WM_QUIT, 0, 0);
	}

	int Resizer(UTILRESIZECONTROL *urc) override
	{
		switch (urc->wId) {
		case IDC_CONSOLE:
			return RD_ANCHORX_WIDTH | RD_ANCHORY_HEIGHT;
		case IDC_CONSOLEIN:
			return RD_ANCHORX_WIDTH | RD_ANCHORY_BOTTOM;

		case IDC_BTN_MSG:
		case IDC_BTN_PRESENCE:
		case IDC_BTN_IQ:
		case IDC_BTN_FILTER:
			return RD_ANCHORX_LEFT | RD_ANCHORY_BOTTOM;

		case IDC_RESET:
		case IDOK:
		case IDC_BTN_FILTER_REFRESH:
			return RD_ANCHORX_RIGHT | RD_ANCHORY_BOTTOM;

		case IDC_CB_FILTER:
			RECT rc;
			GetWindowRect(GetDlgItem(m_hwnd, urc->wId), &rc);
			urc->rcItem.right += (urc->dlgNewSize.cx - urc->dlgOriginalSize.cx);
			urc->rcItem.top += (urc->dlgNewSize.cy - urc->dlgOriginalSize.cy);
			urc->rcItem.bottom = urc->rcItem.top + rc.bottom - rc.top;
			return 0;
		}
		return CSuper::Resizer(urc);
	}

	INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override
	{
		switch (msg) {
		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDOK:
				if (!m_proto->m_bJabberOnline)
					MessageBox(m_hwnd, TranslateT("Can't send data while you are offline."), TranslateT("Jabber Error"), MB_ICONSTOP | MB_OK);
				else {
					int length = GetWindowTextLength(GetDlgItem(m_hwnd, IDC_CONSOLEIN)) + 1;
					wchar_t *textToSend = (wchar_t *)mir_alloc(length * sizeof(wchar_t));
					GetDlgItemText(m_hwnd, IDC_CONSOLEIN, textToSend, length);

					TiXmlDocument doc;
					if (0 == doc.Parse(T2Utf(textToSend)))
						m_proto->m_ThreadInfo->send(doc.RootElement());
					else {
						StringBuf buf = {};
						sttAppendBufRaw(&buf, RTF_HEADER);
						sttAppendBufRaw(&buf, RTF_BEGINPLAINXML);
						sttAppendBufT(&buf, TranslateT("Outgoing XML parsing error"));
						sttAppendBufRaw(&buf, RTF_ENDPLAINXML);
						sttAppendBufRaw(&buf, RTF_SEPARATOR);
						sttAppendBufRaw(&buf, RTF_FOOTER);
						OnProtoRefresh(0, (LPARAM)&buf);
						sttEmptyBuf(&buf);
					}

					mir_free(textToSend);

					SetDlgItemText(m_hwnd, IDC_CONSOLEIN, L"");
				}
				return TRUE;

			case IDC_RESET:
				SetDlgItemText(m_hwnd, IDC_CONSOLE, L"");
				break;

			case IDC_BTN_MSG:
			case IDC_BTN_PRESENCE:
			case IDC_BTN_IQ:
				m_proto->m_filterInfo.msg = IsDlgButtonChecked(m_hwnd, IDC_BTN_MSG);
				m_proto->m_filterInfo.presence = IsDlgButtonChecked(m_hwnd, IDC_BTN_PRESENCE);
				m_proto->m_filterInfo.iq = IsDlgButtonChecked(m_hwnd, IDC_BTN_IQ);
				break;

			case IDC_BTN_FILTER_REFRESH:
				sttJabberConsoleRebuildStrings(m_proto, GetDlgItem(m_hwnd, IDC_CB_FILTER));
				break;

			case IDC_CB_FILTER:
				if (HIWORD(wParam) == CBN_SELCHANGE) {
					int idx = SendDlgItemMessage(m_hwnd, IDC_CB_FILTER, CB_GETCURSEL, 0, 0);
					int len = SendDlgItemMessage(m_hwnd, IDC_CB_FILTER, CB_GETLBTEXTLEN, idx, 0) + 1;

					mir_cslock lck(m_proto->m_filterInfo.csPatternLock);

					if (len > _countof(m_proto->m_filterInfo.pattern)) {
						wchar_t *buf = (wchar_t *)_alloca(len * sizeof(wchar_t));
						SendDlgItemMessage(m_hwnd, IDC_CB_FILTER, CB_GETLBTEXT, idx, (LPARAM)buf);
						mir_wstrncpy(m_proto->m_filterInfo.pattern, buf, _countof(m_proto->m_filterInfo.pattern));
					}
					else SendDlgItemMessage(m_hwnd, IDC_CB_FILTER, CB_GETLBTEXT, idx, (LPARAM)m_proto->m_filterInfo.pattern);
				}
				else if (HIWORD(wParam) == CBN_EDITCHANGE) {
					mir_cslock lck(m_proto->m_filterInfo.csPatternLock);
					GetDlgItemText(m_hwnd, IDC_CB_FILTER, m_proto->m_filterInfo.pattern, _countof(m_proto->m_filterInfo.pattern));
				}
				break;

			case IDC_BTN_FILTER:
				HMENU hMenu = CreatePopupMenu();
				for (auto &it : filter_modes)
					AppendMenu(hMenu, MF_STRING | ((it.type == m_proto->m_filterInfo.type) ? MF_CHECKED : 0), it.type + 1, TranslateW(it.title));

				RECT rc; GetWindowRect(GetDlgItem(m_hwnd, IDC_BTN_FILTER), &rc);
				CheckDlgButton(m_hwnd, IDC_BTN_FILTER, BST_CHECKED);
				int res = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_BOTTOMALIGN, rc.left, rc.top, 0, m_hwnd, nullptr);
				CheckDlgButton(m_hwnd, IDC_BTN_FILTER, BST_UNCHECKED);
				DestroyMenu(hMenu);

				if (res) {
					m_proto->m_filterInfo.type = (TFilterInfo::Type)(res - 1);
					for (auto &it : filter_modes) {
						if (it.type == m_proto->m_filterInfo.type) {
							IcoLib_ReleaseIcon((HICON)SendDlgItemMessage(m_hwnd, IDC_BTN_FILTER, BM_SETIMAGE, IMAGE_ICON, (LPARAM)g_plugin.getIcon(it.icon)));
							break;
						}
					}
					EnableWindow(GetDlgItem(m_hwnd, IDC_CB_FILTER), (m_proto->m_filterInfo.type == TFilterInfo::T_OFF) ? FALSE : TRUE);
					EnableWindow(GetDlgItem(m_hwnd, IDC_BTN_FILTER_REFRESH), (m_proto->m_filterInfo.type == TFilterInfo::T_OFF) ? FALSE : TRUE);
				}
			}
			break;
		}

		return CSuper::DlgProc(msg, wParam, lParam);
	}

	void OnProtoRefresh(WPARAM, LPARAM lParam) override
	{
		SendDlgItemMessage(m_hwnd, IDC_CONSOLE, WM_SETREDRAW, FALSE, 0);

		StringBuf *buf = (StringBuf *)lParam;
		buf->streamOffset = 0;

		EDITSTREAM es = { 0 };
		es.dwCookie = (DWORD_PTR)buf;
		es.pfnCallback = sttStreamInCallback;

		SCROLLINFO si = { 0 };
		si.cbSize = sizeof(si);
		si.fMask = SIF_ALL;
		GetScrollInfo(GetDlgItem(m_hwnd, IDC_CONSOLE), SB_VERT, &si);

		CHARRANGE oldSel, sel;
		POINT ptScroll;
		SendDlgItemMessage(m_hwnd, IDC_CONSOLE, EM_GETSCROLLPOS, 0, (LPARAM)&ptScroll);
		SendDlgItemMessage(m_hwnd, IDC_CONSOLE, EM_EXGETSEL, 0, (LPARAM)&oldSel);
		sel.cpMin = sel.cpMax = GetWindowTextLength(GetDlgItem(m_hwnd, IDC_CONSOLE));
		SendDlgItemMessage(m_hwnd, IDC_CONSOLE, EM_EXSETSEL, 0, (LPARAM)&sel);
		SendDlgItemMessage(m_hwnd, IDC_CONSOLE, EM_STREAMIN, SF_RTF | SFF_SELECTION, (LPARAM)&es);
		SendDlgItemMessage(m_hwnd, IDC_CONSOLE, EM_EXSETSEL, 0, (LPARAM)&oldSel);

		// magic expression from tabsrmm :)
		if ((UINT)si.nPos >= (UINT)si.nMax - si.nPage - 5 || si.nMax - si.nMin - si.nPage < 50) {
			SendDlgItemMessage(m_hwnd, IDC_CONSOLE, WM_VSCROLL, MAKEWPARAM(SB_BOTTOM, 0), 0);
			sel.cpMin = sel.cpMax = GetWindowTextLength(GetDlgItem(m_hwnd, IDC_CONSOLE));
			SendDlgItemMessage(m_hwnd, IDC_CONSOLE, EM_EXSETSEL, 0, (LPARAM)&sel);
		}
		else SendDlgItemMessage(m_hwnd, IDC_CONSOLE, EM_SETSCROLLPOS, 0, (LPARAM)&ptScroll);

		SendDlgItemMessage(m_hwnd, IDC_CONSOLE, WM_SETREDRAW, TRUE, 0);
		InvalidateRect(GetDlgItem(m_hwnd, IDC_CONSOLE), nullptr, FALSE);
	}
};

void __cdecl CJabberProto::ConsoleThread(void*)
{
	Thread_SetName("Jabber: ConsoleThread");
	MThreadLock threadLock(m_hThreadConsole);
	m_dwConsoleThreadId = ::GetCurrentThreadId();

	m_pDlgConsole = new CJabberDlgConsole(this);
	m_pDlgConsole->Show();

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	m_dwConsoleThreadId = 0;
}

void CJabberProto::ConsoleUninit()
{
	if (m_hThreadConsole) {
		PostThreadMessage(m_dwConsoleThreadId, WM_QUIT, 0, 0);
		if (MsgWaitForMultipleObjects(1, &m_hThreadConsole, FALSE, 5000, TRUE) == WAIT_TIMEOUT)
			TerminateThread(m_hThreadConsole, 0);

		if (m_hThreadConsole) {
			CloseHandle(m_hThreadConsole);
			m_hThreadConsole = nullptr;
		}
	}

	m_filterInfo.iq = m_filterInfo.msg = m_filterInfo.presence = false;
	m_filterInfo.type = TFilterInfo::T_OFF;
}

INT_PTR __cdecl CJabberProto::OnMenuHandleConsole(WPARAM, LPARAM)
{
	if (m_pDlgConsole)
		SetForegroundWindow(m_pDlgConsole->GetHwnd());
	else
		m_hThreadConsole = ForkThreadEx(&CJabberProto::ConsoleThread, 0, &m_dwConsoleThreadId);

	return 0;
}
