#include "StdAfx.h"
#include "UIGifAnim.h"

#ifdef _USE_GDIPLUS
using namespace Gdiplus;
#endif

///////////////////////////////////////////////////////////////////////////////////////
DECLARE_HANDLE(HZIP);	// An HZIP identifies a zip file that has been opened
typedef DWORD ZRESULT;
typedef struct
{ 
	int index;                 // index of this file within the zip
	char name[MAX_PATH];       // filename within the zip
	DWORD attr;                // attributes, as in GetFileAttributes.
	FILETIME atime,ctime,mtime;// access, create, modify filetimes
	long comp_size;            // sizes of item, compressed and uncompressed. These
	long unc_size;             // may be -1 if not yet known (e.g. being streamed in)
} ZIPENTRY;
typedef struct
{ 
	int index;                 // index of this file within the zip
	TCHAR name[MAX_PATH];      // filename within the zip
	DWORD attr;                // attributes, as in GetFileAttributes.
	FILETIME atime,ctime,mtime;// access, create, modify filetimes
	long comp_size;            // sizes of item, compressed and uncompressed. These
	long unc_size;             // may be -1 if not yet known (e.g. being streamed in)
} ZIPENTRYW;
#define OpenZip OpenZipU
#define CloseZip(hz) CloseZipU(hz)
extern HZIP OpenZipU(void *z,unsigned int len,DWORD flags);
extern ZRESULT CloseZipU(HZIP hz);
#ifdef _UNICODE
#define ZIPENTRY ZIPENTRYW
#define GetZipItem GetZipItemW
#define FindZipItem FindZipItemW
#else
#define GetZipItem GetZipItemA
#define FindZipItem FindZipItemA
#endif
extern ZRESULT GetZipItemA(HZIP hz, int index, ZIPENTRY *ze);
extern ZRESULT GetZipItemW(HZIP hz, int index, ZIPENTRYW *ze);
extern ZRESULT FindZipItemA(HZIP hz, const TCHAR *name, bool ic, int *index, ZIPENTRY *ze);
extern ZRESULT FindZipItemW(HZIP hz, const TCHAR *name, bool ic, int *index, ZIPENTRYW *ze);
extern ZRESULT UnzipItem(HZIP hz, int index, void *dst, unsigned int len, DWORD flags);
///////////////////////////////////////////////////////////////////////////////////////

namespace DuiLib
{
	REGIST_DUICONTROL(CGifAnimUI)
	CGifAnimUI::CGifAnimUI(void) : CControlUI()
	{
		m_pGifImage			=	NULL;
		m_pPropertyItem		=	NULL;
		m_nFrameCount		=	0;	
		m_nFramePosition	=	0;	
		m_bIsAutoPlay		=	true;
		m_bIsAutoSize		=	false;
		m_bIsPlaying		=	false;

	}


	CGifAnimUI::~CGifAnimUI(void)
	{
		DeleteGif();
		m_pManager->KillTimer( this, EVENT_TIEM_ID );

	}

	LPCTSTR CGifAnimUI::GetClass() const
	{
		return DUI_CTR_GIFANIM;
	}

	LPVOID CGifAnimUI::GetInterface( LPCTSTR pstrName )
	{
		if( _tcscmp(pstrName, DUI_CTR_GIFANIM) == 0 ) return static_cast<CGifAnimUI*>(this);
		return CControlUI::GetInterface(pstrName);
	}

	void CGifAnimUI::DoInit()
	{
		InitGifImage();
	}

	bool CGifAnimUI::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
	{
		if ( NULL == m_pGifImage )
		{		
			InitGifImage();
		}
		DrawFrame( hDC );
        return true;
	}

	void CGifAnimUI::DoEvent( TEventUI& event )
	{
		if( event.Type == UIEVENT_TIMER )
			OnTimer( (UINT_PTR)event.wParam );
	}

	void CGifAnimUI::SetVisible(bool bVisible /* = true */)
	{
		CControlUI::SetVisible(bVisible);
		if (bVisible)
			PlayGif();
		else
			StopGif();
	}

	void CGifAnimUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
	{
		if( _tcscmp(pstrName, _T("bkimage")) == 0 ) SetBkImage(pstrValue);
		else if( _tcscmp(pstrName, _T("autoplay")) == 0 ) {
			SetAutoPlay(_tcscmp(pstrValue, _T("true")) == 0);
		}
		else if( _tcscmp(pstrName, _T("autosize")) == 0 ) {
			SetAutoSize(_tcscmp(pstrValue, _T("true")) == 0);
		}
		else
			CControlUI::SetAttribute(pstrName, pstrValue);
	}

	void CGifAnimUI::SetBkImage(LPCTSTR pStrImage)
	{
		if( m_sBkImage == pStrImage || NULL == pStrImage) return;

		m_sBkImage = pStrImage;

		StopGif();
		DeleteGif();

		Invalidate();

	}

	LPCTSTR CGifAnimUI::GetBkImage()
	{
		return m_sBkImage.GetData();
	}

	void CGifAnimUI::SetAutoPlay(bool bIsAuto)
	{
		m_bIsAutoPlay = bIsAuto;
	}

	bool CGifAnimUI::IsAutoPlay() const
	{
		return m_bIsAutoPlay;
	}

