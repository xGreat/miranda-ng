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

/**
 * System Includes:
 **/
#include "stdafx.h"

/**
 * name:	MAnnivDate
 * class:	MAnnivDate
 * desc:	default constructor
 * param:	none
 * return:	nothing
 **/
MAnnivDate::MAnnivDate()
{
	Clear();
}

/**
 * name:	MAnnivDate
 * class:	MAnnivDate
 * desc:	constructor, which duplicates an existing anniversary date
 * param:	mda	- anniversary date to copy
 * return:	nothing
 **/
MAnnivDate::MAnnivDate(MAnnivDate &mda)
{
	SetDate(mda);
}

/**
 * name:	Clear
 * class:	MAnnivDate
 * desc:	set all attributes to default value
 * param:	none
 * return:	nothing
 **/
void MAnnivDate::Clear()
{
	ZeroDate();
	_wID = ANID_NONE;
	_strDesc.clear();
	_strModule.clear();
	_wFlags = MADF_NONE;
	_bRemind = BST_INDETERMINATE;
	_wDaysEarlier = (uint16_t)-1;
}

/**
 * name:	Set
 * class:	MAnnivDate
 * desc:	set new date
 * param:	none
 * return:	nothing
 **/
void MAnnivDate::SetDate(const SYSTEMTIME &st)
{
	ZeroDate();
	Year(st.wYear);
	Month(st.wMonth);
	Day(st.wDay);
}

/**
 * name:	Set
 * class:	MAnnivDate
 * desc:	duplicates the given anniversary date class
 * param:	none
 * return:	nothing
 **/
void MAnnivDate::SetDate(const MAnnivDate &mda)
{
	SetDate(mda.SystemTime());
	_wID = mda.Id();
	_strDesc = mda.Description();
	_strModule = mda.Module();
	_wFlags = mda.Flags();
	_bRemind = mda.RemindOption();
	_wDaysEarlier = mda.RemindOffset();
}

/**
 * name:	IsValid
 * class:	MAnnivDate
 * desc:	compare the current date with the given one in st
 * param:	st	- SYSTEMTIME to compare with
 * return:	number of days the st differs from the class value
 **/
__inline uint8_t MAnnivDate::IsValid() const
{
	return (
		Month() > 0 && Month() < 13 &&
		Day() > 0 && Day() <= DaysInMonth(Month())
 );
}

/**
 * name:	CompareDays
 * class:	MAnnivDate
 * desc:	compare the current date with the given one in st
 * param:	mt	- MTime to compare with
 * return:	number of days the mt differs from the class value
 **/
int MAnnivDate::CompareDays(MTime mt) const
{
	mt.Year(Year());
	return DayOfYear() - mt.DayOfYear();
}

/**
 * name:	Compare
 * class:	MAnnivDate
 * desc:	compare the current date with the given one in st
 * param:	st	- SYSTEMTIME to compare with
 * return:	number of days the st differs from the class value
 **/
uint8_t MAnnivDate::IsEqual(const SYSTEMTIME &st) const
{
	return (
		Day() == st.wDay &&
		Month() == st.wMonth &&
		Year() == st.wYear
 );
}

/**
 * name:	DateStamp
 * class:	MAnnivDate
 * desc:	return the current date encoded as an uint32_t
 * param:	nothing
 * return:	uint32_t encoded date
 **/
uint32_t MAnnivDate::DateStamp() const
{
	uint32_t dwStamp;

	if (!IsValid()) return 0;

	dwStamp	= (Day() << 24) & 0xFF000000;
	dwStamp |= (Month() << 16) & 0x00FF0000;
	dwStamp |= Year() & 0x0000FFFF;
	return dwStamp;
}

/**
 * name:	DateStamp
 * class:	MAnnivDate
 * desc:	set the date according to a datestamp
 * param:	dwStamp	- the dword encoded date
 * return:	nothing
 **/
void MAnnivDate::DateStamp(const uint32_t dwStamp)
{
	Day((const uint16_t)((dwStamp & 0xFF000000) >> 24));
	Month((const uint16_t)((dwStamp & 0x00FF0000) >> 16));
	Year((const uint16_t)(dwStamp & 0x0000FFFF));
}

