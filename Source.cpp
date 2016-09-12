#include <windows.h>
#include <winscard.h>
#include <stdio.h>
#include <tchar.h>
#include <assert.h>


#include <iostream>

#pragma comment (lib, "winscard.lib")//SCardライブラリインクルード

#define TEST_TITLE		_T("GET DATA (UID)")
#define PCSC_RECV_BUFF_LEN	(262)

class SCard
{
	//メモ
	//暫定テスト用
protected:
	SCARDCONTEXT	hContext;//SCardコンテキスト
	LONG lResult;//結果収納変数
	LPTSTR	lpszReaderName;//デバイス名
	SCARDHANDLE	hCard;//カードハンドル

	SCARD_IO_REQUEST *CardProtocol2PCI(DWORD dwProtocol)//プロトコルタイプを返す
	{
		if (dwProtocol == SCARD_PROTOCOL_T0) {
			return (SCARD_IO_REQUEST *)SCARD_PCI_T0;
		}
		else if (dwProtocol == SCARD_PROTOCOL_T1) {
			return (SCARD_IO_REQUEST *)SCARD_PCI_T1;
		}
		else if (dwProtocol == SCARD_PROTOCOL_RAW) {
			return (SCARD_IO_REQUEST *)SCARD_PCI_RAW;
		}
		else if (dwProtocol == SCARD_PROTOCOL_UNDEFINED) {//プロトコル未定義
			assert(false);
			return NULL;
		}

		return (SCARD_IO_REQUEST *)SCARD_PCI_T1;
	}

public:
	SCard() {};

	~SCard() {
		//解放処理
		SCard_Release();
	};

	//SCard準備。成功したら0を返す。失敗したらそれ以外
	int	SCard_ready() {
		_ftprintf_s(stdout, _T("%s\n"), TEST_TITLE);
		hContext = 0;//SCコンテキスト初期化
		lResult = ::SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &hContext);
		if (lResult != SCARD_S_SUCCESS) {
			if (lResult == SCARD_E_NO_SERVICE) {//ドライバチェック
				_ftprintf_s(stdout, _T("Smart Card Servise is not Started.\n"));//ドライバなし
			}
			else {
				_ftprintf_s(stdout, _T("SCardEstablishContext Error.\nErrorCode %08X\n"), lResult);//それ以外のエラー：コンテキスト取得失敗とか
			}
			return EXIT_FAILURE;//帰る
		}

		lpszReaderName = NULL;//デバイス名変数
		DWORD	dwAutoAllocate = SCARD_AUTOALLOCATE;
		 //リーダーデバイスチェック
		lResult = ::SCardListReaders(hContext, NULL, (LPTSTR)&lpszReaderName, &dwAutoAllocate);
		if (lResult != SCARD_S_SUCCESS) {
			if (lResult == SCARD_E_NO_READERS_AVAILABLE) {
				_ftprintf_s(stdout, _T("Reader/Writer is not Found.\n"));
			}
			else {
				_ftprintf_s(stdout, _T("SCardListReaders Error.\nErrorCode %08X\n"), lResult);
			}
			::SCardReleaseContext(hContext);
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	}
	//SCard処理本体
	VOID SCard_Main() {

		hCard = NULL;
		DWORD		dwActiveProtocol = 0;

		//リーダーとカードとの接続を確立
		lResult = ::SCardConnect(hContext, lpszReaderName, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
		if (lResult != SCARD_S_SUCCESS) {
			if (lResult == SCARD_W_REMOVED_CARD) {
				_ftprintf_s(stdout, "カードがない\n");
			}
			else {
				_ftprintf_s(stdout, _T(" カードがないけどエラーコードはある\nErrorCode %08X\n"), lResult);
			}
			//エラー処理してもいい。今回はループ優先
		}
		else
		{
			//メモ
			//送信するバイナリデータ
			//ここのコマンド変えるとリード、ライトできる
			BYTE	bSendCommand[] = { 0xFF, 0xB0, 0x00, 0x05, 0x10 };

			BYTE	bRecvBuf[PCSC_RECV_BUFF_LEN] = { 0x00 };//返信データ格納配列
			DWORD	dwResponseSize = sizeof(bRecvBuf);

			//カードの情報を取得
			lResult = ::SCardTransmit(hCard, CardProtocol2PCI(dwActiveProtocol), bSendCommand, sizeof(bSendCommand), NULL, bRecvBuf, &dwResponseSize);
			if (lResult != SCARD_S_SUCCESS) {//カード情報取得判定
				_ftprintf_s(stdout, "カード情報取得失敗\n");
			}
			else
			{
				//受信データを書き並べる
				for (UINT uiRespIdx = 0; uiRespIdx < dwResponseSize; uiRespIdx++) {
					_ftprintf_s(stdout, _T("%02X"), bRecvBuf[uiRespIdx]);
					if ((uiRespIdx + 1) >= dwResponseSize) {
						_ftprintf_s(stdout, _T("\n"));
					}
					else {
						_ftprintf_s(stdout, _T(" "));
					}
				}
			}
		}
	}
	//開放処理色々
	void SCard_Release() {
		::SCardDisconnect(hCard, SCARD_LEAVE_CARD);//リーダー接続解除
		::SCardFreeMemory(hContext, lpszReaderName);//メモリー開放
		::SCardReleaseContext(hContext);//コンテキスト開放
	}
};

int main(int argc, _TCHAR* argv[])
{
	SCard* card = new SCard();
	if (card->SCard_ready() != 0)
	{
		return EXIT_FAILURE;
	};

	while (true)
	{
		card->SCard_Main();
		Sleep(1000);
	}
	return EXIT_SUCCESS;
}

