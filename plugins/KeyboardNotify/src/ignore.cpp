/*

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

#define IGNOREEVENT_MAX  4
#define IDI_SMALLDOT   211
#define IDI_FILLEDBLOB 212
#define IDI_EMPTYBLOB  213

static const uint32_t ignoreIdToPf1[IGNOREEVENT_MAX] = {PF1_IMRECV, 0xFFFFFFFF, PF1_FILERECV, 0xFFFFFFFF};

static uint32_t GetMask(MCONTACT hContact)
{
	uint32_t mask = g_plugin.getDword(hContact, "Mask1", (uint32_t)(-1));
	if(mask == (uint32_t)(-1)) {
		if(hContact == NULL)
			mask=0;
		else {
			if (Contact_IsHidden(hContact) || !Contact_OnList(hContact))
				mask = g_plugin.getDword("Mask1", 0);
			else
				mask = g_plugin.getDword("Default1", 0);
		}
	}
	return mask;
}

static void SetListGroupIcons(HWND hwndList, HANDLE hFirstItem, HANDLE hParentItem, int *groupChildCount)
{
	int typeOfFirst;
	int iconOn[IGNOREEVENT_MAX]={1,1,1,1};
	int childCount[IGNOREEVENT_MAX]={0,0,0,0}, i;
	int iImage;
	HANDLE hItem, hChildItem;

	typeOfFirst = SendMessage(hwndList, CLM_GETITEMTYPE, (WPARAM)hFirstItem, 0);
	//check groups
	if(typeOfFirst == CLCIT_GROUP)
		hItem = hFirstItem;
	else
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXTGROUP, (LPARAM)hFirstItem);
	while(hItem) {
		hChildItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_CHILD, (LPARAM)hItem);
		if(hChildItem)
			SetListGroupIcons(hwndList, hChildItem, hItem, childCount);
		for (i=0; i < _countof(iconOn); i++)
			if(iconOn[i] && SendMessage(hwndList, CLM_GETEXTRAIMAGE, (WPARAM)hItem, i) == 0)
				iconOn[i] = 0;
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXTGROUP, (LPARAM)hItem);
	}
	//check contacts
	if(typeOfFirst == CLCIT_CONTACT)
		hItem = hFirstItem;
	else
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXTCONTACT, (LPARAM)hFirstItem);
	while(hItem) {
		for (i=0; i < _countof(iconOn); i++) {
			iImage = SendMessage(hwndList, CLM_GETEXTRAIMAGE, (WPARAM)hItem, i);
			if(iconOn[i] && iImage == 0)
				iconOn[i] = 0;
			if(iImage != EMPTY_EXTRA_ICON)
				childCount[i]++;
		}
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXTCONTACT, (LPARAM)hItem);
	}
	//set icons
	for (i=0; i < _countof(iconOn); i++) {
		SendMessage(hwndList, CLM_SETEXTRAIMAGE, (WPARAM)hParentItem, MAKELPARAM(i, childCount[i]?(iconOn[i]?i+3:0):EMPTY_EXTRA_ICON));
		if(groupChildCount)
			groupChildCount[i] += childCount[i];
	}
	SendMessage(hwndList, CLM_SETEXTRAIMAGE, (WPARAM)hParentItem, MAKELPARAM(IGNOREEVENT_MAX, 1));
	SendMessage(hwndList, CLM_SETEXTRAIMAGE, (WPARAM)hParentItem, MAKELPARAM(IGNOREEVENT_MAX+1, 2));
}

static void SetAllChildIcons(HWND hwndList, HANDLE hFirstItem, int iColumn, int iImage)
{
	int typeOfFirst, iOldIcon;
	HANDLE hItem, hChildItem;

	typeOfFirst = SendMessage(hwndList,CLM_GETITEMTYPE,(WPARAM)hFirstItem, 0);
	//check groups
	if(typeOfFirst == CLCIT_GROUP)
		hItem = hFirstItem;
	else
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXTGROUP, (LPARAM)hFirstItem);
	while(hItem) {
		hChildItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_CHILD, (LPARAM)hItem);
		if(hChildItem)
			SetAllChildIcons(hwndList, hChildItem, iColumn, iImage);
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXTGROUP, (LPARAM)hItem);
	}
	//check contacts
	if(typeOfFirst == CLCIT_CONTACT)
		hItem = hFirstItem;
	else
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXTCONTACT, (LPARAM)hFirstItem);
	while(hItem) {
		iOldIcon = SendMessage(hwndList, CLM_GETEXTRAIMAGE, (WPARAM)hItem, iColumn);
		if(iOldIcon != EMPTY_EXTRA_ICON && iOldIcon != iImage)
			SendMessage(hwndList, CLM_SETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(iColumn, iImage));
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_NEXTCONTACT, (LPARAM)hItem);
	}
}

static void ResetListOptions(HWND hwndList)
{
	SendMessage(hwndList, CLM_SETHIDEEMPTYGROUPS, 1, 0);
}

static void SetIconsForColumn(HWND hwndList, HANDLE hItem, HANDLE hItemAll, int iColumn, int iImage)
{
	int itemType = SendMessage(hwndList, CLM_GETITEMTYPE, (WPARAM)hItem, 0);
	if(itemType == CLCIT_CONTACT) {
		int oldiImage = SendMessage(hwndList, CLM_GETEXTRAIMAGE, (WPARAM)hItem, iColumn);
		if (oldiImage != EMPTY_EXTRA_ICON && oldiImage != iImage)
			SendMessage(hwndList, CLM_SETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(iColumn, iImage));
	}
	else if(itemType == CLCIT_INFO) {
		if(hItem == hItemAll)
			SetAllChildIcons(hwndList, hItem, iColumn, iImage);
		else
			SendMessage(hwndList, CLM_SETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(iColumn, iImage)); //hItemUnknown
	}
	else if(itemType == CLCIT_GROUP) {
		hItem = (HANDLE)SendMessage(hwndList, CLM_GETNEXTITEM, CLGN_CHILD, (LPARAM)hItem);
		if(hItem)
			SetAllChildIcons(hwndList, hItem, iColumn, iImage);
	}
}

static void InitialiseItem(HWND hwndList, MCONTACT hContact, HANDLE hItem, uint32_t protoCaps)
{
	uint32_t mask = GetMask(hContact);
	for (int i=0; i < IGNOREEVENT_MAX; i++)
		if(ignoreIdToPf1[i] == 0xFFFFFFFF || protoCaps & ignoreIdToPf1[i])
			SendMessage(hwndList, CLM_SETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(i, mask&(1<<i)?i+3:0));
	SendMessage(hwndList, CLM_SETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(IGNOREEVENT_MAX, 1));
	SendMessage(hwndList, CLM_SETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(IGNOREEVENT_MAX+1, 2));
}

static void SaveItemMask(HWND hwndList, MCONTACT hContact, HANDLE hItem, const char *pszSetting)
{
	uint32_t mask=0;

	for (int i=0; i < IGNOREEVENT_MAX; i++) {
		int iImage = SendMessage(hwndList, CLM_GETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(i, 0));
		if(iImage && iImage != EMPTY_EXTRA_ICON)
			mask |= 1<<i;
	}
	g_plugin.setDword(hContact, pszSetting, mask);
}

static void SetAllContactIcons(HWND hwndList)
{
	uint32_t protoCaps;

	for (auto &hContact : Contacts()) {
		HANDLE hItem = (HANDLE)SendMessage(hwndList, CLM_FINDCONTACT, hContact, 0);
		if(hItem && SendMessage(hwndList, CLM_GETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(IGNOREEVENT_MAX, 0)) == EMPTY_EXTRA_ICON) {
			char *szProto = Proto_GetBaseAccountName(hContact);
			if(szProto == nullptr)
				protoCaps = 0;
			else
				protoCaps = CallProtoService(szProto, PS_GETCAPS,PFLAGNUM_1, 0);
			InitialiseItem(hwndList, hContact, hItem, protoCaps);
		}
	}
}

INT_PTR CALLBACK DlgProcIgnoreOptions(HWND hwndDlg, UINT msg, WPARAM, LPARAM lParam)
{
	static HICON hIcons[IGNOREEVENT_MAX+2];
	static HANDLE hItemAll, hItemUnknown;

	switch (msg)
	{
		case WM_INITDIALOG:
			TranslateDialogDefault(hwndDlg);
			{
				HIMAGELIST hIml=ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), ILC_COLOR32 | ILC_MASK, 3+IGNOREEVENT_MAX, 3+IGNOREEVENT_MAX);
				ImageList_AddSkinIcon(hIml, SKINICON_OTHER_SMALLDOT);
				ImageList_AddSkinIcon(hIml, SKINICON_OTHER_FILLEDBLOB);
				ImageList_AddSkinIcon(hIml, SKINICON_OTHER_EMPTYBLOB);
				ImageList_AddSkinIcon(hIml, SKINICON_EVENT_MESSAGE);
				ImageList_AddSkinIcon(hIml, SKINICON_EVENT_URL);
				ImageList_AddSkinIcon(hIml, SKINICON_EVENT_FILE);
				ImageList_AddSkinIcon(hIml, SKINICON_OTHER_MIRANDA);
				SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_SETEXTRAIMAGELIST, 0, (LPARAM)hIml);
				for (int i=0; i < _countof(hIcons); i++)
					hIcons[i] = ImageList_GetIcon(hIml, 1+i, ILD_NORMAL);
			}
			SendDlgItemMessage(hwndDlg, IDC_ALLICON, STM_SETICON, (WPARAM)hIcons[0], 0);
			SendDlgItemMessage(hwndDlg, IDC_NONEICON, STM_SETICON, (WPARAM)hIcons[1], 0);
			SendDlgItemMessage(hwndDlg, IDC_MSGICON, STM_SETICON, (WPARAM)hIcons[2], 0);
			SendDlgItemMessage(hwndDlg, IDC_FILEICON, STM_SETICON, (WPARAM)hIcons[4], 0);
			SendDlgItemMessage(hwndDlg, IDC_OTHERICON, STM_SETICON, (WPARAM)hIcons[5], 0);

			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_LIST), GWL_STYLE, GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_LIST), GWL_STYLE) &~ (CLS_CHECKBOXES|CLS_GROUPCHECKBOXES));
			SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_AUTOREBUILD, 0, 0);

			ResetListOptions(GetDlgItem(hwndDlg, IDC_LIST));
			SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_SETEXTRACOLUMNS, IGNOREEVENT_MAX+2, 0);

			{	CLCINFOITEM cii = {0};
				cii.cbSize = sizeof(cii);
				cii.flags = CLCIIF_GROUPFONT;
				cii.pszText = TranslateT("** All contacts **");
				hItemAll=(HANDLE)SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_ADDINFOITEM, 0, (LPARAM)&cii);

				cii.pszText = TranslateT("** Unknown contacts **");
				hItemUnknown=(HANDLE)SendDlgItemMessage(hwndDlg, IDC_LIST,CLM_ADDINFOITEM, 0, (LPARAM)&cii);
				InitialiseItem(GetDlgItem(hwndDlg, IDC_LIST), NULL, hItemUnknown, 0xFFFFFFFF);
			}

			SetAllContactIcons(GetDlgItem(hwndDlg, IDC_LIST));
			SetListGroupIcons(GetDlgItem(hwndDlg, IDC_LIST),(HANDLE)SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_GETNEXTITEM, CLGN_ROOT, 0), hItemAll, nullptr);
			return TRUE;
		case WM_SETFOCUS:
			SetFocus(GetDlgItem(hwndDlg, IDC_LIST));
			break;
		case WM_NOTIFY:
			switch(((LPNMHDR)lParam)->idFrom) {
				case IDC_LIST:
					switch (((LPNMHDR)lParam)->code)
					{
						case CLN_NEWCONTACT:
						case CLN_LISTREBUILT:
							SetAllContactIcons(GetDlgItem(hwndDlg, IDC_LIST));
							//fall through
						case CLN_CONTACTMOVED:
							SetListGroupIcons(GetDlgItem(hwndDlg, IDC_LIST), (HANDLE)SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_GETNEXTITEM, CLGN_ROOT, 0), hItemAll, nullptr);
							break;
						case CLN_OPTIONSCHANGED:
							ResetListOptions(GetDlgItem(hwndDlg, IDC_LIST));
							break;
						case CLN_CHECKCHANGED:
							SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
							break;
						case NM_CLICK:
						{
							int iImage;
							uint32_t hitFlags;
							NMCLISTCONTROL *nm = (NMCLISTCONTROL*)lParam;

							if(nm->iColumn == -1)
								break;
							HANDLE hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_HITTEST, (WPARAM)&hitFlags, MAKELPARAM(nm->pt.x, nm->pt.y));
							if(hItem == nullptr)
								break;
							if (!(hitFlags & CLCHT_ONITEMEXTRA))
								break;
							if(nm->iColumn == IGNOREEVENT_MAX) {   //ignore all
								for (iImage=0; iImage < IGNOREEVENT_MAX; iImage++)
									SetIconsForColumn(GetDlgItem(hwndDlg, IDC_LIST), hItem, hItemAll, iImage, iImage+3);
							}
							else if(nm->iColumn == IGNOREEVENT_MAX+1) {	//ignore none
								for (iImage=0; iImage < IGNOREEVENT_MAX; iImage++)
									SetIconsForColumn(GetDlgItem(hwndDlg, IDC_LIST), hItem, hItemAll, iImage, 0);
							}
							else {
								iImage = SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_GETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(nm->iColumn, 0));
								if(iImage == 0)
									iImage = nm->iColumn + 3;
								else if(iImage != EMPTY_EXTRA_ICON)
									iImage = 0;
								SetIconsForColumn(GetDlgItem(hwndDlg, IDC_LIST), hItem, hItemAll, nm->iColumn, iImage);
							}
							SetListGroupIcons(GetDlgItem(hwndDlg, IDC_LIST),(HANDLE)SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_GETNEXTITEM, CLGN_ROOT, 0), hItemAll, nullptr);
							SendMessage(GetParent(GetParent(hwndDlg)), PSM_CHANGED, 0, 0);
							break;
						}
					}
					break;
				case 0:
					switch (((LPNMHDR)lParam)->code)
					{
						case PSN_APPLY:
						{
							for (auto &hContact : Contacts()) {
								HANDLE hItem = (HANDLE)SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_FINDCONTACT, hContact, 0);
								if(hItem)
									SaveItemMask(GetDlgItem(hwndDlg, IDC_LIST), hContact, hItem, "Mask1");
							}
							SaveItemMask(GetDlgItem(hwndDlg, IDC_LIST), NULL, hItemAll, "Default1");
							SaveItemMask(GetDlgItem(hwndDlg, IDC_LIST), NULL, hItemUnknown, "Mask1");
							return TRUE;
						}
					}
					break;
			}
			break;
		case WM_DESTROY:
		{
			for (int i=0; i < _countof(hIcons); i++)
				DestroyIcon(hIcons[i]);
			HIMAGELIST hIml = (HIMAGELIST)SendDlgItemMessage(hwndDlg, IDC_LIST, CLM_GETEXTRAIMAGELIST, 0, 0);
			ImageList_Destroy(hIml);
			break;
		}
	}
	return FALSE;
}

BOOL IsIgnored(MCONTACT hContact, uint16_t eventType)
{
	uint16_t ignoreID = 0;
	uint32_t mask = GetMask(hContact);

	switch(eventType) {
		case EVENTTYPE_MESSAGE:
			ignoreID = 0;
			break;
		case EVENTTYPE_FILE:
			ignoreID = 2;
			break;
		default:
			ignoreID = 3;
	}

	return (mask>>ignoreID)&1;
}
