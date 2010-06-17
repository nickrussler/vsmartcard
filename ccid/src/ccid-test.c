/*
 * Copyright (C) 2010 Frank Morgner
 *
 * This file is part of ifdnfc.
 *
 * ifdnfc is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * ifdnfc is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ifdnfc.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <pcsclite.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winscard.h>
#include <reader.h>

int
main(int argc, char *argv[])
{
    LONG rv;
    SCARDCONTEXT hContext;
    SCARDHANDLE hCard;
    LPSTR mszReaders = NULL;
    int num;
    char *reader;
    BYTE pbSendBufferCapabilities [] = {
        0x01, /* idxFunction = GetReadersPACECapabilities */
        0x00, /* lengthInputData */
        0x00, /* lengthInputData */
    };
    BYTE pbSendBufferEstablish [] = {
        0x02, /* idxFunction = EstabishPACEChannel */
        0x00, /* lengthInputData */
        0x04, /* lengthInputData */
        0x03, /* PACE with PIN */
        0x00, /* length CHAT */
        0x00, /* length certificate description */
        0x00, /* length certificate description */
    };
    BYTE pbRecvBuffer[1024];
    DWORD dwActiveProtocol, dwRecvLength, dwReaders;

    if (argc > 1) {
        if (argc > 2 || sscanf(argv[1], "%d", &num) != 1) {
            fprintf(stderr, "Usage:  %s [reader_num]\n", argv[0]);
            exit(2);
        }
    } else
        num = 0;

    rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
    if (rv < 0)
        goto err;

    dwReaders = SCARD_AUTOALLOCATE;
    rv = SCardListReaders(hContext, NULL, (LPSTR)&mszReaders, &dwReaders);
    if (rv < 0)
        goto err;

    int l, i;
    for (reader = mszReaders, i = 0;
            dwReaders > 0;
            l = strlen(reader) + 1, dwReaders -= l, reader += l, i++) {

        if (i == num)
            break;
    }
    if (dwReaders <= 0) {
        fprintf(stderr, "Could not find reader number %d\n", num);
        rv = SCARD_E_UNKNOWN_READER;
        goto err;
    }


    rv = SCardConnect(hContext, reader, SCARD_SHARE_DIRECT, 0, &hCard,
            &dwActiveProtocol);
    if (rv < 0)
        goto err;


    rv = SCardControl(hCard, FEATURE_EXECUTE_PACE, pbSendBufferCapabilities,
            sizeof(pbSendBufferCapabilities), pbRecvBuffer,
            sizeof(pbRecvBuffer), &dwRecvLength);
    if (rv < 0)
        goto err;

    rv = SCardControl(hCard, FEATURE_EXECUTE_PACE, pbSendBufferEstablish,
            sizeof(pbSendBufferEstablish), pbRecvBuffer,
            sizeof(pbRecvBuffer), &dwRecvLength);
    if (rv < 0)
        goto err;


    rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    if (rv < 0)
        goto err;

    rv = SCardFreeMemory(hContext, mszReaders);
    if (rv < 0)
        goto err;


    exit(0);

err:
    puts(pcsc_stringify_error(rv));
    if (mszReaders)
        SCardFreeMemory(hContext, mszReaders);


    exit(1);
}