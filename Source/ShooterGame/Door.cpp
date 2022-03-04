// Fill out your copyright notice in the Description page of Project Settings.


#include "Door.h"

// Sets default values
ADoor::ADoor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bIsOpen = false;
	bReadyState = true;
}

// Called when the game starts or when spawned
void ADoor::BeginPlay()
{
	Super::BeginPlay();
	
	rotate_value_ = 1.0f;

	if (door_curve_)
	{
		FOnTimelineFloat timeline_callback;
		//FOnTimelineEventStatic timeline_finished_callback;

		timeline_callback.BindUFunction(this, FName("ControlDoor"));
		//timeline_finished_callback.BindUFunction(this, FName("SetState"));
		my_timeline_.AddInterpFloat(door_curve_, timeline_callback);
		//my_timeline_.SetTimelineFinishedFunc(timeline_finished_callback);
	}
}

// Called every frame
void ADoor::Tick(float deltatime)
{
	Super::Tick(deltatime);
	
	my_timeline_.TickTimeline(deltatime);
}

void ADoor::ControlDoor(float value)
{
	curve_float_value_ = rotate_value_ * value;

	FQuat new_rotation = FQuat(FRotator(0.f, curve_float_value_, 0.f));
	SetActorRelativeRotation(new_rotation);
}

void ADoor::SetState()
{
	UE_LOG(LogTemp, Warning, TEXT("Door is ready to interact"));
	bReadyState = true;
}

void ADoor::Interacted_Implementation(APawn *interactor)
{
	UE_LOG(LogTemp, Warning, TEXT("Door is interacted"));
	//if (bReadyState)
	//{
		if (bIsOpen)
		{
			my_timeline_.Reverse();
			bIsOpen = false;
		}
		else 
		{
			my_timeline_.Play();
			bIsOpen = true;
		}
		bReadyState = false;
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Warning, TEXT("Door is not ready"));
	//}
}

