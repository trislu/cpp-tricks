
/*
  Copyright (c) 2021 Lu Kai
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <atomic>
#include <iostream>
#include <lua.hpp>

class ThreadedLuaVM
{
public:
    static ThreadedLuaVM *Create();
    typedef enum
    {
        INACTIVE,
        IDLE,
        SCRIPTING,
    } STATE;
    STATE GetState() const;
    int ResumeThread();
    void ReloadScript();

private:
    pthread_t tid;
    bool nscript;
    int lasterr;
    sigset_t mask;
    std::atomic<STATE> state;
    ThreadedLuaVM(const ThreadedLuaVM &);
    ThreadedLuaVM &operator=(const ThreadedLuaVM &);
    ThreadedLuaVM();
    static void *run(void *);
    void loop();
    /* define error codes */
#ifndef ENUMERATE_ERRORS
#define ENUMERATE_ERRORS                                                                 \
    DECLARE_ERROR(VM_CREAT_FAILD, "failed to create lua vm")                             \
    DECLARE_ERROR(LOAD_SCRIPT_FAILD, "failed to load lua script")                        \
    DECLARE_ERROR(SIGSET_INVALID, "signal set contains an invalid signal number")        \
    DECLARE_ERROR(BADSCRIPT_MUST_YIELD, "bad script : must return to C side from yield") \
    DECLARE_ERROR(BADSCRIPT_RUNTIME_ERROR, "bad script : lua runtime error")             \
    /* you may add a new error above */
private:
    enum ERR_CODE
    {
        ERR_NONE = 0,
#define DECLARE_ERROR(errcode, errstring) errcode,
        ENUMERATE_ERRORS
#undef DECLARE_ERROR
            ERR_AMOUNT,
    };
public:
    const char *LastError() const
    {
        static const char *errmap[] = {
            "none",
#define DECLARE_ERROR(errcode, errstring) errstring,
            ENUMERATE_ERRORS
#undef DECLARE_ERROR
        };
        if ((lasterr < 0) || (lasterr >= ERR_AMOUNT))
        {
            return NULL;
        }
        return errmap[lasterr];
    }
#undef ENUMERATE_ERRORS
#endif
};

ThreadedLuaVM::ThreadedLuaVM() : tid(),
                                 nscript(true),
                                 lasterr(ERR_NONE),
                                 mask(),
                                 state(INACTIVE)
{
}

ThreadedLuaVM *ThreadedLuaVM::Create()
{
    // create new ThreadedLuaVM
    ThreadedLuaVM *vm = new ThreadedLuaVM();
    // the running thread need to be detached
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    // manage singal set
    sigemptyset(&vm->mask);
    sigaddset(&vm->mask, SIGUSR1);
    sigaddset(&vm->mask, SIGUSR2);
    pthread_sigmask(SIG_SETMASK, &vm->mask, NULL);
    // create running thread
    int creat_ret = pthread_create(&vm->tid, &attr, run, vm);
    assert(0 == creat_ret);
    // destory attr
    pthread_attr_destroy(&attr);
    return vm;
}

void *ThreadedLuaVM::run(void *arg)
{
    ThreadedLuaVM *vm = (ThreadedLuaVM *)arg;
    vm->loop();
    return NULL;
}

void dump_vm_stack(lua_State *L)
{
    std::cout << "[stack bottom]" << std::endl;
    for (int i = 1; i <= lua_gettop(L); i++)
    {
        std::cout << lua_typename(L, lua_type(L, i)) << std::endl;
    }
    std::cout << "[stack top]" << std::endl;
}

