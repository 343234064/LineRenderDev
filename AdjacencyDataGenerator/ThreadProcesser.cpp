#include "ThreadProcesser.h"
#include <iostream>
#include <intrin.h>
#include <utility>

template <typename GuardObject>
class LockGuard
{
public:
	explicit
		LockGuard(GuardObject& InObjRef) :
		ObjRef(InObjRef)
	{
		ObjRef.Lock();
	}

	~LockGuard()
	{
		ObjRef.UnLock();
	}

	LockGuard(const LockGuard& Other) = delete;
	LockGuard& operator=(const LockGuard&) = delete;

private:
	LockGuard() {}

private:
	GuardObject& ObjRef;
};

Thread* Thread::Create(Runnable* ObjectToRun,
	UINT32 InitStackSize,
	ThreadPriority InitPriority,
	UINT64 AffinityMask)
{

	Thread* NewThread = nullptr;
	NewThread = WindowsThread::CreateThread();

	if (NewThread)
	{
		if (!NewThread->PlatformInit(ObjectToRun, InitStackSize, InitPriority, AffinityMask))
		{
			delete NewThread;
			NewThread = nullptr;

		}
	}

	return NewThread;
}



bool WindowsThread::PlatformInit(Runnable* ObjectToRun,
	UINT32 InitStackSize,
	ThreadPriority InitPriority,
	UINT64 AffinityMask)
{
	static bool SetMainThreadPri = false;
	if (!SetMainThreadPri)
	{
		SetMainThreadPri = true;
		::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	}

	RunObject = ObjectToRun;
	ThreadAffinityMask = AffinityMask;


	//Create auto reset sync event
	SyncEvent = ::CreateEvent(NULL, false, 0, nullptr);

	ThreadHandle = ::CreateThread(NULL, InitStackSize, ThreadEntrance, this, STACK_SIZE_PARAM_IS_A_RESERVATION | CREATE_SUSPENDED, (DWORD*)&ThreadID);

	if (ThreadHandle == NULL)
	{
		RunObject = nullptr;
	}
	else
	{
		::SetThreadDescription(ThreadHandle, L"FacialShadowMapGeneratorThread");

		::ResumeThread(ThreadHandle);

		//Here will wait for Runnable's Init() finish
		::WaitForSingleObject(SyncEvent, INFINITE);

		SetThreadPriority(InitPriority);
	}

	return ThreadHandle != NULL;
}


bool WindowsThread::Kill(bool WaitUntilExit)
{

	if (RunObject)
	{
		RunObject->Stop();
	}

	if (WaitUntilExit == true)
	{
		WaitForSingleObject(ThreadHandle, INFINITE);
	}

	if (SyncEvent != NULL)
	{
		CloseHandle(SyncEvent);
		SyncEvent = NULL;
	}

	CloseHandle(ThreadHandle);
	ThreadHandle = NULL;



	return true;
}




UINT32 WindowsThread::RunWrapper()
{
	UINT32 Result = 0;

	::SetThreadAffinityMask(::GetCurrentThread(), (DWORD_PTR)ThreadAffinityMask);

	if (::IsDebuggerPresent())
	{
		Result = Run();
	}
	else
	{

		//Structured exception handling
		__try
		{
			Result = Run();
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			std::cerr << "Error occurred on running Thread !!" << std::endl;
			TerminateProcess(GetCurrentProcess(), 1);
		}
	}

	return Result;
}



UINT32 WindowsThread::Run()
{
	UINT32 Result = 1;

	if (RunObject->Init())
	{
		//Set waiting point here
		::SetEvent(SyncEvent);

		Result = RunObject->Run();

		RunObject->Exit();
	}
	else
	{
		//Set waiting point here
		::SetEvent(SyncEvent);
	}

	return Result;
}


static void* InterlockedExchangePtr(void** Dest, void* Exchange)
{
	return (void*)InterlockedExchange64((long long*)Dest, (long long)Exchange);
}




ThreadProcesser::ThreadProcesser() :
	WriterThreadPtr(nullptr),
	StopTrigger(0),
	WorkingCounter(0),
	ReportCounter(0),
	CurrentQuestPos(0),
	RunFunc(nullptr),
	Progress(0.0),
	ProgressPerQuest(0.0),
	IntervalTime(0.0)
{

	InterlockedExchangePtr((void**)&WriterThreadPtr, WindowsThread::Create(this, 0, ThreadPriority::Normal));
}


ThreadProcesser::~ThreadProcesser()
{
	if (WriterThreadPtr != nullptr)
	{
		delete WriterThreadPtr;
		WriterThreadPtr = nullptr;
	}
}


bool ThreadProcesser::Init()
{
	return true;
}


UINT32 ThreadProcesser::Run()
{
	while (StopTrigger.GetCounter() == 0)
	{
		if (WorkingCounter.GetCounter() > 0)
		{
			InternelDoRequest();
			::Sleep(IntervalTime);
		}
		else
		{
			::Sleep(10);
		}
	}

	return 0;
}


void ThreadProcesser::Stop()
{
	StopTrigger.Increment();
}




bool ThreadProcesser::Kick()
{
	if (IsWorking() || RunFunc == nullptr)
		return false;

	LockGuard<WindowsCriticalSection> Lock(CriticalSection);

	ResultList = std::queue<void*>();

	CurrentQuestPos = 0;
	Progress = 0.0;
	ProgressPerQuest = 1.0 / (double)QuestList.size();

	WorkingCounter.Increment();

	return true;
}


bool ThreadProcesser::IsWorking()
{
	LockGuard<WindowsCriticalSection> Lock(CriticalSection);
	bool a = WorkingCounter.GetCounter() > 0;
	bool b = ResultList.size() > 0;

	return a || b;
}

void* ThreadProcesser::GetResult(double* OutCurrentProcess)
{
	LockGuard<WindowsCriticalSection> Lock(CriticalSection);

	*OutCurrentProcess = Progress;

	void* ResultData = nullptr;
	if (ResultList.size() > 0) {
		ResultData = ResultList.front();
		ResultList.pop();
	}
	return ResultData;
}

void ThreadProcesser::AddData(void* Data)
{
	if (IsWorking()) return;

	QuestList.push_back(Data);
}

void ThreadProcesser::Clear()
{
	if (IsWorking()) 
	{
		Stop();
	}
	::Sleep(1.0);

	QuestList.clear();
	ResultList = std::queue<void*>();
}


void ThreadProcesser::InternelDoRequest()
{
	double t = 0.0;
	if (QuestList.size() == 0) {
		Progress = 1.0;
	}
	else
	{
		void* SourceData = QuestList[CurrentQuestPos];
		double ProgressPerRun = 0.0;
		void* DestData = RunFunc(SourceData, &ProgressPerRun);

		Progress += ProgressPerRun * ProgressPerQuest;

		if (DestData != nullptr)
		{
			LockGuard<WindowsCriticalSection> Lock(CriticalSection);
			ResultList.push(DestData);
			CurrentQuestPos++;
		}

		if (CurrentQuestPos >= QuestList.size())
			Progress = 1.0;
	}

	if (Progress >= 0.999999999999999)
	{
		if (CurrentQuestPos == QuestList.size()) {
			Progress = 1.0;
			WorkingCounter.Decrement();
		}
		else
		{
			std::cout << "Progress Over 1.0 But QuestList Not Finish" << std::endl;
			std::cout << CurrentQuestPos << "|" << QuestList.size() << std::endl;
		}
	}

}

