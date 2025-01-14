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
 * typedefs
 ***********************************************************************************************************/

typedef struct
{
	lpExImParam		ExImContact;
	DB::CEnumList *pModules;
} EXPORTDATA, *LPEXPORTDATA;

/***********************************************************************************************************
 * modules stuff
 ***********************************************************************************************************/

 /**
  * name:	ExportTree_AppendModuleList
  * desc:	according to the checked list items create the module list for exporting
  * param:	hTree		- handle to the window of the treeview
  *			hParent		- parent tree item for the item to add
  *			pModules	- module list to fill
  * return:	nothing
  **/

void ExportTree_AppendModuleList(HWND hTree, HTREEITEM hParent, DB::CEnumList *pModules)
{
	TVITEMA tvi;

	// add all checked modules
	if (tvi.hItem = TreeView_GetChild(hTree, hParent)) {
		CHAR szModule[MAXSETTING];

		// add optional items
		tvi.mask = TVIF_STATE | TVIF_TEXT;
		tvi.stateMask = TVIS_STATEIMAGEMASK;
		tvi.pszText = szModule;
		tvi.cchTextMax = _countof(szModule);

		do {
			if (SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&tvi) &&
				(tvi.state == INDEXTOSTATEIMAGEMASK(0) || tvi.state == INDEXTOSTATEIMAGEMASK(2))) {
				pModules->Insert(tvi.pszText);
			}
		}
			while (tvi.hItem = TreeView_GetNextSibling(hTree, tvi.hItem));
	}
}

/**
 * name:	ExportTree_FindItem
 * desc:	find a item by its label
 * param:	hTree		- handle to the window of the treeview
 *			hParent		- parent tree item for the item to add
 *			pszText		- text to match the label against
 * return:	a handle to the found treeitem or NULL
 **/

HTREEITEM ExportTree_FindItem(HWND hTree, HTREEITEM hParent, LPSTR pszText)
{
	TVITEMA tvi;
	CHAR szBuf[128];

	if (!pszText || !*pszText) return nullptr;

	tvi.mask = TVIF_TEXT;
	tvi.pszText = szBuf;
	tvi.cchTextMax = _countof(szBuf);

	for (tvi.hItem = TreeView_GetChild(hTree, hParent);
		tvi.hItem != nullptr;
		tvi.hItem = TreeView_GetNextSibling(hTree, tvi.hItem)) {
		if (SendMessageA(hTree, TVM_GETITEMA, NULL, (LPARAM)&tvi) && !mir_strcmpi(tvi.pszText, pszText))
			return tvi.hItem;
	}
	return nullptr;
}

/**
 * name:	ExportTree_AddItem
 * desc:	add an item to the tree view with given options
 * param:	hTree		- handle to the window of the treeview
 *			hParent		- parent tree item for the item to add
 *			pszDesc		- item label
 *			bUseImages	- icons are loaded
 *			bState		- 0-hide checkbox/1-unchecked/2-checked
 * return:	return handle to added treeitem
 **/

HTREEITEM ExportTree_AddItem(HWND hTree, HTREEITEM hParent, LPSTR pszDesc, uint8_t bUseImages, uint8_t bState)
{
	TVINSERTSTRUCTA	tvii;
	HTREEITEM hItem = nullptr;

	tvii.hParent = hParent;
	tvii.hInsertAfter = TVI_SORT;
	tvii.itemex.mask = TVIF_TEXT;
	if (bUseImages) {
		tvii.itemex.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvii.itemex.iImage = tvii.itemex.iSelectedImage = 1;
	}
	tvii.itemex.pszText = pszDesc;
	if (hItem = (HTREEITEM)SendMessageA(hTree, TVM_INSERTITEMA, NULL, (LPARAM)&tvii))
		TreeView_SetItemState(hTree, hItem, INDEXTOSTATEIMAGEMASK(bState), TVIS_STATEIMAGEMASK);
	return hItem;
}

