#include "include\souistd.h"
#include "helper\SMenuEx.h"
#include "helper\SplitString.h"

namespace SOUI
{
    #define TIMERID_POPSUBMENU  100
    #define TIME_PUPSUBMENU     500
    
    #define WIDTH_MENU_INIT     10000
    #define WIDTH_MENU_MAX      2000
    //////////////////////////////////////////////////////////////////////////
    class SMenuExRoot : public SWindow
    {
        SOUI_CLASS_NAME(SMenuExRoot,L"menuRoot")
        friend class SMenuEx;
        friend class SMenuExItem;
    protected:
        ISkinObj * m_pItemSkin;
        ISkinObj * m_pIconSkin;
        ISkinObj * m_pCheckSkin;
        
        SMenuEx  * m_pMenuEx;

        int         m_nItemHei;
        int         m_nIconBarWidth;
        CPoint      m_ptIcon;
        CRect       m_rcMargin;
        
        SOUI_ATTRS_BEGIN()
            ATTR_SKIN(L"itemSkin",m_pItemSkin,FALSE)
            ATTR_SKIN(L"checkSkin",m_pCheckSkin,FALSE)
            ATTR_INT(L"itemHeight",m_nItemHei,FALSE)
            ATTR_POINT(L"iconPos", m_ptIcon,FALSE)
            ATTR_SKIN(L"iconSkin",m_pIconSkin,FALSE)
            ATTR_RECT(L"margin", m_rcMargin,FALSE)
        SOUI_ATTRS_END()

    public:
        SMenuExRoot(SMenuEx * pMenuEx)
            :m_pItemSkin(GETBUILTINSKIN(SKIN_SYS_MENU_SKIN))
            //,m_pSepSkin(GETBUILTINSKIN(SKIN_SYS_MENU_SEP))
            ,m_nIconBarWidth(24)
            ,m_pCheckSkin(GETBUILTINSKIN(SKIN_SYS_MENU_CHECK))
            ,m_pIconSkin(NULL)
            ,m_nItemHei(26)
            ,m_pMenuEx(pMenuEx)
        {

        }
        
        CSize CalcMenuSize()
        {
            CRect rcContainer(0,0,WIDTH_MENU_INIT,WIDTH_MENU_INIT);
            return GetDesiredSize(rcContainer);
        }
        
        virtual CSize GetDesiredSize(LPCRECT pRcContainer)
        {
            CSize szRet;
            SWindow *pItem = GetWindow(GSW_FIRSTCHILD);
            while(pItem)
            {
                CSize szItem = pItem->GetDesiredSize(pRcContainer);
                if(szItem.cx<WIDTH_MENU_MAX)
                    szRet.cx = max(szRet.cx,szItem.cx);
                szRet.cy += szItem.cy;
                pItem = pItem->GetWindow(GSW_NEXTSIBLING);
            }
            szRet.cx += m_rcMargin.left + m_rcMargin.right;
            szRet.cy += m_rcMargin.top + m_rcMargin.bottom;
            
            if(szRet.cx > m_nMaxWidth)
                szRet.cx = m_nMaxWidth;
            return szRet;
        }
        
        virtual BOOL InitFromXml(pugi::xml_node xmlNode)
        {
            BOOL bRet = __super::InitFromXml(xmlNode);

            //�ҵ����ڵ㣬��ȡ�ڸ��ڵ������õ�ȫ�ֲ˵���������
            pugi::xml_node xmlRoot = xmlNode.root().first_child();
            if(xmlNode != xmlRoot)
            {
                SObject::InitFromXml(xmlRoot);
            }
            return bRet;
        }    

        virtual BOOL CreateChildren(pugi::xml_node xmlNode);