/**
 * name:	Age
 * class:	MAnnivDate
 * desc:	calculates the age according to the date of the class and current date
 * param:	pNow	- optional pointer to a MTime class to specify a certain time to use for age calculation
 * return:	number of years the anniversary differs from now
 **/
int MAnnivDate::Age(MTime *pNow)
{
	int age = 0;
	MTime now;

	if (!IsValid()) return -1;

	if (pNow) now = *pNow;
	else now.GetLocalTime();

	if (Year() == 0)
		return 0;

	age = now.Year() - Year();
	if (age > 1 && CompareDays(now) > 0)
		age--;
	return age;
}

/**
 * name:	Zodiac
 * class:	MAnnivDate
 * desc:	returns the zodiac icon and text for the date
 * param:	none
 * return:	structure, holding zodiac information
 **/

struct
{
	uint16_t startDays;
	uint16_t endDays;
	LPCTSTR szZodiac;
	int szZodiacIcon;
}
static zodiac[] = {
	{ 80,  110,	LPGENW("Aries"),       IDI_ZOD_ARIES       }, // Widder
	{ 111, 140,	LPGENW("Taurus"),      IDI_ZOD_TAURUS      }, // Stier
	{ 141, 172,	LPGENW("Gemini"),      IDI_ZOD_GEMINI      }, // Zwillinge
	{ 173, 203,	LPGENW("Cancer"),      IDI_ZOD_CANCER      }, // Krebs
	{ 204, 235,	LPGENW("Leo"),         IDI_ZOD_LEO         }, // Löwe
	{ 236, 266,	LPGENW("Virgo"),       IDI_ZOD_VIRGO       }, // Jungfrau
	{ 267, 296,	LPGENW("Libra"),       IDI_ZOD_LIBRA       }, // Waage
	{ 297, 326,	LPGENW("Scorpio"),     IDI_ZOD_SCORPIO     }, // Scorpion
	{ 327, 355,	LPGENW("Sagittarius"), IDI_ZOD_SAGITTARIUS }, // Schütze
	{ 356, 364,	LPGENW("Capricorn"),   IDI_ZOD_CAPRICORN   }, // Steinbock
	{   1,  19, LPGENW("Capricorn"),   IDI_ZOD_CAPRICORN   }, // Steinbock
	{  20,  49, LPGENW("Aquarius"),    IDI_ZOD_AQUARIUS    }, // Wassermann
	{  50,  79, LPGENW("Pisces"),      IDI_ZOD_PISCES      }, // Fische
	{   0,	0,	nullptr,               0                   }  // end of array
};

MZodiac MAnnivDate::Zodiac()
{
	const uint16_t wDays = DayOfYear();

	int i;
	for (i = 0; i < 13 && (wDays < zodiac[i].startDays || wDays > zodiac[i].endDays); i++);

	MZodiac mZodiac;
	mZodiac.hIcon = g_plugin.getIcon(zodiac[i].szZodiacIcon);
	mZodiac.pszName = zodiac[i].szZodiac;
	return mZodiac;
}

/***********************************************************************************************************
 * reading and writing options
 ***********************************************************************************************************/

/**
 * name:	DBGetReminderOpts
 * class:	MAnnivDate
 * desc:	read reminder options for previously read date from database
 * param:	hContact	- handle to a contact to read the date from
 * return:	0 on success, 1 otherwise
 **/
