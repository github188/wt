#include "stmp_header.h"


static char base64_table[64] =
{
		'A', 'B', 'C', 'D', 'E', 'F', 'G',
		'H', 'I', 'J', 'K', 'L', 'M', 'N',
		'O', 'P', 'Q', 'R', 'S', 'T',
		'U', 'V', 'W', 'X', 'Y', 'Z',
		'a', 'b', 'c', 'd', 'e', 'f', 'g',
		'h', 'i', 'j', 'k', 'l', 'm', 'n',
		'o', 'p', 'q', 'r', 's', 't',
		'u', 'v', 'w', 'x', 'y', 'z',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'+', '/'
};

int base64_encode(unsigned char* pBase64, int nLen, char* pOutBuf, int nBufSize)
{
	int i = 0;
	int j = 0;
	int nOutStrLen = 0;

	/* nOutStrLen does not contain null terminator. */
	nOutStrLen = nLen / 3 * 4 + (0 == (nLen % 3) ? 0 : 4);
	if ( pOutBuf && nOutStrLen < nBufSize ){
		char cTmp = 0;
		for ( i = 0, j = 0; i < nLen; i += 3, j += 4 ){
			/* the first character: from the first byte. */
			pOutBuf[j] = base64_table[pBase64[i] >> 2];

			/* the second character: from the first & second byte. */
			cTmp = (char)((pBase64[i] & 0x3) << 4);
			if ( i + 1 < nLen ){
				cTmp |= ((pBase64[i + 1] & 0xf0) >> 4);
			}
			pOutBuf[j+1] = base64_table[(int)cTmp];

			/* the third character: from the second & third byte. */
			cTmp = '=';
			if ( i + 1 < nLen ){
				cTmp = (char)((pBase64[i + 1] & 0xf) << 2);
				if ( i + 2 < nLen ){
					cTmp |= (pBase64[i + 2] >> 6);
				}
				cTmp = base64_table[(int)cTmp];
			}
			pOutBuf[j + 2] = cTmp;

			/* the fourth character: from the third byte. */
			cTmp = '=';
			if ( i + 2 < nLen ){
				cTmp = base64_table[pBase64[i + 2] & 0x3f];
			}
			pOutBuf[j + 3] = cTmp;
		}

		pOutBuf[j] = '\0';
	}
	return nOutStrLen + 1;
}
