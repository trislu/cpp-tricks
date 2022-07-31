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
    int Exec();
    void UpdateScript();

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
    // loop
    int sig = 0;
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
            const char *script =
                "while true do \
                    print('[lua] executing') \
                    coroutine.yield(); \
                end";
            luaL_loadstring(vm, script);
            // set JIT mode
            luaJIT_setmode(vm, 1, LUAJIT_MODE_ALLFUNC | LUAJIT_MODE_ON);
            // set script flag to false
            nscript = false;
        }
        state.store(IDLE);
        std::cout << "[c++] begin signal wait " << std::endl;
        int wret = sigwait(&mask, &sig);
        if (0 != wret)
        {
            // panic : invalid signal setup
            std::cout << "[c++] invalid signal setup = " << wret << std::endl;
            lasterr = SIGSET_INVALID;
            break;
        }
        // set state to 'SCRIPTING'
        state.store(SCRIPTING);
        // resume the script with 0 argument
        int ret = lua_resume(vm, 0);
        std::cout << "[c++] vm status = " << lua_status(vm) << std::endl;
        if (LUA_YIELD == ret)
        {
            // good script
            continue;
        }
        else
        {
            // bad script
            lasterr = (0 == ret) ? BADSCRIPT_MUST_YIELD : BADSCRIPT_RUNTIME_ERROR;
            break;
        }
    }
    state.store(INACTIVE);
}

ThreadedLuaVM::STATE ThreadedLuaVM::GetState() const
{
    return state.load();
}

int ThreadedLuaVM::Exec()
{
    /// TODO: send via message queue
    STATE s = GetState();
    std::cout << "state is:" << s << std::endl;
    if (IDLE == s)
    {
        return pthread_kill(tid, SIGUSR1);
    }
    // should never enter here
    return -1;
}

void ThreadedLuaVM::UpdateScript()
{
    // boolean value update is thread safe
    nscript = true;
}

int main(int argc, char **argv)
{
    ThreadedLuaVM *vm = ThreadedLuaVM::Create();
    for (;;)
    {
        sleep(1);
        int exec_ret = vm->Exec();
        if (-1 == exec_ret)
        {
            std::cout << "attempt exec failed, ThreadedLuaVM is running" << std::endl;
        }
        else
        {
            std::cout << "attempt exec success :" << exec_ret << std::endl;
        }
    }
    return 0;
}