int MAnnivDate::DBGetReminderOpts(MCONTACT hContact)
{
	if (!hContact || hContact == INVALID_CONTACT_ID)
		return 1;

	if (_wID == ANID_BIRTHDAY) {
		_bRemind = db_get_b(hContact, USERINFO, SET_REMIND_BIRTHDAY_ENABLED, BST_INDETERMINATE);
		_wDaysEarlier = db_get_w(hContact, USERINFO, SET_REMIND_BIRTHDAY_OFFSET, (uint16_t)-1);
	}
	else if (_wID <= ANID_LAST) {
		char pszSetting[MAXSETTING];

		// read reminder option
		mir_snprintf(pszSetting, "Anniv%dReminder", _wID);
		_bRemind = db_get_b(hContact, Module(), pszSetting, BST_INDETERMINATE);
		// read offset
		mir_snprintf(pszSetting, "Anniv%dOffset", _wID);
		_wDaysEarlier = db_get_w(hContact, Module(), pszSetting, (uint16_t)-1);
	}
	else {
		_bRemind = BST_INDETERMINATE;
		_wDaysEarlier = (uint16_t)-1;
	}
	return 0;
}

/**
 * name:	DBWriteReminderOpts
 * class:	MAnnivDate
 * desc:	write reminder options for date to database
 * param:	hContact	- handle to a contact to read the date from
 * return:	0 on success, 1 otherwise
 **/
int MAnnivDate::DBWriteReminderOpts(MCONTACT hContact)
{
	if (!hContact || hContact == INVALID_CONTACT_ID)
		return 1;

	if (_wID == ANID_BIRTHDAY) {
		if (_bRemind == BST_INDETERMINATE)
			db_unset(hContact, USERINFO, SET_REMIND_BIRTHDAY_ENABLED);
		else
			db_set_b(hContact, USERINFO, SET_REMIND_BIRTHDAY_ENABLED, _bRemind);

		if (_wDaysEarlier == (uint16_t)-1)
			db_unset(hContact, USERINFO, SET_REMIND_BIRTHDAY_OFFSET);
		else
			db_set_w(hContact, USERINFO, SET_REMIND_BIRTHDAY_OFFSET, _wDaysEarlier);
	}
	else if (_wID <= ANID_LAST) {
		char pszSetting[MAXSETTING];
		// read reminder option
		mir_snprintf(pszSetting, "Anniv%dReminder", _wID);
		if (_bRemind == BST_INDETERMINATE)
			db_unset(hContact, USERINFO, pszSetting);
		else
			db_set_b(hContact, USERINFO, pszSetting, _bRemind);

		// read offset
		mir_snprintf(pszSetting, "Anniv%dOffset", _wID);
		if (_wDaysEarlier == (uint16_t)-1)
			db_unset(hContact, USERINFO, pszSetting);
		else
			db_set_w(hContact, USERINFO, pszSetting, _wDaysEarlier);
	}
	return 0;
}

/***********************************************************************************************************
 * reading, writing and deleting general date
 ***********************************************************************************************************/

/**
 * name:	DBGetDate
 * class:	MAnnivDate
 * desc:	read a certain date from database
 * param:	hContact	- handle to a contact to read the date from
 *			pszModule	- module holding the date
 *			szDay		- setting of the day to read
 *			szMonth		- setting of the month to read
 *			szYear		- setting of the year to read
 * return:	0 on success, 1 otherwise
 **/
int MAnnivDate::DBGetDate(MCONTACT hContact, LPCSTR pszModule, LPCSTR szDay, LPCSTR szMonth, LPCSTR szYear)
{
	ZeroDate();

	int wtmp = db_get_w(hContact, pszModule, szYear);
	if (wtmp <= 1900)
		wtmp = 0;
	Year(wtmp);

	wtmp = db_get_w(hContact, pszModule, szMonth);
	if (wtmp > 0 && wtmp < 13) {
		Month(wtmp);

		wtmp = db_get_w(hContact, pszModule, szDay);
		if (wtmp > 0 && wtmp <= DaysInMonth(Month())) {
			Day(wtmp);
			// date was correctly read from db
			_strModule = pszModule;
			return 0;
		}
	}
	ZeroDate();
	return 1;
}

/**
 * name:	DBWriteDate
 * class:	MAnnivDate
 * desc:	write a certain date from database
 * param:	hContact	- handle to a contact to read the date from
 *			pszModule	- module holding the date
 *			szDay		- setting of the day to read
 *			szMonth		- setting of the month to read
 *			szYear		- setting of the year to read
 * return:	0 on success, 1 otherwise
 **/