        virtual void UpdateChildrenPosition()
        {
            CRect rcClient = GetClientRect();
            rcClient.DeflateRect(m_rcMargin.left,m_rcMargin.top,m_rcMargin.right,m_rcMargin.bottom);

            SWindow *pChild = GetWindow(GSW_FIRSTCHILD);
            CRect rcItem = rcClient;
            rcItem.bottom = rcItem.top;
            while(pChild)
            {
                CSize szItem = pChild->GetDesiredSize(rcClient);
                rcItem.top = rcItem.bottom;
                rcItem.bottom += szItem.cy;
                pChild->Move(rcItem);
                pChild = pChild->GetWindow(GSW_NEXTSIBLING);
            }
        }
    };

    //////////////////////////////////////////////////////////////////////////

    class SMenuExItem : public SWindow
    {
        SOUI_CLASS_NAME(SMenuExItem,L"menuItem")
    public:
        SMenuExItem(SMenuEx *pOwnerMenu,ISkinObj *pItemSkin)
        :m_pSubMenu(NULL)
        ,m_pOwnerMenu(pOwnerMenu)
        ,m_iIcon(-1)
        ,m_bCheck(FALSE)
        ,m_bRadio(FALSE)
        ,m_bSubPopped(FALSE)
        {
            m_pBgSkin = pItemSkin;
            m_style.m_bTrackMouseEvent=TRUE;
            m_style.SetAttribute(L"align",L"left",TRUE);
        }
        
        ~SMenuExItem()
        {
            if(m_pSubMenu) 
            {
                delete m_pSubMenu;
            }
        }
               
        SMenuEx * GetSubMenu()
        {
            return m_pSubMenu;
        }
        
        SMenuEx * GetOwnerMenu()
        {
            return m_pOwnerMenu;
        }
        
        void SetSubMenuFlag(BOOL bSubPopped){
            m_bSubPopped = bSubPopped;
            Invalidate();
        }
        
        void HideSubMenu()
        {
            SASSERT(m_pSubMenu);
            m_pSubMenu->HideMenu();
            SetSubMenuFlag(FALSE);
            if(m_pOwnerMenu) 
            {
                SASSERT(m_pOwnerMenu->m_pCheckItem);
                m_pOwnerMenu->m_pCheckItem = NULL;
            }
        }
        
        void ShowSubMenu()
        {
            SASSERT(m_pSubMenu);
            m_pOwnerMenu->PopupSubMenu(this);
        }
    protected:
        virtual BOOL CreateChildren(pugi::xml_node xmlNode)
        {
            __super::CreateChildren(xmlNode);
            pugi::xml_node xmlChild = xmlNode.child(SMenuExItem::GetClassName());
            if(xmlChild)
            {//���Ӳ˵�
                m_pSubMenu = new SMenuEx(this);
                m_pSubMenu->LoadMenu(xmlNode);
            }
            return TRUE;
        }
        
        virtual CSize GetDesiredSize(LPCRECT pRcContainer)
        {
            CSize szRet = __super::GetDesiredSize(pRcContainer);
            
            SMenuExRoot * pMenuRoot = sobj_cast<SMenuExRoot>(GetRoot()->GetWindow(GSW_FIRSTCHILD));
            SASSERT(pMenuRoot);
            if(!m_layout.IsSpecifySize(PD_X))
            {
                szRet.cx += pMenuRoot->m_nIconBarWidth;
                if(m_pSubMenu) szRet.cx += pMenuRoot->m_pCheckSkin->GetSkinSize().cx;//�����Ӳ˵���ͷ����
            }
            if(!m_layout.IsSpecifySize(PD_Y))
            {
                szRet.cy = pMenuRoot->m_nItemHei;                
            }
            return szRet;
        }
        
        virtual void GetTextRect(LPRECT pRect)
        {
            GetClientRect(pRect);
            SMenuExRoot * pMenuRoot = sobj_cast<SMenuExRoot>(GetRoot()->GetWindow(GSW_FIRSTCHILD));
            SASSERT(pMenuRoot);
            pRect->left+=pMenuRoot->m_nIconBarWidth;
            if(m_pSubMenu) pRect->right -= pMenuRoot->m_pCheckSkin->GetSkinSize().cx;
        }
        
        
    protected:
        
