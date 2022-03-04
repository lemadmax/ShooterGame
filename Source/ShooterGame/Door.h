// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InteractInterface.h"
#include "Components/TimelineComponent.h"

#include "Door.generated.h"

UCLASS()
class SHOOTERGAME_API ADoor : public AActor, public IInteractInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADoor();

	void BeginPlay() override;

public:	

	void Tick(float deltatime) override;

	UFUNCTION(NetMulticast, Reliable)
	virtual void Interacted(APawn *interactor) override;

protected:

	UFUNCTION()
	void ControlDoor(float value);

	UFUNCTION()
	void SetState();

	UPROPERTY(EditAnywhere)
	UCurveFloat* door_curve_;

	bool bIsOpen;
	bool bReadyState;
	float rotate_value_;
	float curve_float_value_;
	float timeline_value_;
	FRotator door_rotation_;
	FTimeline my_timeline_;

};