int MAnnivDate::DBWriteDate(MCONTACT hContact, LPCSTR pszModule, LPCSTR szDay, LPCSTR szMonth, LPCSTR szYear)
{
	return
		db_set_b(hContact, pszModule, szDay, (uint8_t)Day()) ||
		db_set_b(hContact, pszModule, szMonth, (uint8_t)Month()) ||
		db_set_w(hContact, pszModule, szYear, Year());
}

/**
 * name:	DBDeleteDate
 * class:	MAnnivDate
 * desc:	delete a certain date from database
 * param:	hContact	- handle to a contact to read the date from
 *			pszModule	- module holding the date
 *			szDay		- setting of the day to read
 *			szMonth		- setting of the month to read
 *			szYear		- setting of the year to read
 * return:	0 on success, 1 otherwise
 **/
int MAnnivDate::DBDeleteDate(MCONTACT hContact, LPCSTR pszModule, LPCSTR szDay, LPCSTR szMonth, LPCSTR szYear) const
{
	int ret;

	ret = db_unset(hContact, pszModule, szDay);
	ret &= db_unset(hContact, pszModule, szMonth);
	ret &= db_unset(hContact, pszModule, szYear);
	return ret;
}

/***********************************************************************************************************
 * reading, writing and deleting general datestamp
 ***********************************************************************************************************/

/**
 * name:	DBGetDateStamp
 * class:	MAnnivDate
 * desc:	Read a datestamp from database. A datestamp is an uint32_t of the form <ddmmyyyy>.
 * param:	hContact		- handle to a contact to read the datestamp from
 *			pszModule		- module to read the datestamp from
 *			pszSetting		- key used to identify the datestamp
 * return:	0 on success, 1 otherwise
 **/
int MAnnivDate::DBGetDateStamp(MCONTACT hContact, LPCSTR pszModule, LPCSTR pszSetting)
{
	DBVARIANT dbv;
	if (DB::Setting::GetAsIs(hContact, pszModule, pszSetting, &dbv))
		return 1;

	if (dbv.type != DBVT_DWORD) {
		db_free(&dbv);
		return 1;
	}
	DateStamp(dbv.dVal);
	return IsValid() == 0;
}

/**
 * name:	DBWriteDateStamp
 * class:	MAnnivDate
 * desc:	Write a datestamp to database. A datestamp is an uint32_t of the form <ddmmyyyy>.
 * param:	hContact		- handle to a contact to write the datestamp to
 *			pszModule	 - module to write the datestamp to
 *			pszSetting	- key used to save the datestamp
 * return:	0 on success, 1 otherwise
 **/
int MAnnivDate::DBWriteDateStamp(MCONTACT hContact, LPCSTR pszModule, LPCSTR pszSetting)
{
	if (hContact == INVALID_CONTACT_ID || pszModule == nullptr || *pszModule	== 0 || pszSetting == nullptr || *pszSetting == 0)
		return 1;

	uint32_t dwStamp = DateStamp();
	return (dwStamp == 0) ? 1 : db_set_dw(hContact, pszModule, pszSetting, dwStamp);
}

/***********************************************************************************************************
 * reading, writing and deleting birthday
 ***********************************************************************************************************/

/**
 * name:	DBGetBirthDate
 * class:	MAnnivDate
 * desc:	try to read birthday date from all known modules
 * param:	hContact		- handle to a contact to read the date from
 *			pszProto		- basic protocol module
 * return:	0 on success, 1 otherwise
 **/
