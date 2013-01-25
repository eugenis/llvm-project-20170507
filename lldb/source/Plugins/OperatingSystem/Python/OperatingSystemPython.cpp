//===-- OperatingSystemPython.cpp --------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/lldb-python.h"

#ifndef LLDB_DISABLE_PYTHON

#include "OperatingSystemPython.h"
// C Includes
// C++ Includes
// Other libraries and framework includes
#include "lldb/Core/ArchSpec.h"
#include "lldb/Core/DataBufferHeap.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/RegisterValue.h"
#include "lldb/Core/ValueObjectVariable.h"
#include "lldb/Interpreter/CommandInterpreter.h"
#include "lldb/Interpreter/PythonDataObjects.h"
#include "lldb/Symbol/ClangNamespaceDecl.h"
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Symbol/VariableList.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/StopInfo.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/ThreadList.h"
#include "lldb/Target/Thread.h"
#include "Plugins/Process/Utility/DynamicRegisterInfo.h"
#include "Plugins/Process/Utility/RegisterContextMemory.h"
#include "Plugins/Process/Utility/ThreadMemory.h"

using namespace lldb;
using namespace lldb_private;

void
OperatingSystemPython::Initialize()
{
    PluginManager::RegisterPlugin (GetPluginNameStatic(),
                                   GetPluginDescriptionStatic(),
                                   CreateInstance);
}

void
OperatingSystemPython::Terminate()
{
    PluginManager::UnregisterPlugin (CreateInstance);
}

OperatingSystem *
OperatingSystemPython::CreateInstance (Process *process, bool force)
{
    // Python OperatingSystem plug-ins must be requested by name, so force must be true
    FileSpec python_os_plugin_spec (process->GetPythonOSPluginPath());
    if (python_os_plugin_spec && python_os_plugin_spec.Exists())
    {
        std::auto_ptr<OperatingSystemPython> os_ap (new OperatingSystemPython (process, python_os_plugin_spec));
        if (os_ap.get() && os_ap->IsValid())
            return os_ap.release();
    }
    return NULL;
}


const char *
OperatingSystemPython::GetPluginNameStatic()
{
    return "python";
}

const char *
OperatingSystemPython::GetPluginDescriptionStatic()
{
    return "Operating system plug-in that gathers OS information from a python class that implements the necessary OperatingSystem functionality.";
}


OperatingSystemPython::OperatingSystemPython (lldb_private::Process *process, const FileSpec &python_module_path) :
    OperatingSystem (process),
    m_thread_list_valobj_sp (),
    m_register_info_ap (),
    m_interpreter (NULL),
    m_python_object_sp ()
{
    if (!process)
        return;
    TargetSP target_sp = process->CalculateTarget();
    if (!target_sp)
        return;
    m_interpreter = target_sp->GetDebugger().GetCommandInterpreter().GetScriptInterpreter();
    if (m_interpreter)
    {
        
        std::string os_plugin_class_name (python_module_path.GetFilename().AsCString(""));
        if (!os_plugin_class_name.empty())
        {
            const bool init_session = false;
            const bool allow_reload = true;
            char python_module_path_cstr[PATH_MAX];
            python_module_path.GetPath(python_module_path_cstr, sizeof(python_module_path_cstr));
            Error error;
            if (m_interpreter->LoadScriptingModule (python_module_path_cstr, allow_reload, init_session, error))
            {
                // Strip the ".py" extension if there is one
                size_t py_extension_pos = os_plugin_class_name.rfind(".py");
                if (py_extension_pos != std::string::npos)
                    os_plugin_class_name.erase (py_extension_pos);
                // Add ".OperatingSystemPlugIn" to the module name to get a string like "modulename.OperatingSystemPlugIn"
                os_plugin_class_name += ".OperatingSystemPlugIn";
                ScriptInterpreterObjectSP object_sp = m_interpreter->OSPlugin_CreatePluginObject(os_plugin_class_name.c_str(), process->CalculateProcess());
                if (object_sp && object_sp->GetObject())
                    m_python_object_sp = object_sp;
            }
        }
    }
}

OperatingSystemPython::~OperatingSystemPython ()
{
}