        BOOL OnEraseBkgnd(IRenderTarget *pRT)
        {
            if(!m_pBgSkin) return FALSE;
            int nState=0;

            if(GetState()&WndState_Disable)
            {
                nState=2;
            }
            else if(m_bSubPopped || GetState()&WndState_Check || GetState()&WndState_PushDown || GetState()&WndState_Hover)
            {
                nState=1;
            }
            if(nState>=m_pBgSkin->GetStates()) nState=0;
            m_pBgSkin->Draw(pRT, GetClientRect(), nState); 

            return TRUE;    
        }
        
        void OnPaint(IRenderTarget *pRT)
        {
            __super::OnPaint(pRT);
            
            CRect rc=GetClientRect();
            SMenuExRoot * pMenuRoot = sobj_cast<SMenuExRoot>(GetRoot()->GetWindow(GSW_FIRSTCHILD));
            SASSERT(pMenuRoot);
            rc.right = rc.left+pMenuRoot->m_nIconBarWidth;
            rc.left += pMenuRoot->m_ptIcon.x;
            rc.top +=pMenuRoot->m_ptIcon.y;
            if(m_bCheck || m_bRadio)
            {
                SASSERT(pMenuRoot->m_pCheckSkin);
                int nState=0;
                if(m_bRadio)
                {
                    nState = m_bCheck?1:2;
                }
                CRect rcIcon(rc.TopLeft(),pMenuRoot->m_pCheckSkin->GetSkinSize());
                pMenuRoot->m_pCheckSkin->Draw(pRT,rcIcon,nState);
            }else
            {
                SASSERT(pMenuRoot->m_pIconSkin);
                CRect rcIcon(rc.TopLeft(),pMenuRoot->m_pIconSkin->GetSkinSize());
                pMenuRoot->m_pIconSkin->Draw(pRT,rcIcon,m_iIcon);
            }
            
            if(m_pSubMenu)
            {
                CRect rcArrow = GetClientRect();
                CSize szArrow = pMenuRoot->m_pCheckSkin->GetSkinSize();
                rcArrow.left = rcArrow.right - szArrow.cx;
                rcArrow.DeflateRect(0,(rcArrow.Height()-szArrow.cy)/2);
                
                pMenuRoot->m_pCheckSkin->Draw(pRT,rcArrow,3);
            }
        }
        
        SOUI_MSG_MAP_BEGIN()
            MSG_WM_ERASEBKGND_EX(OnEraseBkgnd)
            MSG_WM_PAINT_EX(OnPaint)
        SOUI_MSG_MAP_END()

    protected:
        SOUI_ATTRS_BEGIN()
            ATTR_INT(L"icon",m_iIcon,FALSE)
            ATTR_INT(L"check",m_bCheck,FALSE)
            ATTR_INT(L"radio",m_bRadio,FALSE)
        SOUI_ATTRS_END()
        
        SMenuEx * m_pSubMenu;
        SMenuEx * m_pOwnerMenu;
        int       m_iIcon;
        BOOL      m_bCheck;
        BOOL      m_bRadio;    
        BOOL      m_bSubPopped;  
    };
    
    //////////////////////////////////////////////////////////////////////////
    BOOL SMenuExRoot::CreateChildren(pugi::xml_node xmlNode)
    {
        pugi::xml_node xmlItem = xmlNode.child(SMenuExItem::GetClassName());
        while(xmlItem)
        {
            SMenuExItem *pMenuItem = new SMenuExItem(m_pMenuEx,m_pItemSkin);
            InsertChild(pMenuItem);
            pMenuItem->InitFromXml(xmlItem);
            pMenuItem->SetAttribute(L"pos",L"0,[0",TRUE);
            xmlItem = xmlItem.next_sibling(SMenuExItem::GetClassName());
        }
        return TRUE;
    }