int MAnnivDate::DBGetBirthDate(MCONTACT hContact, LPSTR pszProto)
{
	Clear();

	// try to get birthday from any custom module
	if (	!DBGetDate(hContact, USERINFO, SET_CONTACT_BIRTHDAY, SET_CONTACT_BIRTHMONTH, SET_CONTACT_BIRTHYEAR) ||
			!DBGetDate(hContact, SET_CONTACT_BIRTHDAY, SET_CONTACT_BIRTHDAY, SET_CONTACT_BIRTHMONTH, SET_CONTACT_BIRTHYEAR) ||
			!DBGetDate(hContact, USERINFO, SET_CONTACT_DOBD, SET_CONTACT_DOBM, SET_CONTACT_DOBY))
	{
		SetFlags(MADF_HASCUSTOM);
	}
	// if pszProto is set to NULL, this will be scaned only incase the birthday date has not been found yet
	else if (pszProto || (pszProto = Proto_GetBaseAccountName(hContact)) != nullptr)
	{
		// try to get birthday from basic protocol
		if (!DBGetDate(hContact, pszProto, SET_CONTACT_BIRTHDAY, SET_CONTACT_BIRTHMONTH, SET_CONTACT_BIRTHYEAR))
		{
			SetFlags(MADF_HASPROTO);
		}
		// try to get birthday date from metacontact's subcontact
		else if (DB::Module::IsMetaAndScan(pszProto)) {
			// try to get setting from the default subcontact first
			const int def = db_mc_getDefaultNum(hContact);
			if (def > -1 && def < INT_MAX) {
				MCONTACT hSubContact = db_mc_getSub(hContact, def);
				if (hSubContact && !DBGetBirthDate(hSubContact, nullptr)) {
					RemoveFlags(MADF_HASCUSTOM);
					SetFlags(MADF_HASMETA);
				}
			}

			// scan all subcontacts for the setting
			if (_wFlags == 0) {
				const int cnt = db_mc_getSubCount(hContact);

				if (cnt < INT_MAX) {
					for (int i = 0; i < cnt; i++) {
						if (i != def) {
							MCONTACT hSubContact = db_mc_getSub(hContact, i);
							if (hSubContact && !DBGetBirthDate(hSubContact, nullptr)) {
								RemoveFlags(MADF_HASCUSTOM);
								SetFlags(MADF_HASMETA);
								break;
	}	}	}	}	}	}	}

	if (_wFlags != 0) {
		_wID = ANID_BIRTHDAY;
		_strDesc = TranslateT("Birthday");
		return 0;
	}
	return 1;
}

/**
 * name:	DBWriteBirthDate
 * class:	MAnnivDate
 * desc:	write birthday date to desired module
 * param:	hContact		- handle to a contact to read the date from
 *			pszProto		- basic protocol module
 * return:	0 on success, 1 otherwise
 **/

int MAnnivDate::DBWriteBirthDate(MCONTACT hContact)
{
	int rc = DBWriteDate(hContact, USERINFO, SET_CONTACT_BIRTHDAY, SET_CONTACT_BIRTHMONTH, SET_CONTACT_BIRTHYEAR);
	if (!rc) {
		if (
				// only delete values from current contact's custom modules
				!(_wFlags & (MADF_HASPROTO|MADF_HASMETA)) &&
				// check whether user wants this feature
				g_plugin.getByte(SET_REMIND_SECUREBIRTHDAY, TRUE))
		{
			// keep the database clean
			DBDeleteDate(hContact, SET_CONTACT_BIRTHDAY, SET_CONTACT_BIRTHDAY, SET_CONTACT_BIRTHMONTH, SET_CONTACT_BIRTHYEAR);
			DBDeleteDate(hContact, USERINFO, SET_CONTACT_DOBD, SET_CONTACT_DOBM, SET_CONTACT_DOBY);
		}

		rc = db_set_w(hContact, USERINFO, SET_CONTACT_AGE, Age());
	}
	return rc;
}

/**
 * name:	DBDeleteBirthDate
 * class:	MAnnivDate
 * desc:	delete birthday date from desired module
 * param:	hContact		- handle to a contact to read the date from
 *			pszProto		- basic protocol module
 * return:	0 on success, 1 otherwise
 **/

int MAnnivDate::DBDeleteBirthDate(MCONTACT hContact)
{
	return DBDeleteDate(hContact, Module(), SET_CONTACT_BIRTHDAY, SET_CONTACT_BIRTHMONTH, SET_CONTACT_BIRTHYEAR);
}

/***********************************************************************************************************
 * reading, writing and deleting anniversary
 ***********************************************************************************************************/