DynamicRegisterInfo *
OperatingSystemPython::GetDynamicRegisterInfo ()
{
    if (m_register_info_ap.get() == NULL)
    {
        if (!m_interpreter || !m_python_object_sp)
            return NULL;
        LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_PROCESS));
        
        if (log)
            log->Printf ("OperatingSystemPython::GetDynamicRegisterInfo() fetching thread register definitions from python for pid %" PRIu64, m_process->GetID());
        
        PythonDictionary dictionary(m_interpreter->OSPlugin_RegisterInfo(m_python_object_sp));
        if (!dictionary)
            return NULL;
        
        m_register_info_ap.reset (new DynamicRegisterInfo (dictionary));
        assert (m_register_info_ap->GetNumRegisters() > 0);
        assert (m_register_info_ap->GetNumRegisterSets() > 0);
    }
    return m_register_info_ap.get();
}

//------------------------------------------------------------------
// PluginInterface protocol
//------------------------------------------------------------------
const char *
OperatingSystemPython::GetPluginName()
{
    return "OperatingSystemPython";
}

const char *
OperatingSystemPython::GetShortPluginName()
{
    return GetPluginNameStatic();
}

uint32_t
OperatingSystemPython::GetPluginVersion()
{
    return 1;
}

bool
OperatingSystemPython::UpdateThreadList (ThreadList &old_thread_list, ThreadList &new_thread_list)
{
    if (!m_interpreter || !m_python_object_sp)
        return false;
    
    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_PROCESS));
    
    // First thing we have to do is get the API lock, and the run lock.  We're going to change the thread
    // content of the process, and we're going to use python, which requires the API lock to do it.
    // So get & hold that.  This is a recursive lock so we can grant it to any Python code called on the stack below us.
    Target &target = m_process->GetTarget();
    Mutex::Locker api_locker (target.GetAPIMutex());
    
    if (log)
        log->Printf ("OperatingSystemPython::UpdateThreadList() fetching thread data from python for pid %" PRIu64, m_process->GetID());

    PythonList threads_list(m_interpreter->OSPlugin_ThreadsInfo(m_python_object_sp));
    if (threads_list)
    {
        const uint32_t num_threads = threads_list.GetSize();
        for (uint32_t i=0; i<num_threads; ++i)
        {
            PythonDictionary thread_dict(threads_list.GetItemAtIndex(i));
            if (thread_dict)
            {
                ThreadSP thread_sp (CreateThreadFromThreadInfo (thread_dict, &old_thread_list, NULL));
                if (thread_sp)
                    new_thread_list.AddThread(thread_sp);
            }
        }
    }
    else
    {
        new_thread_list = old_thread_list;
    }
    return new_thread_list.GetSize(false) > 0;
}

ThreadSP
OperatingSystemPython::CreateThreadFromThreadInfo (PythonDictionary &thread_dict, ThreadList *old_thread_list_ptr, bool *did_create_ptr)
{
    ThreadSP thread_sp;
    if (thread_dict)
    {
        PythonString tid_pystr("tid");
        const tid_t tid = thread_dict.GetItemForKeyAsInteger (tid_pystr, LLDB_INVALID_THREAD_ID);
        if (tid != LLDB_INVALID_THREAD_ID)
        {
            PythonString name_pystr("name");
            PythonString queue_pystr("queue");
            PythonString state_pystr("state");
            PythonString stop_reason_pystr("stop_reason");
            PythonString reg_data_addr_pystr ("register_data_addr");
            
            const addr_t reg_data_addr = thread_dict.GetItemForKeyAsInteger (reg_data_addr_pystr, LLDB_INVALID_ADDRESS);
            const char *name = thread_dict.GetItemForKeyAsString (name_pystr);
            const char *queue = thread_dict.GetItemForKeyAsString (queue_pystr);
            //const char *state = thread_dict.GetItemForKeyAsString (state_pystr);
            //const char *stop_reason = thread_dict.GetItemForKeyAsString (stop_reason_pystr);
            
            if (old_thread_list_ptr)
                thread_sp = old_thread_list_ptr->FindThreadByID (tid, false);
            if (!thread_sp)
            {
                if (did_create_ptr)
                    *did_create_ptr = true;
                thread_sp.reset (new ThreadMemory (*m_process,
                                                   tid,
                                                   name,
                                                   queue,
                                                   reg_data_addr));
            }
        }
    }
    return thread_sp;
}