    //////////////////////////////////////////////////////////////////////////
    class SMenuExRunData
    {
    friend class SMenuEx;
    public:
        SMenuExRunData():m_bExit(FALSE),m_nCmdID(-1)
        {
        
        }
        
        BOOL IsMenuWnd(HWND hWnd)
        {
            SPOSITION pos = m_lstMenuEx.GetTailPosition();
            while(pos)
            {
                if(m_lstMenuEx.GetPrev(pos)->m_hWnd == hWnd) return TRUE;
            }
            return FALSE;
        }
        
        void PushMenuEx(SMenuEx * pMenu)
        {
            m_lstMenuEx.AddTail(pMenu);
        }
        
        SMenuEx * GetMenuEx()
        {
            if(m_lstMenuEx.IsEmpty()) return 0;
            return m_lstMenuEx.GetTail();
        }
        
        SMenuEx * PopMenuEx()
        {
            SASSERT(!m_lstMenuEx.IsEmpty());
            SMenuEx *pMenuEx = m_lstMenuEx.RemoveTail();
            return pMenuEx;
        }
        
        SMenuEx * SMenuExFromHwnd(HWND hWnd)
        {
            SPOSITION pos = m_lstMenuEx.GetTailPosition();
            while(pos)
            {
                SMenuEx * pMenuEx = m_lstMenuEx.GetPrev(pos);
                if(pMenuEx->m_hWnd == hWnd) return pMenuEx;
            }
            return NULL;
        }
        
        BOOL IsMenuExited()
        {
            return m_bExit;
        }
        
        void ExitMenu(int nCmdID)
        {
            m_bExit=TRUE;
            m_nCmdID = nCmdID;
        }
        
        int GetCmdID(){return m_nCmdID;}
    protected:
        SList<SMenuEx*> m_lstMenuEx;
        
        BOOL m_bExit;
        int  m_nCmdID;
    };
    
    static SMenuExRunData * s_MenuData=NULL;
    
    //////////////////////////////////////////////////////////////////////////
    SMenuEx::SMenuEx(void):m_pParent(NULL),m_pHoverItem(NULL),m_pCheckItem(NULL)
    {
        m_hostAttr.m_bTranslucent = TRUE;
    }

    SMenuEx::SMenuEx(SMenuExItem *pParent):m_pParent(pParent),m_pHoverItem(NULL),m_pCheckItem(NULL)
    {

    }

    SMenuEx::~SMenuEx(void)
    {
        if(IsWindow())
            DestroyWindow();
    }

    BOOL SMenuEx::LoadMenu(LPCTSTR pszMenu)
    {
        SStringTList strMenu;
        if(1==SplitString<SStringT,TCHAR>(pszMenu,_T(':'),strMenu))
            strMenu.InsertAt(0,_T("SMENUEX"));
        
        pugi::xml_document xmlMenu;
        BOOL bLoad = LOADXML(xmlMenu,strMenu[1],strMenu[0]);
        if(!bLoad) return FALSE;
        return LoadMenu(xmlMenu.first_child());
    }
    
    BOOL SMenuEx::LoadMenu(pugi::xml_node xmlNode)
    {
        if(IsWindow()) return FALSE;
        if(xmlNode.name() != SStringW(SMenuExRoot::GetClassName())
         && xmlNode.name() != SStringW(SMenuExItem::GetClassName()))
            return FALSE;
            
        HWND hWnd = Create(NULL,WS_POPUP,WS_EX_TOOLWINDOW,0,0,0,0);
        pugi::xml_document souiXml;
        souiXml.append_child(L"SOUI").append_attribute(L"translucent").set_value(1);
        _InitFromXml(souiXml.child(L"SOUI"),0,0);
        
        if(!hWnd) return FALSE;
        
        
        SMenuExRoot *pMenuRoot = new SMenuExRoot(this);
        InsertChild(pMenuRoot);
        
        pMenuRoot->InitFromXml(xmlNode);
        pMenuRoot->GetLayout()->SetFitContent(PD_ALL);

        return TRUE;
    }