/**
 * name:	DBGetAnniversaryDate
 * class:	MAnnivDate
 * desc:	try to read anniversary date from userinfo module
 * param:	hContact		- handle to a contact to read the date from
 *			pszProto		- basic protocol module
 * return: 0 on success, 1 otherwise
 **/

int MAnnivDate::DBGetAnniversaryDate(MCONTACT hContact, uint16_t iIndex)
{
	Clear();

	// read date and convert older versions
	CHAR szStamp[MAXSETTING];
	mir_snprintf(szStamp, "Anniv%dDate", iIndex);
	int rc = DBGetDateStamp(hContact, USERINFO, szStamp);
	if (!rc) {
		_strModule = USERINFO;
		_wFlags |= MADF_HASCUSTOM;
		_wID = iIndex;

		// read description
		mir_snprintf(szStamp, "Anniv%dDesc", iIndex);
		DBVARIANT dbv;
		if (!DB::Setting::GetWString(hContact, USERINFO, szStamp, &dbv)) {
			_strDesc = dbv.pwszVal;
			db_free(&dbv);
		}
	}
	return rc;
}

/**
 * name:	DBWriteAnniversaryDate
 * class:	MAnnivDate
 * desc:	write birthday date to desired module
 * param:	hContact		- handle to a contact to read the date from
 *			pszProto		- basic protocol module
 * return:	0 on success, 1 otherwise
 **/
int MAnnivDate::DBWriteAnniversaryDate(MCONTACT hContact, uint16_t wIndex)
{
	// date can only be written to db as anniversary if it is not marked as birthday
	if (wIndex <= ANID_LAST && _wID != ANID_BIRTHDAY) {
		_wID = wIndex;

		CHAR pszSetting[MAXSETTING];
		mir_snprintf(pszSetting, "Anniv%dDate", wIndex);
		if (!DBWriteDateStamp(hContact, USERINFO, pszSetting)) {
			// write description
			mir_snprintf(pszSetting, "Anniv%dDesc", wIndex);
			db_set_ws(hContact, USERINFO, pszSetting, (LPTSTR)Description());
			return 0;
		}
		// delete date if written incompletely
		db_unset(hContact, USERINFO, pszSetting);
	}
	return 1;
}

/***********************************************************************************************************
 * automatic backup service
 ***********************************************************************************************************/

static uint16_t AskUser(MCONTACT hContact, MAnnivDate *pOldCustomDate, MAnnivDate *pNewProtoDate)
{
	MSGBOX	MB;
	wchar_t	 szMsg[MAXDATASIZE];
	wchar_t	 szDate[MAX_PATH];
	wchar_t	 szoldDate[MAX_PATH];

	pOldCustomDate->DateFormat(szoldDate, _countof(szoldDate));
	pNewProtoDate->DateFormat(szDate, _countof(szDate));

	mir_snwprintf(szMsg,
		TranslateT("%s provides a new birthday via protocol.\nIt is %s. The old one was %s.\n\nDo you want to use this as the new birthday for this contact?"),
		Clist_GetContactDisplayName(hContact), szDate, szoldDate);

	MB.cbSize = sizeof(MSGBOX);
	MB.hParent = nullptr;
	MB.hiLogo = g_plugin.getIcon(IDI_ANNIVERSARY);
	MB.hiMsg = nullptr;
	MB.uType = MB_YESALLNO|MB_ICON_QUESTION|MB_INFOBAR|MB_NOPOPUP;
	MB.ptszTitle = LPGENW("Update custom birthday");
	MB.ptszInfoText = LPGENW("Keeps your custom birthday up to date.");
	MB.ptszMsg = szMsg;
	return MsgBoxService(NULL, (LPARAM)&MB);
}

/**
 * name:	BackupBirthday
 * class:	MAnnivDate
 * desc:	tries to read birthday date from protocol and compares it with the classes date
 * param:	hContact		- handle to a contact to read the date from
 *			pszProto		- basic protocol module
 * return:	0 if backup was done, 1 otherwise
 **/