	void CGifAnimUI::SetAutoSize(bool bIsAuto)
	{
		m_bIsAutoSize = bIsAuto;
	}

	bool CGifAnimUI::IsAutoSize() const
	{
		return m_bIsAutoSize;
	}

	void CGifAnimUI::PlayGif()
	{
		if (m_bIsPlaying || m_pGifImage == NULL)
		{
			return;
		}

		long lPause = ((long*) m_pPropertyItem->value)[m_nFramePosition] * 10;
		if ( lPause == 0 ) lPause = 100;
		m_pManager->SetTimer( this, EVENT_TIEM_ID, lPause );

		m_bIsPlaying = true;
	}

	void CGifAnimUI::PauseGif()
	{
		if (!m_bIsPlaying || m_pGifImage == NULL)
		{
			return;
		}

		m_pManager->KillTimer(this, EVENT_TIEM_ID);
		this->Invalidate();
		m_bIsPlaying = false;
	}

	void CGifAnimUI::StopGif()
	{
		if (!m_bIsPlaying)
		{
			return;
		}

		m_pManager->KillTimer(this, EVENT_TIEM_ID);
		m_nFramePosition = 0;
		this->Invalidate();
		m_bIsPlaying = false;
	}

	void CGifAnimUI::InitGifImage()
	{
		m_pGifImage = LoadGifFromFile(GetBkImage());
		if ( NULL == m_pGifImage ) return;
		UINT nCount	= 0;
		nCount	=	m_pGifImage->GetFrameDimensionsCount();
		GUID* pDimensionIDs	=	new GUID[ nCount ];
		m_pGifImage->GetFrameDimensionsList( pDimensionIDs, nCount );
		m_nFrameCount	=	m_pGifImage->GetFrameCount( &pDimensionIDs[0] );
		int nSize		=	m_pGifImage->GetPropertyItemSize( PropertyTagFrameDelay );
		m_pPropertyItem	=	(PropertyItem*) malloc( nSize );
		m_pGifImage->GetPropertyItem( PropertyTagFrameDelay, nSize, m_pPropertyItem );
		delete  pDimensionIDs;
		pDimensionIDs = NULL;

		if (m_bIsAutoSize)
		{
			SetFixedWidth(m_pGifImage->GetWidth());
			SetFixedHeight(m_pGifImage->GetHeight());
		}
		if (m_bIsAutoPlay && nSize > 0)
		{
			PlayGif();
		}
	}

	void CGifAnimUI::DeleteGif()
	{
		if ( m_pGifImage != NULL )
		{
			delete m_pGifImage;
			m_pGifImage = NULL;
		}

		if ( m_pPropertyItem != NULL )
		{
			free( m_pPropertyItem );
			m_pPropertyItem = NULL;
		}
		m_nFrameCount		=	0;	
		m_nFramePosition	=	0;	
	}

	void CGifAnimUI::OnTimer( UINT_PTR idEvent )
	{
		if ( idEvent != EVENT_TIEM_ID )
			return;
		m_pManager->KillTimer( this, EVENT_TIEM_ID );
		this->Invalidate();

		m_nFramePosition = (++m_nFramePosition) % m_nFrameCount;

		long lPause = ((long*) m_pPropertyItem->value)[m_nFramePosition] * 10;
		if ( lPause == 0 ) lPause = 100;
		m_pManager->SetTimer( this, EVENT_TIEM_ID, lPause );
	}

	void CGifAnimUI::DrawFrame( HDC hDC )
	{
		if ( NULL == hDC || NULL == m_pGifImage ) return;
		GUID pageGuid = FrameDimensionTime;
		Graphics graphics( hDC );
		graphics.DrawImage( m_pGifImage, m_rcItem.left, m_rcItem.top, m_rcItem.right-m_rcItem.left, m_rcItem.bottom-m_rcItem.top );
		m_pGifImage->SelectActiveFrame( &pageGuid, m_nFramePosition );
	}

	Image* CGifAnimUI::LoadGifFromFile(LPCTSTR pstrGifPath)
	{
		LPBYTE pData = NULL;
		DWORD dwSize = 0;

		do 
		{
			CResourceManager::Instance()->LoadAsset(pstrGifPath, NULL, &pData, &dwSize);

		} while (0);

		if (!pData)
		{
			return NULL;
		}

		Image* pImage = LoadGifFromMemory(pData, dwSize);
		CResourceManager::Instance()->FreeAsset(pData);
		return pImage;
	}

	Image* CGifAnimUI::LoadGifFromMemory( LPVOID pBuf,size_t dwSize )
	{
		HGLOBAL hMem = ::GlobalAlloc(GMEM_FIXED, dwSize);
		BYTE* pMem = (BYTE*)::GlobalLock(hMem);

		memcpy(pMem, pBuf, dwSize);

		IStream* pStm = NULL;
		::CreateStreamOnHGlobal(hMem, TRUE, &pStm);
		Image *pImg = Image::FromStream(pStm);
		if(!pImg || pImg->GetLastStatus() != Gdiplus::Ok)
		{
			pStm->Release();
			::GlobalUnlock(hMem);
			return 0;
		}
		return pImg;
	}


}
