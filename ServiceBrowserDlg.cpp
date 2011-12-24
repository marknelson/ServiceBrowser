//
// Copyright (c) 2011 Mark Nelson
//
// This software is licensed under the OSI MIT License, contained in
// the file license.txt included with this project.
//
// ServiceBrowserDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ServiceBrowser.h"
#include "ServiceBrowserDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CServiceBrowserDlg dialog




CServiceBrowserDlg::CServiceBrowserDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CServiceBrowserDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServiceBrowserDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TREE1, m_Tree);
    DDX_Control(pDX, IDC_EDIT1, m_Text);
}

BEGIN_MESSAGE_MAP(CServiceBrowserDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CServiceBrowserDlg message handlers

BOOL CServiceBrowserDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	StartBrowser();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CServiceBrowserDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CServiceBrowserDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CServiceBrowserDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CServiceBrowserDlg::StartBrowser()
{
    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);
	DNSServiceRef client = NULL;
	DNSServiceErrorType err = DNSServiceBrowse(&client, 0, 0, "_services._dns-sd._udp", "", IterateServiceTypes, this );
	if ( err == 0 ) {
        m_Text.SetWindowText( _T("Browsing for service types using _services._dns-sd._udp" ) );
        m_ClientToFdMap[client] = DNSServiceRefSockFD(client);
		SetTimer( BROWSER_TIMER, 250, 0 );
	} else {
		CString msg;
		msg.Format( _T("Error starting discovery: %d"), err );
		AfxMessageBox( msg );
	}
}

