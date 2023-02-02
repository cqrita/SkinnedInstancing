#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimSequence.h"
#include "SIAnimationComponent.generated.h"

class FSIAnimationData;

UCLASS(hidecategories = (Object, LOD), meta = (BlueprintSpawnableComponent), ClassGroup = Rendering)
class SKINNEDINSTANCING_API USIAnimationComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** The Skeleton used by this component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkinnedInstancing")
	USkeleton* Skeleton;

	/** The AnimSequence used by this component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkinnedInstancing")
	TArray<UAnimSequence*> AnimSequences;

	/** Base pose to use when retargeting */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SkinnedInstancing")
	FName RetargetSource;

	UFUNCTION(BlueprintCallable, Category = "Components|SkinnedInstancing")
	UAnimSequence* GetSequence(int Id);

public:
	virtual ~USIAnimationComponent();

	const FSIAnimationData* GetAnimationData() const { return AnimationData; }

	//~ Override Functions
protected:
	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;
	virtual void DestroyRenderState_Concurrent() override;
	virtual bool RequiresGameThreadEndOfFrameRecreate() const override { return false; }
	//~ End UActorComponent Interface

private:
	void CreateAnimationData();

private:
	FSIAnimationData* AnimationData;
};
