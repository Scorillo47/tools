#pragma once


// CMyPic

class CMyPic : public CStatic
{
	DECLARE_DYNAMIC(CMyPic)

public:
	CMyPic();
	virtual ~CMyPic();

	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

protected:
	DECLARE_MESSAGE_MAP()
};