void ThreadedLuaVM::loop()
{
    // lua vm of this thread
    lua_State *vm = NULL;
    for (;;)
    {
        // even if script manager may update 'nscript' from another thread, but its fine
        if (nscript)
        {
            // close previous vm
            if (vm)
                lua_close(vm);
            // create new vm
            vm = luaL_newstate();
            if (NULL == vm)
            {
                // panic : vm creation failed
                lasterr = VM_CREAT_FAILD;
                break;
            }
            // open lua basic libraries
            luaL_openlibs(vm);
            /// TODO: create sandbox
            // script
            int ret = luaL_loadfile(vm, "script.lua");
            std::cout << "[c++ vm] load lua script: " << ret << std::endl;
            // set JIT mode
            luaJIT_setmode(vm, 1, LUAJIT_MODE_ALLFUNC | LUAJIT_MODE_ON);
            // set script flag to false
            nscript = false;
        }
        state.store(IDLE);
        std::cout << "[c++] wait for resume/reload signal " << std::endl;
        siginfo_t info;
        int wret = sigwaitinfo(&mask, &info);
        if (info.si_pid != getpid())
        {
            std::cout << "[c++ vm] ignore signal from another process" << std::endl;
            continue;
        }
        else if (SIGUSR2 == info.si_signo)
        {
            // panic : invalid signal setup
            std::cout << "[c++ vm] recv reload signal " << std::endl;
            continue;
        }
        std::cout << "[c++ vm] recv resume signal " << std::endl;
        if (-1 == wret)
        {
            // panic : invalid signal setup
            std::cout << "[c++ vm] sigwaitinfo error = " << errno << std::endl;
            continue;
        }
        // set state to 'SCRIPTING'
        state.store(SCRIPTING);
        // resume the script with 0 argument
        int ret = lua_resume(vm, 0);
        if (LUA_YIELD == ret)
        {
            // good script
            continue;
        }
        else
        {
            // bad script
            if (0 == ret)
            {
                lasterr = BADSCRIPT_MUST_YIELD;
                std::cout << "[c++] " << LastError() << std::endl;
            }
            else
            {
                lasterr = BADSCRIPT_RUNTIME_ERROR;
                std::cout << "[c++] " << LastError() << std::endl;
                std::cout << "lua runtime error :" << std::endl
                          << lua_tostring(vm, lua_gettop(vm)) << std::endl;
            }
            break;
        }
    }
    state.store(INACTIVE);
}

ThreadedLuaVM::STATE ThreadedLuaVM::GetState() const
{
    return state.load();
}

int ThreadedLuaVM::ResumeThread()
{
    /// TODO: send via message queue
    STATE s = GetState();
    if (IDLE == s)
    {
        return pthread_kill(tid, SIGUSR1);
    }
    // should never enter here
    return -1;
}

void ThreadedLuaVM::ReloadScript()
{
    // boolean value update is thread safe
    nscript = true;
    pthread_kill(tid, SIGUSR2);
}

/* main area */

static volatile sig_atomic_t run = 1;

static void sigint_handler(int sig)
{
    (void)sig;
    run = 0;
    fclose(stdin);
}

int main(int argc, char **argv)
{
    ThreadedLuaVM *vm = ThreadedLuaVM::Create();
    signal(SIGINT, sigint_handler);

    const char *helpmsg = "%% Type '?' then Enter to print this message once again \n"
                          "%% Type 'c' then Enter to resume the Lua VM thread \n"
                          "%% Type 'r' then Enter to reload the Lua script \n"
                          "%% Press Ctrl-C or Ctrl-D to exit\n";

    std::cerr << helpmsg << std::endl;
    char buf[128];
    while (run && fgets(buf, sizeof(buf), stdin))
    {
        size_t len = strlen(buf);
        // remove newline
        if ('\n' == buf[len - 1])
            buf[--len] = '\0';

        if (1 == len)
        {
            switch (buf[0])
            {
            case '?':
                // print once again
                std::cerr << helpmsg << std::endl;
                break;
            case 'c':
                vm->ResumeThread();
                break;
            case 'r':
                vm->ReloadScript();
                break;
            default:
                std::cerr << "[c++ main] unkown option :" << buf << std::endl;
                break;
            }
        }
        else
        {
            std::cerr << "[c++ main] option string too " << (len ? "long" : "short") << ": " << buf << std::endl;
        }
    }
    return 0;
}
