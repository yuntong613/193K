#include "StdAfx.h"
#include "common.h"
#include "YSerialDevice.h"
#include "IniFile.h"
#include "ItemBrowseDlg.h"
#include "YSerialItem.h"
#include <cstringt.h>
#include "ModelDll.h"
#include "OPCIniFile.h"

extern CModelDllApp theApp;

DWORD CALLBACK QuertyThread(LPVOID pParam)
{
	YSerialDevice* pDevice = (YSerialDevice*)pParam;
	while(!pDevice->m_bStop)
	{
		Sleep(1000);
		pDevice->QueryOnce();
	}
	return 0;
}


YSerialDevice::YSerialDevice(LPCSTR pszAppPath)
: m_nBaudRate(0)
, m_nComPort(0)
, m_bStop(true)
{
	m_nParity = 0;
	m_hQueryThread = INVALID_HANDLE_VALUE;
	y_lUpdateTimer = 0;
	m_nUseLog = 0;
	CString strConfigFile(pszAppPath);
	strConfigFile+= _T("\\ComFile.ini");
	if(!InitConfig(strConfigFile))
	{
		return;
	}

	CString strListItemsFile(pszAppPath);
	strListItemsFile += _T("\\ListItems.ini");
	COPCIniFile opcFile;
	if (!opcFile.Open(strListItemsFile,CFile::modeRead|CFile::typeText))
	{
		AfxMessageBox("Can't open INI file!");
		return;
	}
	CArchive ar(&opcFile,CArchive::load);
	Serialize(ar);
	opcFile.Close();
}

YSerialDevice::~YSerialDevice(void)
{
	POSITION pos = m_ItemsArray.GetStartPosition();
	YSerialItem* pItem = NULL;
	CString strItemName;
	while(pos){
		m_ItemsArray.GetNextAssoc(pos,strItemName,(CObject*&)pItem);
		if(pItem)
		{
			delete pItem;
			pItem = NULL;
		}
	}
	m_ItemsArray.RemoveAll();
}

void YSerialDevice::Serialize(CArchive& ar)
{
	if (ar.IsStoring()){
	}else{
		Load(ar);
	}
}

BOOL YSerialDevice::InitConfig(CString strFilePath)
{
	if(!PathFileExists(strFilePath))
		return FALSE;
	CIniFile iniFile(strFilePath);
	m_lRate = iniFile.GetUInt("param","UpdateRate",3000);
	m_nUseLog = iniFile.GetUInt("param","Log",0);

	m_nComPort = iniFile.GetUInt("ComInfo","ComPort",1);
	m_nBaudRate = iniFile.GetUInt("ComInfo","BaudRate",9600);
	m_nParity = iniFile.GetUInt("ComInfo","Parity",0);

	return TRUE;
}

void YSerialDevice::Load(CArchive& ar)
{
	LoadItems(ar);
}

void YSerialDevice::LoadItems(CArchive& ar)
{
	COPCIniFile* pIniFile = static_cast<COPCIniFile*>(ar.GetFile());
	YOPCItem* pItem  = NULL;
	int nItems = 0;
	CString strTmp("Item");
	CString strItemName;
	CString strItemDesc;
	CString strValue;
	DWORD dwItemPId = 0L;
	strTmp+=CString(_T("List"));
	if(pIniFile->ReadNoSeqSection(strTmp)){
		nItems = pIniFile->GetItemsCount(strTmp,"Item");
		for (int i=0;i<nItems && !pIniFile->Endof();i++ )
		{
			try{
				if (pIniFile->ReadIniItem("Item",strTmp))
				{
					if (!pIniFile->ExtractSubValue(strTmp,strValue,1))
						throw new CItemException(CItemException::invalidId,pIniFile->GetFileName());
					dwItemPId = atoi(strValue);
					if(!pIniFile->ExtractSubValue(strTmp,strItemName,2))strItemName = _T("Unknown");
					if(!pIniFile->ExtractSubValue(strTmp,strItemDesc,3)) strItemDesc = _T("Unknown");
					pItem = new YShortItem(dwItemPId,strItemName,strItemDesc);
					if(GetItemByName(strItemName))
						delete pItem;
					else 
						m_ItemsArray.SetAt(pItem->GetName(),(CObject*)pItem);
				}
			}
			catch(CItemException* e){
				if(pItem) delete pItem;
				e->Delete();
			}
		}
	}
}