void
OperatingSystemPython::ThreadWasSelected (Thread *thread)
{
}

RegisterContextSP
OperatingSystemPython::CreateRegisterContextForThread (Thread *thread, addr_t reg_data_addr)
{
    RegisterContextSP reg_ctx_sp;
    if (!m_interpreter || !m_python_object_sp || !thread)
        return RegisterContextSP();
    
    // First thing we have to do is get the API lock, and the run lock.  We're going to change the thread
    // content of the process, and we're going to use python, which requires the API lock to do it.
    // So get & hold that.  This is a recursive lock so we can grant it to any Python code called on the stack below us.
    Target &target = m_process->GetTarget();
    Mutex::Locker api_locker (target.GetAPIMutex());

    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_THREAD));

    if (reg_data_addr != LLDB_INVALID_ADDRESS)
    {
        // The registers data is in contiguous memory, just create the register
        // context using the address provided
        if (log)
            log->Printf ("OperatingSystemPython::CreateRegisterContextForThread (tid = 0x%" PRIx64 ", reg_data_addr = 0x%" PRIx64 ") creating memory register context", thread->GetID(), reg_data_addr);
        reg_ctx_sp.reset (new RegisterContextMemory (*thread, 0, *GetDynamicRegisterInfo (), reg_data_addr));
    }
    else
    {
        // No register data address is provided, query the python plug-in to let
        // it make up the data as it sees fit
        if (log)
            log->Printf ("OperatingSystemPython::CreateRegisterContextForThread (tid = 0x%" PRIx64 ") fetching register data from python", thread->GetID());

        PythonString reg_context_data(m_interpreter->OSPlugin_RegisterContextData (m_python_object_sp, thread->GetID()));
        if (reg_context_data)
        {
            DataBufferSP data_sp (new DataBufferHeap (reg_context_data.GetString(),
                                                      reg_context_data.GetSize()));
            if (data_sp->GetByteSize())
            {
                RegisterContextMemory *reg_ctx_memory = new RegisterContextMemory (*thread, 0, *GetDynamicRegisterInfo (), LLDB_INVALID_ADDRESS);
                if (reg_ctx_memory)
                {
                    reg_ctx_sp.reset(reg_ctx_memory);
                    reg_ctx_memory->SetAllRegisterData (data_sp);
                }
            }
        }
    }
    return reg_ctx_sp;
}

StopInfoSP
OperatingSystemPython::CreateThreadStopReason (lldb_private::Thread *thread)
{
    // We should have gotten the thread stop info from the dictionary of data for
    // the thread in the initial call to get_thread_info(), this should have been
    // cached so we can return it here
    StopInfoSP stop_info_sp; //(StopInfo::CreateStopReasonWithSignal (*thread, SIGSTOP));
    return stop_info_sp;
}

lldb::ThreadSP
OperatingSystemPython::CreateThread (lldb::tid_t tid, addr_t context)
{
    LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_THREAD));
    
    if (log)
        log->Printf ("OperatingSystemPython::CreateThread (tid = 0x%" PRIx64 ", context = 0x%" PRIx64 ") fetching register data from python", tid, context);
    
    if (m_interpreter && m_python_object_sp)
    {
        // First thing we have to do is get the API lock, and the run lock.  We're going to change the thread
        // content of the process, and we're going to use python, which requires the API lock to do it.
        // So get & hold that.  This is a recursive lock so we can grant it to any Python code called on the stack below us.
        Target &target = m_process->GetTarget();
        Mutex::Locker api_locker (target.GetAPIMutex());
        
        PythonDictionary thread_info_dict (m_interpreter->OSPlugin_CreateThread(m_python_object_sp, tid, context));
        if (thread_info_dict)
        {
            ThreadList &thread_list = m_process->GetThreadList();
            bool did_create = false;
            ThreadSP thread_sp (CreateThreadFromThreadInfo (thread_info_dict, &thread_list, &did_create));
            if (did_create)
                thread_list.AddThread(thread_sp);
            return thread_sp;
        }
    }
    return ThreadSP();
}



#endif // #ifndef LLDB_DISABLE_PYTHON
