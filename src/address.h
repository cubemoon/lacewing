
/* vim: set et ts=4 sw=4 ft=cpp:
 *
 * Copyright (C) 2011 James McLaughlin.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

struct lw_addr
{
    lw_thread resolver_thread;

    char * hostname, * hostname_to_free;
    char service [64]; /* port or service name */

    int hints;

    struct addrinfo * info_list, * info, * info_to_free;

    lw_error error;

    char buffer [64]; /* for to_string */
};

/*struct AddressWrapper : public Address::Internal
{
    char AddressBytes [sizeof (Lacewing::Address)];

    sockaddr_storage SockAddr;
    addrinfo info;

    inline AddressWrapper ()
    {
        Lacewing::Address * address = (Lacewing::Address *) AddressBytes;

        address->Tag = 0;
        address->internal = this;

        memset (&info, 0, sizeof (info));

        info.ai_addr = (sockaddr *) &SockAddr;

        Info = &info;       
    }

    inline void Set (sockaddr_storage * s)
    {
        memcpy (&SockAddr, s, sizeof(sockaddr_storage));

        if ((info.ai_family = s->ss_family) == AF_INET6)
            info.ai_addrlen = sizeof (sockaddr_in6);
        else
            info.ai_addrlen = sizeof (sockaddr_in);
    }

    inline operator Lacewing::Address & ()
    {
        return *(Lacewing::Address *) AddressBytes;
    }
};*/

