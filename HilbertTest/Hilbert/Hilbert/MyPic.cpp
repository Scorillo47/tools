// MyPic.cpp : implementation file
//

#include "stdafx.h"
#include "Hilbert.h"
#include "MyPic.h"


// CMyPic

IMPLEMENT_DYNAMIC(CMyPic, CStatic)

CMyPic::CMyPic()
{

}

CMyPic::~CMyPic()
{
}


void CMyPic::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CImage img;
	img.LoadFromResource(AfxGetInstanceHandle(), IDB_BITMAP1);

	CDC dcScreen;
	dcScreen.Attach(lpDrawItemStruct->hDC);

	CDC dcMem;
	dcMem.CreateCompatibleDC(&dcScreen);
	CBitmap * pold = (CBitmap*)dcMem.SelectObject(img);

	dcMem.DrawText(L"Hi", &lpDrawItemStruct->rcItem, NULL);

	dcScreen.BitBlt(0, 0, lpDrawItemStruct->rcItem.right, lpDrawItemStruct->rcItem.bottom, &dcMem, 0, 0, SRCCOPY);

	dcMem.SelectObject(pold);
	dcScreen.Detach();
}


BEGIN_MESSAGE_MAP(CMyPic, CStatic)
	ON_WM_DRAWITEM()
END_MESSAGE_MAP()



// CMyPic message handlers


