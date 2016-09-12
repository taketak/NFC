#include <windows.h>
#include <winscard.h>
#include <stdio.h>
#include <tchar.h>
#include <assert.h>


#include <iostream>

#pragma comment (lib, "winscard.lib")//SCard���C�u�����C���N���[�h

#define TEST_TITLE		_T("GET DATA (UID)")
#define PCSC_RECV_BUFF_LEN	(262)

class SCard
{
	//����
	//�b��e�X�g�p
protected:
	SCARDCONTEXT	hContext;//SCard�R���e�L�X�g
	LONG lResult;//���ʎ��[�ϐ�
	LPTSTR	lpszReaderName;//�f�o�C�X��
	SCARDHANDLE	hCard;//�J�[�h�n���h��

	SCARD_IO_REQUEST *CardProtocol2PCI(DWORD dwProtocol)//�v���g�R���^�C�v��Ԃ�
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
		else if (dwProtocol == SCARD_PROTOCOL_UNDEFINED) {//�v���g�R������`
			assert(false);
			return NULL;
		}

		return (SCARD_IO_REQUEST *)SCARD_PCI_T1;
	}

public:
	SCard() {};

	~SCard() {
		//�������
		SCard_Release();
	};

	//SCard�����B����������0��Ԃ��B���s�����炻��ȊO
	int	SCard_ready() {
		_ftprintf_s(stdout, _T("%s\n"), TEST_TITLE);
		hContext = 0;//SC�R���e�L�X�g������
		lResult = ::SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &hContext);
		if (lResult != SCARD_S_SUCCESS) {
			if (lResult == SCARD_E_NO_SERVICE) {//�h���C�o�`�F�b�N
				_ftprintf_s(stdout, _T("Smart Card Servise is not Started.\n"));//�h���C�o�Ȃ�
			}
			else {
				_ftprintf_s(stdout, _T("SCardEstablishContext Error.\nErrorCode %08X\n"), lResult);//����ȊO�̃G���[�F�R���e�L�X�g�擾���s�Ƃ�
			}
			return EXIT_FAILURE;//�A��
		}

		lpszReaderName = NULL;//�f�o�C�X���ϐ�
		DWORD	dwAutoAllocate = SCARD_AUTOALLOCATE;
		 //���[�_�[�f�o�C�X�`�F�b�N
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
	//SCard�����{��
	VOID SCard_Main() {

		hCard = NULL;
		DWORD		dwActiveProtocol = 0;

		//���[�_�[�ƃJ�[�h�Ƃ̐ڑ����m��
		lResult = ::SCardConnect(hContext, lpszReaderName, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
		if (lResult != SCARD_S_SUCCESS) {
			if (lResult == SCARD_W_REMOVED_CARD) {
				_ftprintf_s(stdout, "�J�[�h���Ȃ�\n");
			}
			else {
				_ftprintf_s(stdout, _T(" �J�[�h���Ȃ����ǃG���[�R�[�h�͂���\nErrorCode %08X\n"), lResult);
			}
			//�G���[�������Ă������B����̓��[�v�D��
		}
		else
		{
			//����
			//���M����o�C�i���f�[�^
			//�����̃R�}���h�ς���ƃ��[�h�A���C�g�ł���
			BYTE	bSendCommand[] = { 0xFF, 0xB0, 0x00, 0x05, 0x10 };

			BYTE	bRecvBuf[PCSC_RECV_BUFF_LEN] = { 0x00 };//�ԐM�f�[�^�i�[�z��
			DWORD	dwResponseSize = sizeof(bRecvBuf);

			//�J�[�h�̏����擾
			lResult = ::SCardTransmit(hCard, CardProtocol2PCI(dwActiveProtocol), bSendCommand, sizeof(bSendCommand), NULL, bRecvBuf, &dwResponseSize);
			if (lResult != SCARD_S_SUCCESS) {//�J�[�h���擾����
				_ftprintf_s(stdout, "�J�[�h���擾���s\n");
			}
			else
			{
				//��M�f�[�^���������ׂ�
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
	//�J�������F�X
	void SCard_Release() {
		::SCardDisconnect(hCard, SCARD_LEAVE_CARD);//���[�_�[�ڑ�����
		::SCardFreeMemory(hContext, lpszReaderName);//�������[�J��
		::SCardReleaseContext(hContext);//�R���e�L�X�g�J��
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