void YSerialDevice::OnUpdate()
{
// 	y_lUpdateTimer--;
// 	if(y_lUpdateTimer>0)return;
// 	y_lUpdateTimer = m_lRate/1000;

}

int YSerialDevice::QueryOnce()
{
// 	y_lUpdateTimer--;
// 	if(y_lUpdateTimer>0)return;
// 	y_lUpdateTimer = m_lRate/1000;

	return 0;
}

void YSerialDevice::BeginUpdateThread()
{
	bool bOpen = m_Com.Open(m_nComPort, m_nBaudRate, m_nParity);
	if (!bOpen)
	{
		AfxMessageBox("串口打开失败");
	}
}

void YSerialDevice::EndUpdateThread()
{
	m_Com.Close();
}

BYTE YSerialDevice::Hex2Bin(CString strHex)
{
	int iDec = 0;
	if(strHex.GetLength() == 2){
		char cCh = strHex[0];
		if((cCh >= '0') && (cCh <= '9'))iDec = cCh - '0';
		else if((cCh >= 'A') && (cCh <= 'F'))iDec = cCh - 'A' + 10;
		else if((cCh >= 'a') && (cCh <= 'f'))iDec = cCh - 'a' + 10;
		else return 0;
		iDec *= 16;
		cCh = strHex[1];
		if((cCh >= '0') && (cCh <= '9'))iDec += cCh - '0';
		else if((cCh >= 'A') && (cCh <= 'F'))iDec += cCh - 'A' + 10;
		else if((cCh >= 'a') && (cCh <= 'f'))iDec += cCh - 'a' + 10;
		else return 0;
	}
	return iDec & 0xff;
}

int YSerialDevice::HexStr2Bin(BYTE * cpData,CString strData)
{
	CString strByte;
	for(int i=0;i<strData.GetLength();i+=2){
		strByte = strData.Mid(i,2);
		cpData[i/2] = Hex2Bin(strByte);
	}
	return strData.GetLength() / 2;
}

CString YSerialDevice::Bin2HexStr(BYTE* cpData,int nLen)
{
	CString strResult,strTemp;
	for (int i=0;i<nLen;i++)
	{
		strTemp.Format("%02X ",cpData[i]);
		strResult+=strTemp;
	}
	return strResult;
}

bool YSerialDevice::CheckSum(BYTE* cpData, int nLen)
{	
	CByteArray byteData;
	for (int i = 1; i < nLen - 1; i+=2)
	{
		byteData.Add(((cpData[i] & 0x0F)<< 4) | ((cpData[i+1]) & 0x0F));
	}
	int nSum = 0;
	for (int k = 0;k<byteData.GetCount()-1;k++)
	{
		nSum += byteData[k];
	}

	BYTE bCheck = nSum & 0xFF;

	CString strHex = Bin2HexStr(byteData.GetData(), byteData.GetCount());

	return false;
}

void YSerialDevice::HandleData()
{
	if (m_Com.IsOpen())
	{
		YOPCItem* pItem = NULL;
		BYTE cpData[1024] = { 0 };
		DWORD dwRead = m_Com.Read(cpData, 1024, 1000);
		OutPutLog("[接收] " + Bin2HexStr(cpData, dwRead));
		if (dwRead == 26)
		{
			if ((cpData[0] == 0x82) && (cpData[dwRead-1] == 0x83))
			{
				char szData[100] = { 0 };
				memcpy(szData, cpData + 1, dwRead - 2);
				CString szText = szData;
				CString strDataType = szText.Left(2);
				CString strValue;
				switch (atoi(strDataType))
				{
				case 80: //火警
					strValue = "1";
					break;
				case 81: //故障
					strValue = "2";
					break;
				case 1: //正常
				case 9: //复原
					strValue = "0";
					break;
				case 82: //故障恢复
					strValue = "0";
					break;
				default:
					break;
				}
				szText.Delete(0, 2);
				CString strItemCode = szText.Left(6);
				pItem = GetItemByName(strItemCode);
				if (pItem)
				{
					pItem->OnUpdate(strValue);
				}
// 				if (CheckSum(cpData, dwRead))
// 				{
// 
// 				}
				int K = 0;
			}
		}
	}

	return;
}

bool YSerialDevice::SetDeviceItemValue(CBaseItem* pAppItem)
{
	return false;
}

void YSerialDevice::OutPutLog(CString strMsg)
{
	if(m_nUseLog)
		m_Log.Write(strMsg);
}
