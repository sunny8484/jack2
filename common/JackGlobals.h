/*
Copyright (C) 2004-2008 Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __JackGlobals__
#define __JackGlobals__

#include "JackError.h"
#include "JackThread.h"
#include "JackThread.h"
#include "JackExports.h"

namespace Jack
{

class JackSynchro;
class JackServerNotifyChannelInterface;
class JackClientChannelInterface;
class JackNotifyChannelInterface;
class JackServerChannelInterface;
class JackSyncInterface;
class JackThread;
class JackDriverClientInterface;
class JackRunnableInterface;
class JackEngine;

/*!
\brief Factory description

\todo possibly use in a dynamic way to test different communication/synchro implementations.
*/

class JackFactoryImpl
{
    public:

        JackFactoryImpl()
        {}
        virtual ~JackFactoryImpl()
        {}

        virtual JackSynchro* MakeSynchro() = 0;
        virtual JackServerNotifyChannelInterface* MakeServerNotifyChannel() = 0;
        virtual JackClientChannelInterface* MakeClientChannel() = 0;
        virtual JackNotifyChannelInterface* MakeNotifyChannel() = 0;
        virtual JackServerChannelInterface* MakeServerChannel() = 0;
        virtual JackSyncInterface* MakeInterProcessSync() = 0;
        virtual JackThread* MakeThread(JackRunnableInterface* runnable) = 0;
};

#ifdef __linux__

class JackFactoryLinuxServer : public JackFactoryImpl
{
    public:

        JackFactoryLinuxServer()
        {}
        virtual ~JackFactoryLinuxServer()
        {}

        JackSynchro* MakeSynchro();
        JackServerNotifyChannelInterface* MakeServerNotifyChannel();
        JackClientChannelInterface* MakeClientChannel();
        JackNotifyChannelInterface* MakeNotifyChannel();
        JackServerChannelInterface* MakeServerChannel();
        JackSyncInterface* MakeInterProcessSync();
        JackThread* MakeThread(JackRunnableInterface* runnable);
};

class JackFactoryLinuxClient : public JackFactoryImpl
{
    public:

        JackFactoryLinuxClient()
        {}
        virtual ~JackFactoryLinuxClient()
        {}

        JackSynchro* MakeSynchro();
        JackServerNotifyChannelInterface* MakeServerNotifyChannel();
        JackClientChannelInterface* MakeClientChannel();
        JackNotifyChannelInterface* MakeNotifyChannel();
        JackServerChannelInterface* MakeServerChannel();
        JackSyncInterface* MakeInterProcessSync();
        JackThread* MakeThread(JackRunnableInterface* runnable);
};

#endif

#ifdef WIN32

class JackFactoryWindowsServer : public JackFactoryImpl
{
    public:

        JackFactoryWindowsServer()
        {}
        virtual ~JackFactoryWindowsServer()
        {}

        JackSynchro* MakeSynchro();
        JackServerNotifyChannelInterface* MakeServerNotifyChannel();
        JackClientChannelInterface* MakeClientChannel();
        JackNotifyChannelInterface* MakeNotifyChannel();
        JackServerChannelInterface* MakeServerChannel();
        JackSyncInterface* MakeInterProcessSync();
        JackThread* MakeThread(JackRunnableInterface* runnable);
};

class JackFactoryWindowsClient : public JackFactoryImpl
{
    public:

        JackFactoryWindowsClient()
        {}
        virtual ~JackFactoryWindowsClient()
        {}

        JackSynchro* MakeSynchro();
        JackServerNotifyChannelInterface* MakeServerNotifyChannel();
        JackClientChannelInterface* MakeClientChannel();
        JackNotifyChannelInterface* MakeNotifyChannel();
        JackServerChannelInterface* MakeServerChannel();
        JackSyncInterface* MakeInterProcessSync();
        JackThread* MakeThread(JackRunnableInterface* runnable);
};

#endif

#if defined(__APPLE__)

class JackFactoryOSXServer : public JackFactoryImpl
{
    public:

        JackFactoryOSXServer()
        {}
        virtual ~JackFactoryOSXServer()
        {}

        JackSynchro* MakeSynchro();
        JackServerNotifyChannelInterface* MakeServerNotifyChannel();
        JackClientChannelInterface* MakeClientChannel();
        JackNotifyChannelInterface* MakeNotifyChannel();
        JackServerChannelInterface* MakeServerChannel();
        JackSyncInterface* MakeInterProcessSync();
        JackThread* MakeThread(JackRunnableInterface* runnable);
};

class JackFactoryOSXClient : public JackFactoryImpl
{
    public:

        JackFactoryOSXClient()
        {}
        virtual ~JackFactoryOSXClient()
        {}

        JackSynchro* MakeSynchro();
        JackServerNotifyChannelInterface* MakeServerNotifyChannel();
        JackClientChannelInterface* MakeClientChannel();
        JackNotifyChannelInterface* MakeNotifyChannel();
        JackServerChannelInterface* MakeServerChannel();
        JackSyncInterface* MakeInterProcessSync();
        JackThread* MakeThread(JackRunnableInterface* runnable);
};

#endif

/*!
\brief Factory for OS specific ressources.
*/

class JackGlobals
{
    private:

        static JackFactoryImpl* fInstance;

    public:

        JackGlobals()
        {}
        virtual ~JackGlobals()
        {}

        static JackSynchro* MakeSynchro()
        {
            return fInstance->MakeSynchro();
        }
        static JackServerNotifyChannelInterface* MakeServerNotifyChannel()
        {
            return fInstance->MakeServerNotifyChannel();
        }
        static JackClientChannelInterface* MakeClientChannel()
        {
            return fInstance->MakeClientChannel();
        }
        static JackNotifyChannelInterface* MakeNotifyChannel()
        {
            return fInstance->MakeNotifyChannel();
        }
        static JackServerChannelInterface* MakeServerChannel()
        {
            return fInstance->MakeServerChannel();
        }
        static JackSyncInterface* MakeInterProcessSync()
        {
            return fInstance->MakeInterProcessSync();
        }
        static JackThread* MakeThread(JackRunnableInterface* runnable)
        {
            return fInstance->MakeThread(runnable);
        }

        static void InitServer();
   
        static void InitClient();
    
        static void Destroy();
 
};

namespace detail
{
struct JackGlobalsServerInitializer
{
        JackGlobalsServerInitializer(void)
        {
            JackGlobals::InitServer();
        }

        ~JackGlobalsServerInitializer(void)
        {
            JackGlobals::Destroy();
        }
};
}

} // end of namespace

#ifdef __cplusplus
extern "C"
{
#endif

extern jack_tls_key gRealTime;
extern jack_tls_key g_key_log_function;

#ifdef WIN32

EXPORT void jack_init();
EXPORT void jack_uninit();

#else

void __attribute__ ((constructor)) jack_init();
void __attribute__ ((destructor)) jack_uninit();

#endif

#ifdef __cplusplus
}
#endif

#endif
