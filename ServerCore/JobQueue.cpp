#include "pch.h"
#include "JobQueue.h"
#include "GlobalQueue.h"

void JobQueue::Push(JobRef job, bool pushOnly)
{
	// ī��Ʈ ������Ű�� Job �߰�
	const int32 prevCount = _jobCount.fetch_add(1);
	_jobs.Push(job); // WRITE_LOCK;

	// ù��° Job ���� �����尡 ������� ���
	if (prevCount == 0)
	{
		// �̹� �������� JobQueue�� ������ ����
		if (LCurrentJobQueue == nullptr && pushOnly == false)
			Execute();
		else
		{
			// �����ִ� �ٸ� �����尡 �����ϵ��� GlobalQueue �� �ѱ��
			GGlobalQueue->Push(shared_from_this());
		}
	}
}

// ���ʸ� �����ϰԵǼ� �ϰ��� �ʹ� ������?
// DoAsync Ÿ�� Ÿ�� ���� ���� ������ �ʴ� ��Ȳ(�ϰ��� �� �����忡 ������ ��)
// �ϰ� ����� ���ؾ���
void JobQueue::Execute()
{
	LCurrentJobQueue = this;

	while (true)
	{
		Vector<JobRef> jobs;
		_jobs.PopAll(OUT jobs);

		const int32 jobCount = static_cast<int32>(jobs.size());
		for (int32 i = 0; i < jobCount; ++i)
			jobs[i]->Execute();

		// ���� �ϰ��� 0����� ����
		// �� �����ϰ� ī��Ʈ ����(ī��Ʈ ������ _jobs.Push(job)���̿� job�� �߰��ɼ�����)
		if (_jobCount.fetch_sub(jobCount) == jobCount)
		{
			LCurrentJobQueue = nullptr;
			return;
		}

		const uint64 now = ::GetTickCount64();
		if (now >= LEndTickCount)
		{
			LCurrentJobQueue = nullptr;
			// �����ִ� �ٸ� �����尡 �����ϵ��� GlobalQueue �� �ѱ��
			GGlobalQueue->Push(shared_from_this());
		}
	}
}
