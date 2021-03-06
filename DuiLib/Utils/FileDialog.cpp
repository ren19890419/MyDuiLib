#include "StdAfx.h"

namespace DuiLib
{
	CFileDialog::CFileDialog(const TCHAR* title)
	{
		szFile = NULL;
		memset(m_szTitleName, 0, sizeof(m_szTitleName));
		//memcpy(m_szTitleName, title, sizeof(title));
		_tcscpy(m_szTitleName, title);

		ZeroMemory(&m_stOFN, sizeof(OPENFILENAME));
		m_szFilter[0] = 0x0;

		szFile = new TCHAR[MAX_PATH];
		szFile[0] = 0x0;

		m_stOFN.lStructSize = sizeof(OPENFILENAME);
		m_stOFN.lpstrTitle = m_szTitleName;
		m_stOFN.hInstance = CPaintManagerUI::GetInstance();
		m_stOFN.lpstrFilter = m_szFilter;
		m_stOFN.lpstrCustomFilter = (LPTSTR)NULL;
		m_stOFN.nMaxCustFilter = 0L;
		m_stOFN.nFilterIndex = 1L;
		m_stOFN.lpstrFile = szFile;
		m_stOFN.nMaxFile = MAX_PATH * 101 + 1;
		m_stOFN.lpstrFileTitle = m_szTitleName;
		m_stOFN.nMaxFileTitle = MAX_PATH;
		m_stOFN.lpstrInitialDir = NULL;
		m_stOFN.nFileOffset = 0;
		m_stOFN.nFileExtension = 0;
		m_stOFN.lCustData = 0;

		m_stOFN.Flags = OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_READONLY | OFN_EXPLORER /*|OFN_LONGNAMES*/;


		//m_stOFN.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_EXTENSIONDIFFERENT;
	}

	CFileDialog::~CFileDialog(void)
	{
		if (szFile)
		{
			delete[] szFile;
			szFile = NULL;
		}
	}

	CDuiString CFileDialog::ToString() const
	{
		return _T("CFileDialog");
	}

	void CFileDialog::SetDefExt(LPCTSTR lpszDefExt)
	{
		m_stOFN.lpstrDefExt = lpszDefExt;
	}

	void CFileDialog::SetFileName(LPCTSTR lpszFileName)
	{
		m_stOFN.lpstrFile = (LPTSTR)lpszFileName;
	}

	void CFileDialog::SetFlags(DWORD dwFlags)
	{
		m_stOFN.Flags |= dwFlags;
	}

	// 示例_T("Text Files(*.txt)\0*.txt\0网页文件\0*.htm;*.html\0All Files(*.*)\0*.*\0\0")
	void CFileDialog::SetFilter(CDuiString& lpszFilter)
	{
		m_szFilter[0] = 0x0;
		_tcscpy(m_szFilter, lpszFilter.GetData());
		int i = 0;
		TCHAR lpszOld;
		while (1)
		{
			if (i >= MAX_PATH - 1)
				break;
			TCHAR lpsz = lpszFilter.GetAt(i);
			

			if(lpsz != _T('$'))
				m_szFilter[i] = lpsz;
			else
				m_szFilter[i] = _T('\0');
			lpszOld = lpsz;
			i++;
			if (i < lpszFilter.GetLength())
				lpsz = lpszFilter.GetAt(i);
			else
				break;
			if (lpszOld == '$' && lpsz == '$')
			{
				m_szFilter[i] = _T('\0');
				break;
			}
		}
	}

	void CFileDialog::SetMultiSel(bool bMultiSel/* = TRUE*/)
	{
		if (szFile != NULL)
		{
			delete[]szFile;
			szFile = NULL;
		}

		if (bMultiSel)
		{
			m_stOFN.Flags |= OFN_ALLOWMULTISELECT;

			szFile = new TCHAR[MAX_PATH * 100 + 1]; //最多可以选中100项
			szFile[0] = 0x0;

			m_stOFN.lpstrFile = szFile;
			m_stOFN.nMaxFile = MAX_PATH * 100 + 1;
		}
		else
		{
			m_stOFN.Flags &= ~OFN_ALLOWMULTISELECT;

			szFile = new TCHAR[MAX_PATH]; 
			szFile[0] = 0x0;

			m_stOFN.lpstrFile = szFile;
			m_stOFN.nMaxFile = MAX_PATH;
		}
	}

	void CFileDialog::SetParentWnd(HWND hParentWnd)
	{
		m_stOFN.hwndOwner = hParentWnd;
	}

	void CFileDialog::SetTitle(LPCTSTR lpszTitle)
	{
		m_stOFN.lpstrTitle = lpszTitle;
	}

	bool CFileDialog::ShowOpenFileDlg()
	{
		return ::GetOpenFileName(&m_stOFN);
	}

	bool CFileDialog::ShowSaveFileDlg()
	{
		return ::GetSaveFileName(&m_stOFN);
	}

	CDuiString CFileDialog::GetPathName()
	{
		CDuiString pstrFile(m_stOFN.lpstrFile);
		return pstrFile;
	}

	CDuiString CFileDialog::GetFileName()
	{
		return _T("");
	}

	CDuiString CFileDialog::GetFileExt()
	{
		return _T("");
	}

	CDuiString CFileDialog::GetFileTitle()
	{
		return _T("");
	}

	CDuiString CFileDialog::GetFolderPath()
	{
		return _T("");
	}


	CFileDialog::POSITION CFileDialog::GetStartPosition()
	{
		return (CFileDialog::POSITION)m_stOFN.lpstrFile;
	}

	CDuiString CFileDialog::GetNextPathName(CFileDialog::POSITION& pos)
	{
		LPTSTR lpsz = (LPTSTR)pos;
		if (lpsz == m_stOFN.lpstrFile) // first time
		{
			if ((m_stOFN.Flags & OFN_ALLOWMULTISELECT) == 0)
			{
				pos = NULL;
				return m_stOFN.lpstrFile;
			}

			// find char pos after first Delimiter
			while (*lpsz != _T('\0'))
				lpsz = _tcsinc(lpsz);
			lpsz = _tcsinc(lpsz);

			// if single selection then return only selection
			if (*lpsz == _T('\0'))
			{
				pos = NULL;
				return m_stOFN.lpstrFile;
			}
		}

		CDuiString strBasePath(m_stOFN.lpstrFile);
		CDuiString strFileName(lpsz);
		CDuiString strPath = Path::CombinePath(strBasePath, strFileName);

		// find char pos at next Delimiter
		while (*lpsz != _T('\0'))
			lpsz = _tcsinc(lpsz);
		lpsz = _tcsinc(lpsz);
		if (*lpsz == _T('\0')) // if double terminated then done
			pos = NULL;
		else
			pos = (CFileDialog::POSITION)lpsz;

		return strPath;
	}
}