void CServiceBrowserDlg::OnTimer(UINT_PTR nIDEvent)
{
    int count = 0;
	for ( ; ; ) {
        if ( m_ClientToFdMap.size() == 0 ) {
            m_Text.SetWindowText( _T("Done browsing") );
            KillTimer( BROWSER_TIMER );
            break;
        }
		fd_set readfds;
		FD_ZERO(&readfds);
		for ( auto ii = m_ClientToFdMap.cbegin() ; ii != m_ClientToFdMap.cend() ; ii++ ) {
			FD_SET(ii->second, &readfds);
		}
		struct timeval tv = { 0, 1000 };
		int result = select(0, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
		if ( result > 0 ) {
            //
            // While iterating through the loop, the callback functions might delete
            // the client pointed to by the current iterator, so I have to increment
            // it BEFORE calling DNSServiceProcessResult
            //
			for ( auto ii = m_ClientToFdMap.cbegin() ; ii != m_ClientToFdMap.cend() ; ) {
                auto jj = ii++;
				if (FD_ISSET(jj->second, &readfds) ) {
					DNSServiceErrorType err = DNSServiceProcessResult(jj->first);
                    if ( ++count > 10 )
                        break;
				}
			}
		} else
			break;
        if ( count > 10 )
            break;
	}
	CDialogEx::OnTimer(nIDEvent);
}

void DNSSD_API CServiceBrowserDlg::IterateServiceTypes( DNSServiceRef sdRef,
                                                        DNSServiceFlags flags,
                                                        uint32_t interfaceIndex,
											            DNSServiceErrorType errorCode,
                                                        const char *serviceName,
                                                        const char *regtype,
                                                        const char *replyDomain,
                                                        void *context )
{
	CServiceBrowserDlg *p = (CServiceBrowserDlg *) context;
	//
	// Service types are added to the top level of the tree
	//
	if ( flags & kDNSServiceFlagsAdd && !errorCode ) {
		std::string r( regtype );
		size_t n = r.find_last_of('.');
		if ( n != std::string::npos )
			r = r.substr(0,n);
		n = r.find_last_of('.');
		if ( n != std::string::npos )
			r = r.substr(0,n);
		std::string service_type = serviceName;
        service_type += '.';
		service_type += r.c_str();
        auto ii = p->m_ServiceTypes.find( service_type );
        if ( ii == p->m_ServiceTypes.end() ) 
        {
            p->m_ServiceTypes.insert( service_type );
            HTREEITEM item = p->m_Tree.InsertItem( CA2T(service_type.c_str()), TVI_ROOT, TVI_SORT );
            DNSServiceRef client = NULL;
	        DNSServiceErrorType err = DNSServiceBrowse( &client, 
                                                        0, 
                                                        0, 
                                                        service_type.c_str(), 
                                                        "", 
                                                        IterateServiceInstances, 
                                                        context );
            CString msg;
            msg.Format( _T("Browsing for instances of %s"), CA2T(service_type.c_str()) );
            p->m_Text.SetWindowText( msg );
            if ( err == 0 ) {
                p->m_ClientToFdMap[client] = DNSServiceRefSockFD(client);
                p->m_TreeInsertionMap[client] = item;
            } else {
		        CString msg;
		        msg.Format( _T("Error trying to browse service type: %s"), CA2T(service_type.c_str()) );
		        AfxMessageBox( msg );
            }
        }
	}
    if ( !(flags & kDNSServiceFlagsMoreComing ) ) {
        auto ii = p->m_ClientToFdMap.find( sdRef );
        if ( ii != p->m_ClientToFdMap.end() )
            p->m_ClientToFdMap.erase( ii );
        p->m_Tree.Invalidate();
    }
}

void DNSSD_API CServiceBrowserDlg::IterateServiceInstances( DNSServiceRef sdRef,
                                                            DNSServiceFlags flags,
                                                            uint32_t interfaceIndex,
											                DNSServiceErrorType errorCode,
                                                            const char *serviceName,
                                                            const char *regtype,
                                                            const char *replyDomain,
                                                            void *context )
{
	CServiceBrowserDlg *p = (CServiceBrowserDlg *) context;
	if ( (flags & kDNSServiceFlagsAdd) && !errorCode ) {
        auto ii = p->m_TreeInsertionMap.find( sdRef );
        if ( ii != p->m_TreeInsertionMap.end() ) {
            HTREEITEM item = p->m_Tree.InsertItem( CA2T(serviceName, CP_UTF8), ii->second, TVI_SORT );
            DNSServiceRef client = NULL;
            DNSServiceErrorType err = DNSServiceResolve ( &client,
                                                          0,
                                                          interfaceIndex,
                                                          serviceName,
                                                          regtype,
                                                          replyDomain,
                                                          ResolveInstance,
                                                          context );
            CString msg;
            wchar_t *p1 = CA2T(serviceName, CP_UTF8);
            wchar_t *p2 = CA2T(regtype, CP_UTF8);
            msg.Format( _T("Resolving instance of %s %s"), p1, p2 );
            p->m_Text.SetWindowText( msg );
            if ( err == 0 ) {
                p->m_ClientToFdMap[client] = DNSServiceRefSockFD(client);
                p->m_TreeInsertionMap[client] = item;
            } else {
		        CString msg;
		        msg.Format( _T("Error trying to browse service instance: %s"), CA2T(serviceName) );
		        AfxMessageBox( msg );
            }
        }
        else
            AfxMessageBox( _T("???") );
    }
    if ( !(flags & kDNSServiceFlagsMoreComing ) ) {
        auto ii = p->m_ClientToFdMap.find( sdRef );
        if ( ii != p->m_ClientToFdMap.end() )
            p->m_ClientToFdMap.erase( ii );
        auto jj = p->m_TreeInsertionMap.find( sdRef );
        p->m_TreeInsertionMap.erase( jj );
        p->m_Tree.Invalidate();
    }
}

void DNSSD_API CServiceBrowserDlg::ResolveInstance( DNSServiceRef sdRef,
                                                    DNSServiceFlags flags,
                                                    uint32_t interfaceIndex,
                                                    DNSServiceErrorType errorCode,
                                                    const char *fullname,
                                                    const char *hosttarget,
                                                    uint16_t port,
                                                    uint16_t txtLen,
                                                    const unsigned char *txtRecord,
                                                    void *context )
{
	CServiceBrowserDlg *p = (CServiceBrowserDlg *) context;
    if ( !errorCode ) {
        auto ii = p->m_TreeInsertionMap.find( sdRef );
        if ( ii != p->m_TreeInsertionMap.end() ) {
            CString msg( _T("Host name: ") );
            msg += CA2T( hosttarget, CP_UTF8);
            HTREEITEM item = p->m_Tree.InsertItem( msg, ii->second );
            DNSServiceRef client = NULL;
			MIB_IFROW IfRow;
			IfRow.dwIndex = interfaceIndex;
			DWORD result  = GetIfEntry ( &IfRow );
			wchar_t *adapterstr = _T("Unknown");
			if ( result == 0 ) {
				adapterstr = CA2T( (char*)IfRow.bDescr );
                DNSServiceErrorType err = DNSServiceGetAddrInfo( &client,
                                                                 kDNSServiceFlagsTimeout,
                                                                 interfaceIndex,
                                                                 kDNSServiceProtocol_IPv4,
                                                                 hosttarget,
                                                                 GetAddress,
                                                                 context );
                if ( err == 0 ) {
                    p->m_ClientToFdMap[client] = DNSServiceRefSockFD(client);
                    p->m_TreeInsertionMap[client] = item;
                    CString msg;
					wchar_t *targetstr = CA2T(hosttarget, CP_UTF8);
                    msg.Format( _T("Looking up %s on %s"), targetstr, adapterstr );
                    p->m_Text.SetWindowText( msg );
                } else {
                    CString msg;
                    msg.Format( _T("Error looking up address info for %s"), CA2T( hosttarget, CP_UTF8) );
                    AfxMessageBox( msg );
                }
			}
            msg.Format( _T("Host port: %d"), port );
            p->m_Tree.InsertItem( msg, ii->second );
            msg.Format( _T("Network interface: %s"), adapterstr );
            p->m_Tree.InsertItem( msg, ii->second );
            if  ( errorCode == 0 ) {
                std::string records;
                size_t pos = 0;
                for ( ; ; ) {
                    if ( pos >= txtLen )
                        break;
                    int length = txtRecord[pos++] & 0xff;
                    if ( length == 0 )
                        break;
                    p->m_Tree.InsertItem( CA2T(std::string( txtRecord + pos, txtRecord + pos + length ).c_str(), CP_UTF8), ii->second );
                    records += "\r\n";
                   pos += length;
                }
            }
        }
        else
            AfxMessageBox( _T("???") );
    }
    if ( !(flags & kDNSServiceFlagsMoreComing ) ) {
        auto ii = p->m_ClientToFdMap.find( sdRef );
        if ( ii != p->m_ClientToFdMap.end() )
            p->m_ClientToFdMap.erase( ii );
        auto jj = p->m_TreeInsertionMap.find( sdRef );
        p->m_TreeInsertionMap.erase( jj );
        p->m_Tree.Invalidate();
    }
}
void DNSSD_API CServiceBrowserDlg::GetAddress( DNSServiceRef sdRef,
                                      DNSServiceFlags flags,
                                      uint32_t interfaceIndex,
                                      DNSServiceErrorType errorCode,
                                      const char *hostname,
                                      const struct sockaddr *address,
                                      uint32_t ttl,
                                      void *context )
{
	CServiceBrowserDlg *p = (CServiceBrowserDlg *) context;
    if ( !errorCode ) {
        auto ii = p->m_TreeInsertionMap.find( sdRef );
        if ( ii != p->m_TreeInsertionMap.end() ) {
            const sockaddr_in *in = (const sockaddr_in *) address;
            char *ip = inet_ntoa( in->sin_addr );
            HTREEITEM item = p->m_Tree.InsertItem( CA2T(ip), ii->second );
        } else
            AfxMessageBox( _T("???") );
    }
    if ( !(flags & kDNSServiceFlagsMoreComing ) ) {
        auto ii = p->m_ClientToFdMap.find( sdRef );
        if ( ii != p->m_ClientToFdMap.end() )
            p->m_ClientToFdMap.erase( ii );
        auto jj = p->m_TreeInsertionMap.find( sdRef );
        p->m_TreeInsertionMap.erase( jj );
        p->m_Tree.Invalidate();
    }
}