/**
 * name:	SelectModulesToExport_DlgProc
 * desc:	dialog procedure for a dialogbox, which lists modules for a specific contact
 * param:	hDlg		- handle to the window of the dialogbox
 *			uMsg		- message to handle
 *			wParam		- message specific parameter
 *			lParam		- message specific parameter
 * return:	message specific
 **/

INT_PTR CALLBACK SelectModulesToExport_DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LPEXPORTDATA pDat = (LPEXPORTDATA)GetUserData(hDlg);

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			uint8_t bImagesLoaded = 0;

			// get tree handle and set treeview style
			HWND hTree = GetDlgItem(hDlg, IDC_TREE);
			if (!hTree) break;
			SetWindowLongPtr(hTree, GWL_STYLE, GetWindowLongPtr(hTree, GWL_STYLE) | TVS_NOHSCROLL | TVS_CHECKBOXES);

			// init the datastructure
			if (!(pDat = (LPEXPORTDATA)mir_alloc(sizeof(EXPORTDATA))))
				return FALSE;
			pDat->ExImContact = ((LPEXPORTDATA)lParam)->ExImContact;
			pDat->pModules = ((LPEXPORTDATA)lParam)->pModules;
			SetUserData(hDlg, pDat);

			// set icons
			{
				const ICONCTRL idIcon[] = {
					{ IDI_EXPORT,    WM_SETICON,   NULL        },
					{ IDI_EXPORT,    STM_SETIMAGE, ICO_DLGLOGO },
					{ IDI_EXPORT,    BM_SETIMAGE,  IDOK        },
					{ IDI_BTN_CLOSE, BM_SETIMAGE,  IDCANCEL    }
				};
				const int numIconsToSet = g_plugin.getByte(SET_ICONS_BUTTONS, 1) ? _countof(idIcon) : 2;
				IcoLib_SetCtrlIcons(hDlg, idIcon, numIconsToSet);

				// create imagelist for treeview
				HIMAGELIST hImages = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), (IsWinVerVistaPlus() ? ILC_COLOR32 : ILC_COLOR16) | ILC_MASK, 0, 1);
				if (hImages != nullptr) {
					SendMessage(hTree, TVM_SETIMAGELIST, TVSIL_NORMAL, (LPARAM)hImages);
					g_plugin.addImgListIcon(hImages, IDI_LST_MODULES);
					g_plugin.addImgListIcon(hImages, IDI_LST_FOLDER);
					bImagesLoaded = true;
				}
			}
			// do the translation stuff
			{
				SendDlgItemMessage(hDlg, BTN_CHECK, BUTTONTRANSLATE, NULL, NULL);
				SendDlgItemMessage(hDlg, BTN_UNCHECK, BUTTONTRANSLATE, NULL, NULL);
				SendDlgItemMessage(hDlg, IDOK, BUTTONTRANSLATE, NULL, NULL);
				SendDlgItemMessage(hDlg, IDCANCEL, BUTTONTRANSLATE, NULL, NULL);
			}
			// Set the Window Title and description
			{
				LPCTSTR name = nullptr;
				wchar_t	oldTitle[MAXDATASIZE],
					newTitle[MAXDATASIZE];
				switch (pDat->ExImContact->Typ) {
				case EXIM_ALL:
				case EXIM_GROUP:
					name = TranslateT("All Contacts");
					break;
				case EXIM_CONTACT:
					if (pDat->ExImContact->hContact == NULL) {
						name = TranslateT("Owner");
					}
					else {
						name = Clist_GetContactDisplayName(pDat->ExImContact->hContact);
					}
					break;
				case EXIM_SUBGROUP:
					name = (LPCTSTR)pDat->ExImContact->ptszName;
					break;
				case EXIM_ACCOUNT:
					PROTOACCOUNT *acc = Proto_GetAccount(pDat->ExImContact->pszName);
					name = (LPCTSTR)acc->tszAccountName;
					break;
				}
				TranslateDialogDefault(hDlg);			//to translate oldTitle
				GetWindowText(hDlg, oldTitle, _countof(oldTitle));
				mir_snwprintf(newTitle, L"%s - %s", name, oldTitle);
				SetWindowText(hDlg, newTitle);
			}

			{
				LPSTR pszProto;
				TVINSERTSTRUCT	tviiT;
				DB::CEnumList Modules;
				HTREEITEM hItemEssential, hItemOptional;

				TreeView_SetIndent(hTree, 6);
				TreeView_SetItemHeight(hTree, 18);

				pszProto = (pDat->ExImContact->Typ == EXIM_CONTACT && pDat->ExImContact->hContact != NULL)
					? (LPSTR)Proto_GetBaseAccountName(pDat->ExImContact->hContact)
					: nullptr;

				// add items that are always exported
				tviiT.hParent = TVI_ROOT;
				tviiT.hInsertAfter = TVI_FIRST;
				tviiT.itemex.mask = TVIF_TEXT | TVIF_STATE;
				tviiT.itemex.pszText = TranslateT("Required modules");
				tviiT.itemex.state = tviiT.itemex.stateMask = TVIS_EXPANDED;
				if (bImagesLoaded) {
					tviiT.itemex.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
					tviiT.itemex.iImage = tviiT.itemex.iSelectedImage = 0;
				}
				if (hItemEssential = TreeView_InsertItem(hTree, &tviiT)) {
					// disable state images
					TreeView_SetItemState(hTree, hItemEssential, INDEXTOSTATEIMAGEMASK(0), TVIS_STATEIMAGEMASK);

					// insert essential items (modul from UIEX)
					ExportTree_AddItem(hTree, hItemEssential, USERINFO, bImagesLoaded, 0);

					/*Filter/ protocol module is ignored for owner contact
					if (pDat->hContact != NULL) {
						ExportTree_AddItem(hTree, hItemEssential, "Protocol", bImagesLoaded, 0);
					}*/

					// base protocol is only valid for single exported contact at this position
					if (pszProto) {
						ExportTree_AddItem(hTree, hItemEssential, pszProto, bImagesLoaded, 0);
					}
				}

				// add items that are optional (and more essential)
				tviiT.hInsertAfter = TVI_LAST;
				tviiT.itemex.pszText = TranslateT("Optional modules");
				if (hItemOptional = TreeView_InsertItem(hTree, &tviiT)) {
					TreeView_SetItemState(hTree, hItemOptional, INDEXTOSTATEIMAGEMASK(0), TVIS_STATEIMAGEMASK);

					if (!Modules.EnumModules())	// init Modul list
					{
						for (auto &p : Modules) {
							/*Filter/
							if (!DB::Module::IsMeta(p))/end Filter*/
							{
								// module must exist in at least one contact
								if (pDat->ExImContact->Typ != EXIM_CONTACT) // TRUE = All Contacts
								{
									for (auto &hContact: Contacts()) {
										// ignore empty modules
										if (!db_is_module_empty(hContact, p)) {
											pszProto = Proto_GetBaseAccountName(hContact);
											// Filter by mode
											switch (pDat->ExImContact->Typ) {
											case EXIM_ALL:
											case EXIM_GROUP:
												break;
											case EXIM_SUBGROUP:
												if (mir_wstrncmp(pDat->ExImContact->ptszName, DB::Setting::GetWString(hContact, "CList", "Group"), mir_wstrlen(pDat->ExImContact->ptszName))) {
													continue;
												}
												break;
											case EXIM_ACCOUNT:
												if (mir_strcmp(pDat->ExImContact->pszName, pszProto)) {
													continue;
												}
												break;
											}

											// contact's base protocol is to be added to the treeview uniquely
											if (!mir_strcmpi(p, pszProto)) {
												if (!ExportTree_FindItem(hTree, hItemEssential, p))
													ExportTree_AddItem(hTree, hItemEssential, p, bImagesLoaded, 0);
												break;
											}

											// add optional module, which is valid for at least one contact
											/*/Filter/*/
											if (mir_strcmpi(p, USERINFO) && mir_strcmpi(p, META_PROTO)) {
												ExportTree_AddItem(hTree, hItemOptional, p, bImagesLoaded, 1);
												break;
											}
										}
									} // end for
								} // end TRUE = All Contacts

								// module must exist in the selected contact
								else if (!db_is_module_empty(pDat->ExImContact->hContact, p) && (!pDat->ExImContact->hContact || mir_strcmpi(p, pszProto)) && mir_strcmpi(p, USERINFO)) {
									ExportTree_AddItem(hTree, hItemOptional, (LPSTR)p, bImagesLoaded, 1);
								}
							}
						}
					}
				}
			}
			TranslateDialogDefault(hDlg);
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (GetDlgItem(hDlg, STATIC_WHITERECT) == (HWND)lParam || GetDlgItem(hDlg, ICO_DLGLOGO) == (HWND)lParam) {
			SetBkColor((HDC)wParam, RGB(255, 255, 255));
			return (INT_PTR)GetStockObject(WHITE_BRUSH);
		}
		SetBkMode((HDC)wParam, TRANSPARENT);
		return (INT_PTR)GetStockObject(NULL_BRUSH);

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				HWND hTree = GetDlgItem(hDlg, IDC_TREE);
				HTREEITEM hParent;

				// search the tree item of optional items
				for (hParent = TreeView_GetRoot(hTree);
					hParent != nullptr;
					hParent = TreeView_GetNextSibling(hTree, hParent)) {
					ExportTree_AppendModuleList(hTree, hParent, pDat->pModules);
				}
				return EndDialog(hDlg, IDOK);
			}
		case IDCANCEL:
			return EndDialog(hDlg, IDCANCEL);

		case BTN_CHECK:
		case BTN_UNCHECK:
			{
				HWND hTree = GetDlgItem(hDlg, IDC_TREE);
				LPCSTR pszRoot = Translate("Optional modules");
				TVITEMA tvi;
				CHAR szText[128];

				tvi.mask = TVIF_TEXT;
				tvi.pszText = szText;
				tvi.cchTextMax = _countof(szText);

				// search the tree item of optional items
				for (tvi.hItem = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_ROOT, NULL);
					tvi.hItem != nullptr && SendMessageA(hTree, TVM_GETITEMA, 0, (LPARAM)&tvi);
					tvi.hItem = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)tvi.hItem)) {
					if (!mir_strcmpi(tvi.pszText, pszRoot)) {
						tvi.mask = TVIF_STATE;
						tvi.state = INDEXTOSTATEIMAGEMASK(LOWORD(wParam) == BTN_UNCHECK ? 1 : 2);
						tvi.stateMask = TVIS_STATEIMAGEMASK;

						for (tvi.hItem = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)tvi.hItem);
							tvi.hItem != nullptr;
							tvi.hItem = (HTREEITEM)SendMessageA(hTree, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)tvi.hItem)) {
							SendMessageA(hTree, TVM_SETITEMA, NULL, (LPARAM)&tvi);
						}
						break;
					}
				}
				break;
			}
		}
		break;

	case WM_DESTROY:
		mir_free(pDat);
		break;
	}
	return 0;
}

/**
 * name:	DlgExImModules_SelectModulesToExport
 * desc:	calls a dialog box that lists all modules for a specific contact
 * param:	ExImContact	- lpExImParam
 *			pModules	- pointer to an ENUMLIST structure that retrieves the resulting list of modules
 *			hParent		- handle to a window which should act as the parent of the created dialog
 * return:	0 if user pressed ok, 1 on cancel
 **/

int DlgExImModules_SelectModulesToExport(lpExImParam ExImContact, DB::CEnumList *pModules, HWND hParent)
{
	EXPORTDATA dat;
	dat.ExImContact = ExImContact;
	dat.pModules = pModules;
	return (IDOK != DialogBoxParam(g_plugin.getInst(), MAKEINTRESOURCE(IDD_EXPORT), hParent, SelectModulesToExport_DlgProc, (LPARAM)&dat));
}
