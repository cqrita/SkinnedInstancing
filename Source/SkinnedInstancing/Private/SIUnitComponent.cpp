#include "SIUnitComponent.h"
#include "Animation/AnimSequence.h"
#include "SIMeshComponent.h"

#pragma optimize( "", off )

class USIUnitComponent::FAnimtionPlayer
{
public:
	struct Sequence
	{
		int Id;
		float Time;
		float Length;
		int NumFrames;

		Sequence() : Id(-1), Time(0), Length(0), NumFrames(0) {}
		Sequence(int Id, float Length, int NumFrames) : Id(Id), Time(0), Length(Length), NumFrames(NumFrames) {}

		void Tick(float DeltaTime, bool Loop = false)
		{
			Time += DeltaTime;
			if (Loop)
				Time = FMath::Fmod(Time, Length);
			else
				Time = FMath::Min(Time, Length);
		}
	};

public:
	const Sequence& GetCurrentSeq() const { return CurrentSeq; }
	const Sequence& GetNextSeq() const { return NextSeq; }
	float GetFadeTime() const { return FadeTime; }
	float GetFadeLength() const { return FadeLength; }

public:
	void Tick(float DeltaTime)
	{
		CurrentSeq.Tick(DeltaTime, IsLoop);

		if (FadeTime > 0 && FadeLength > 0)
		{
			FadeTime = FMath::Max(FadeTime - DeltaTime, 0.0f);

			NextSeq.Tick(DeltaTime);

			if (FadeTime <= 0)
			{
				CurrentSeq = NextSeq;
			}
		}
	}

	void Play(const Sequence& Seq, bool Loop)
	{
		IsLoop = Loop;
		CurrentSeq = NextSeq = Seq;
		FadeLength = FadeTime = 0;
	}

	void CrossFade(const Sequence& Seq, bool Loop, float Fade)
	{
		if (CurrentSeq.Id < 0)
		{
			Play(Seq, Loop);
			return;
		}
		NextSeq = Seq;
		FadeLength = FadeTime = Fade;
	}

private:
	bool IsLoop = false;
	float FadeLength = 0;

private:
	Sequence CurrentSeq;
	Sequence NextSeq;
	float FadeTime = 0;
};

namespace
{
	void GetInstanceDataFromPlayer(FSIMeshInstanceData::FAnimData& Data,
		const USIUnitComponent::FAnimtionPlayer::Sequence& Seq)
	{
		int NumFrames = Seq.NumFrames;
		float SequenceLength = Seq.Length;
		float Interval = (NumFrames > 1) ? (SequenceLength / (NumFrames - 1)) : MINIMUM_ANIMATION_LENGTH;

		float Time = Seq.Time;
		int Frame = Time / Interval;
		float Lerp = (Time - Frame * Interval) / Interval;

		Data.Sequence = Seq.Id;
		Data.PrevFrame = FMath::Clamp(Frame, 0, NumFrames - 1);
		Data.NextFrame = FMath::Clamp(Frame + 1, 0, NumFrames - 1);
		Data.FrameLerp = FMath::Clamp(Lerp, 0.0f, 1.0f);
	}
}

USIUnitComponent::USIUnitComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	InstanceId = 0;

	AnimtionPlayer = new FAnimtionPlayer();
}

USIUnitComponent::~USIUnitComponent()
{
	delete AnimtionPlayer;
}

void USIUnitComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{
	Super::CreateRenderState_Concurrent(Context);
	RecreateInstance();
}

void USIUnitComponent::SendRenderDynamicData_Concurrent()
{
	Super::SendRenderDynamicData_Concurrent();
	RecreateInstance();
}

void USIUnitComponent::DestroyRenderState_Concurrent()
{
	Super::DestroyRenderState_Concurrent();
	RemoveInstance();
}

void USIUnitComponent::RecreateInstance()
{
	RemoveInstance();

	if (MeshComponent!=nullptr)
	{
		InstanceId = MeshComponent->AddInstance(GetComponentTransform());
	}
}

void USIUnitComponent::RemoveInstance()
{
	if (InstanceId > 0)
	{
		if (MeshComponent!= nullptr)
		{
			MeshComponent->RemoveInstance(InstanceId);
		}
		InstanceId = 0;
	}
}

void USIUnitComponent::CrossFade(int Sequence, float FadeLength, bool Loop)
{
	if (MeshComponent != nullptr)
	{
		UAnimSequence* AnimSequence = MeshComponent->GetSequence(Sequence);
		if (AnimSequence)
		{
			int NumFrames = AnimSequence->GetNumberOfFrames();
			FAnimtionPlayer::Sequence Seq(Sequence, AnimSequence->SequenceLength, NumFrames);
			AnimtionPlayer->CrossFade(Seq, Loop, FadeLength);
		}
	}
}

void USIUnitComponent::Play(int Sequence, bool Loop)
{
	if (MeshComponent != nullptr)
	{
		UAnimSequence* AnimSequence = MeshComponent->GetSequence(Sequence);
		if (AnimSequence)
		{
			int NumFrames = AnimSequence->GetNumberOfFrames();
			FAnimtionPlayer::Sequence Seq(Sequence, AnimSequence->SequenceLength, NumFrames);
			AnimtionPlayer->Play(Seq,Loop);
		}
	}
}


void USIUnitComponent::SetMeshComponent(USIMeshComponent * _MeshComponent)
{
	RemoveInstance();
	MeshComponent = _MeshComponent;
}

void USIUnitComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	// Tick ActorComponent first.
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!AnimtionPlayer)
		return;

	AnimtionPlayer->Tick(DeltaTime);

	if (MeshComponent != nullptr && InstanceId > 0)
	{
		FSIMeshInstanceData* Instance = MeshComponent->GetInstanceData(InstanceId);
		check(Instance);
		Instance->Transform = GetComponentTransform().ToMatrixWithScale();

		GetInstanceDataFromPlayer(Instance->AnimDatas[0], AnimtionPlayer->GetCurrentSeq());
		GetInstanceDataFromPlayer(Instance->AnimDatas[1], AnimtionPlayer->GetNextSeq());

		float BlendWeight = 1;

		if (AnimtionPlayer->GetCurrentSeq().Id != AnimtionPlayer->GetNextSeq().Id)
		{
			float FadeTime = AnimtionPlayer->GetFadeTime();
			float FadeLength = FMath::Max(AnimtionPlayer->GetFadeLength(), 0.001f);
			BlendWeight = FadeTime / FadeLength;
		}

		Instance->AnimDatas[0].BlendWeight = BlendWeight;
		Instance->AnimDatas[1].BlendWeight = 1 - BlendWeight;
	}
	else if(MeshComponent!= nullptr && InstanceId==0)
	{
		RecreateInstance();
	}
}

#pragma optimize( "", on )