    UINT SMenuEx::TrackPopupMenu(UINT flag,int x,int y,HWND hOwner)
    {
        if(!IsWindow()) return -1;
        s_MenuData = new SMenuExRunData;

        ShowMenu(flag,x,y);
        RunMenu(hOwner);
        HideMenu();
        
        int nRet = s_MenuData->GetCmdID();
        delete s_MenuData;
        s_MenuData=NULL;
        
        if(flag & TPM_RETURNCMD)
        {
            return nRet;
        }else
        {
            ::SendMessage(hOwner,WM_COMMAND,MAKEWPARAM(nRet,0),0);
            return TRUE;
        }
        
    }

    void SMenuEx::ShowMenu(UINT uFlag,int x,int y)
    {
        
        SMenuExRoot *pMenuRoot = sobj_cast<SMenuExRoot>(GetRoot()->GetWindow(GSW_FIRSTCHILD));
        SASSERT(pMenuRoot);
        CSize szMenu = pMenuRoot->CalcMenuSize();
        
        pMenuRoot->Move(CRect(CPoint(),szMenu));
        if(uFlag&TPM_CENTERALIGN)
        {
            x -= szMenu.cx/2;
        }else if(uFlag & TPM_RIGHTALIGN)
        {
            x -= szMenu.cx;
        }
        
        if(uFlag & TPM_VCENTERALIGN)
        {
            y -= szMenu.cy/2;
        }
        else if(uFlag&TPM_BOTTOMALIGN)
        {
            y -= szMenu.cy;
        }
        SetWindowPos(HWND_TOPMOST,x,y,szMenu.cx,szMenu.cy,SWP_NOZORDER|SWP_SHOWWINDOW|SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSENDCHANGING);        
        s_MenuData->PushMenuEx(this);
    }

    void SMenuEx::HideMenu()
    {
        if(!CSimpleWnd::IsWindowVisible()) return;
        ShowWindow(SW_HIDE);
        s_MenuData->PopMenuEx();
    }
    
    void SMenuEx::HideSubMenu()
    {
        if(m_pCheckItem) m_pCheckItem->HideSubMenu();
    }
    
    int SMenuEx::OnMouseActivate(HWND wndTopLevel, UINT nHitTest, UINT message)
    {
        return MA_NOACTIVATE;
    }


