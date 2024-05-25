// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PlanetGeneration.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT2_API UPlanetGeneration : public UBlueprintFunctionLibrary
{

	
	GENERATED_BODY()

		UPlanetGeneration();

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "bla")
		int32 radius;
	
};