int MAnnivDate::BackupBirthday(MCONTACT hContact, LPSTR pszProto, const uint8_t bDontIgnoreAnything, PWORD lastAnswer)
{
	if (!hContact)
		return 1;

	// This birthday is a protocol based or metasubcontact's anniversary and no custom information exist,
	// so directly back it up under all circumstances!
	if ( _wFlags & (MADF_HASPROTO | MADF_HASMETA)) {
		if (lastAnswer == nullptr) {
			DBWriteDateStamp(hContact, USERINFO, SET_REMIND_BIRTHDAY_IGNORED);
			DBWriteBirthDate(hContact);
		}
		return 1;
	}

	// A custom birthday was set by user before and is not to be ignored
	if ((_wFlags & MADF_HASCUSTOM) && (bDontIgnoreAnything || !lastAnswer || (*lastAnswer != IDNONE))) {
		if (!pszProto)
			pszProto = Proto_GetBaseAccountName(hContact);

		if (pszProto) {
			uint8_t bIsMeta = DB::Module::IsMeta(pszProto);
			uint8_t bIsMetaSub = !bIsMeta && db_mc_isSub(hContact);
			MAnnivDate mdbNewProto;
			MAnnivDate mdbIgnore;

			const int nSubContactCount = (bIsMeta) ? db_mc_getSubCount(hContact) : 0;

			uint8_t bWantBackup = !mdbNewProto.DBGetDate(hContact, pszProto, SET_CONTACT_BIRTHDAY, SET_CONTACT_BIRTHMONTH, SET_CONTACT_BIRTHYEAR)
									&& !IsEqual(mdbNewProto.SystemTime())
									&& (bDontIgnoreAnything || (db_get_dw(hContact, USERINFO, SET_REMIND_BIRTHDAY_IGNORED, 0) != mdbNewProto.DateStamp()))
									&& !bIsMetaSub;

			// allow backup only, if the custom setting differs from all meta subcontacts' protocol based settings, too.
			for (int i = 0; (i < nSubContactCount) && bWantBackup && bIsMeta; i++) {
				MCONTACT hSubContact = db_mc_getSub(hContact, i);
				if (hSubContact && !mdbIgnore.DBGetDate(hSubContact, pszProto, SET_CONTACT_BIRTHDAY, SET_CONTACT_BIRTHMONTH, SET_CONTACT_BIRTHYEAR))
					bWantBackup = bWantBackup
											&& !IsEqual(mdbIgnore.SystemTime())
											&& (bDontIgnoreAnything || (db_get_dw(hSubContact, USERINFO, SET_REMIND_BIRTHDAY_IGNORED, 0) != mdbIgnore.DateStamp()));
			}
			if (bWantBackup) {
				if (!lastAnswer || *lastAnswer != IDALL) {
					uint16_t rc = AskUser(hContact, this, &mdbNewProto);
					if (lastAnswer)
						*lastAnswer = rc;

					if (IDYES != rc && IDALL != rc) {
						// special handling for metasubcontacts required?!
						mdbNewProto.DBWriteDateStamp(hContact, USERINFO, SET_REMIND_BIRTHDAY_IGNORED);
						bWantBackup = FALSE;
					}
				}
				if (bWantBackup) {
					Set(mdbNewProto);
					DBWriteDateStamp(hContact, USERINFO, SET_REMIND_BIRTHDAY_IGNORED);
					DBWriteBirthDate(hContact);

					// update metasubcontacts
					for (int i = 0; i < nSubContactCount; i++) {
						MCONTACT hSubContact = db_mc_getSub(hContact, i);
						if (hSubContact) {
							if (!mdbIgnore.DBGetDate(hSubContact, Proto_GetBaseAccountName(hSubContact), SET_CONTACT_BIRTHDAY, SET_CONTACT_BIRTHMONTH, SET_CONTACT_BIRTHYEAR))
								mdbIgnore.DBWriteDateStamp(hSubContact, USERINFO, SET_REMIND_BIRTHDAY_IGNORED);

							DBWriteBirthDate(hSubContact);
						}
					}
					return 0;
				}
			}
		}
	}
	return 1;
}