    void SMenuEx::RunMenu(HWND hOwner)
    {
        SASSERT(s_MenuData);
        
        
        SetForegroundWindow(hOwner);

        BOOL bMsgQuit(FALSE);
        
        for(;;)
        {

            if(s_MenuData->IsMenuExited())
            {
                break;
            }

            if(GetForegroundWindow() != hOwner)
            {
                break;
            }
            BOOL bInterceptOther(FALSE);
            MSG msg = {0};
            if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if(msg.message == WM_KEYDOWN
                    || msg.message == WM_SYSKEYDOWN
                    || msg.message == WM_KEYUP
                    || msg.message == WM_SYSKEYUP
                    || msg.message == WM_CHAR
                    || msg.message == WM_IME_CHAR)
                {
                    //transfer the message to menu window
                    msg.hwnd = s_MenuData->GetMenuEx()->m_hWnd;
                }
                else if(msg.message == WM_LBUTTONDOWN
                    || msg.message  == WM_RBUTTONDOWN
                    || msg.message  == WM_NCLBUTTONDOWN
                    || msg.message  == WM_NCRBUTTONDOWN
                    || msg.message   ==WM_LBUTTONDBLCLK
                    )
                {
                    //click on other window
                    if(!s_MenuData->IsMenuWnd(msg.hwnd))
                    {
                        bInterceptOther = true;
                    }else
                    {
                        SMenuEx *pMenu = s_MenuData->SMenuExFromHwnd(msg.hwnd);
                        pMenu->HideSubMenu();
                    }
                }else if (msg.message == WM_LBUTTONUP
                    ||msg.message==WM_RBUTTONUP
                    ||msg.message==WM_NCLBUTTONUP
                    ||msg.message==WM_NCRBUTTONUP
                    ||msg.message==WM_CONTEXTMENU)
                {
                    if(!s_MenuData->IsMenuWnd(msg.hwnd))
                    {
                        break;
                    }

                }
                else if(msg.message == WM_QUIT)
                {

                    bMsgQuit = TRUE;
                }

                //���طǲ˵����ڵ�MouseMove��Ϣ
                if (msg.message == WM_MOUSEMOVE)
                {
                    SMenuEx *pMenu = s_MenuData->SMenuExFromHwnd(msg.hwnd);
                    if (!pMenu)
                    {
                        bInterceptOther=TRUE;
                    }
                }


                if (!bInterceptOther)
                {
                    TranslateMessage (&msg);
                    DispatchMessage (&msg);
                }

            }
            else
            {
                MsgWaitForMultipleObjects (0, 0, 0, 10, QS_ALLINPUT);
            }

            if(bMsgQuit)
            {
                PostQuitMessage(msg.wParam);
                break;
            }
        }
        
    }

    BOOL SMenuEx::_HandleEvent(EventArgs *pEvt)
    {
        if(pEvt->sender->IsClass(SMenuExItem::GetClassName()))
        {
            SMenuExItem *pMenuItem = sobj_cast<SMenuExItem>(pEvt->sender);
            if(pEvt->GetID() == EventSwndMouseHover::EventID)
            {
                if(pMenuItem->GetSubMenu() != NULL)
                {
                    CSimpleWnd::SetTimer(TIMERID_POPSUBMENU,TIME_PUPSUBMENU);
                    m_pHoverItem = pMenuItem;
                }
                if(m_pCheckItem)
                {
                    m_pCheckItem->HideSubMenu();
                }
                return FALSE;
            }else if(pEvt->GetID() == EventSwndMouseLeave::EventID)
            {
                if(pMenuItem->GetSubMenu() != NULL)
                {
                    CSimpleWnd::KillTimer(TIMERID_POPSUBMENU);
                    m_pHoverItem=NULL;
                }
                return FALSE;
            }
            
            if(pEvt->GetID() != EventCmd::EventID) return FALSE;
            SASSERT(pMenuItem);
            if(pMenuItem->GetSubMenu())
            {
                PopupSubMenu(pMenuItem);
                return FALSE;
            }else if(pMenuItem->GetID()==0)
            {
                return FALSE;
            }
            s_MenuData->ExitMenu(pMenuItem->GetID());
            return TRUE;
        }else if(m_pOwner)
        {
            return m_pOwner->GetContainer()->OnFireEvent(*pEvt);
        }else
        {
            return FALSE;
        }
    }

    void SMenuEx::OnTimer(UINT_PTR timeID)
    {
        if(timeID == TIMERID_POPSUBMENU)
        {
            PopupSubMenu(m_pHoverItem);
        }else
        {
            SetMsgHandled(FALSE);
        }
    }

    void SMenuEx::PopupSubMenu(SMenuExItem * pItem)
    {
        CSimpleWnd::KillTimer(TIMERID_POPSUBMENU);

        SMenuEx * pSubMenu = pItem->GetSubMenu();
        SASSERT(pSubMenu);
        CRect rcItem = pItem->GetWindowRect();
        ClientToScreen(&rcItem);
        HMONITOR hMor = MonitorFromWindow(m_hWnd,MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi={sizeof(MONITORINFO),0};
        GetMonitorInfo(hMor,&mi);
        
        m_pCheckItem = pItem;
        pItem->SetSubMenuFlag(TRUE);
        pSubMenu->ShowMenu(0,rcItem.right,rcItem.top);
        
    }

}