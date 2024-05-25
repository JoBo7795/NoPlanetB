// Fill out your copyright notice in the Description page of Project Settings.


#include "MyBlueprintFunctionLibrary.h"

void UMyBlueprintFunctionLibrary::GetHeightMap(UMaterial* mat) {


	TArray<UTexture*> OutTextures;
	EMaterialQualityLevel::Type QualityLevel;
	bool bAllQualityLevels = true, bAllFeatureLevels = true;
	ERHIFeatureLevel::Type FeatureLevel;

	QualityLevel = EMaterialQualityLevel::Type(2);
	FeatureLevel = ERHIFeatureLevel::Type(1);


	if (mat != NULL) {
		mat->GetUsedTextures(OutTextures, QualityLevel, bAllQualityLevels, FeatureLevel, bAllFeatureLevels);
		printf("test");
		//OutTextures[0];
	}
}