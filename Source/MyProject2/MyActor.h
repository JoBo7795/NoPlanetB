// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <iostream>
#include <array>
#include <ctime>
#include <map>
#include "Containers/EnumAsByte.h"
#include "Engine/UserDefinedStruct.h"
//#include <set>

#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/KismetMathLibrary.h"
#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "ProceduralMeshComponent.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "Math/UnrealMathUtility.h"
#include "Components/SphereComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "MyActor.generated.h"

#define CITY_ID_NO_CITY -1
#define MINIMUM_CONTINENT_SIZE 250


UENUM()
enum EHouseData {
	House UMETA(DisplayName = "House"),
	Empty UMETA(DisplayName = "Empty"),
	Tree UMETA(DisplayName = "Tree"),
	Oil  UMETA(DisplayName = "Oil"),
	OilP  UMETA(DisplayName = "OilP"),
	OilPP  UMETA(DisplayName = "OilPP"),
	Coal  UMETA(DisplayName = "Coal"),
	CoalP  UMETA(DisplayName = "CoalP"),
	CoalPP  UMETA(DisplayName = "CoalPP"),
	Gas  UMETA(DisplayName = "Gas"),
	GasP  UMETA(DisplayName = "GasP"),
	GasPP  UMETA(DisplayName = "GasPP"),
	Nuclear  UMETA(DisplayName = "Nuclear"),
	NuclearP  UMETA(DisplayName = "NuclearP"),
	NuclearPP  UMETA(DisplayName = "NuclearPP"),
	Hydro  UMETA(DisplayName = "Hydro"),
	HydroP  UMETA(DisplayName = "HydroP"),
	HydroPP  UMETA(DisplayName = "HydroPP"),
	Wind  UMETA(DisplayName = "Wind"),
	WindP  UMETA(DisplayName = "WindP"),
	WindPP  UMETA(DisplayName = "WindPP"),
	Solar  UMETA(DisplayName = "Solar"),
	SolarP  UMETA(DisplayName = "SolarP"),
	SolarPP  UMETA(DisplayName = "SolarPP"),
	BioFuel  UMETA(DisplayName = "BioFuel"),
	BioFuelP  UMETA(DisplayName = "BioFuelP"),
	BioFuelPP  UMETA(DisplayName = "BioFuelPP"),
	Cyclone  UMETA(DisplayName = "Cyclone"),
	CityReference UMETA(DisplayName = "CityReference")
};

UENUM()
enum ETerrainClass {
	Water UMETA(DisplayName = "Water"),
	LandNoCity UMETA(DisplayName = "LandNoCity"),
	LandCity UMETA(DisplayName = "LandCity"),
	LandObject UMETA(DisplayName = "LandObject"),
	Mountain UMETA(DisplayName = "Mountain")
};

USTRUCT()
struct FMovingObject {
	GENERATED_USTRUCT_BODY()

private:



public:

	AMyActor* actor;

	//TArray<int> cubeSideList;
	TArray < std::array<int, 2>> cubeSideList;

	int  mappedX = 0, mappedY = 0;
	int x, y;
	int* p_x, * p_y;
	int currSide, lastSide;
	int turnI = 0;
	int currIndex = 0;
	bool swapX = false, swapY = false;
	int turnOffset = 0;

	UPROPERTY()
		bool setCity = false;
	UPROPERTY()
		bool setBuilding = false;

	template<class T>
	int MapCoordsRelative(TArray<T>& arr, int posX, int posY, int resolution);

	void IncreaseX();
	void DecreaseX();
	void IncreaseY();
	void DecreaseY();

};

USTRUCT(BlueprintType)
struct FCityData : public FMovingObject {

	GENERATED_USTRUCT_BODY()

		int pos, cityGrowIndex = 50, lastSetFieldPos, lastDirection, lastRad, ID, initX, initY, posX, posY;
	FLinearColor color;
	TSet<int> cityFields;
	int lastSideDownLeft, lastSideDownRight, lastSideUpLeft, lastSideUpRight;
	int updateX = 1, updateY = 1;
	UPROPERTY()
		AActor* actorObj;

};

USTRUCT(BlueprintType)
struct FCycloneData : public FMovingObject {

	GENERATED_USTRUCT_BODY()
		FCycloneData() {}

	FCycloneData(int spawnPosition, FVector2D _direction, int moveRange) {

		pos = spawnPosition;
		direction = _direction;
		this->moveRange = moveRange;

	}

	int pos, lastSetFieldPos, ID, initX, initY, posX, posY, moveRange;
	FVector2D direction;
	FVector actorLocation;
	FRotator actorRotation;

	UPROPERTY()
		AActor* actorObj;
};

USTRUCT(BlueprintType)
struct FFloodData : public FMovingObject {

	GENERATED_USTRUCT_BODY()
		FFloodData() {}

	FFloodData(int _spawnPosition, int _moveLength, int _size) {

		spawnPosition = _spawnPosition;
		moveLength = _moveLength;
		size = 0;
		maxSize = _size;

	}

	int spawnPosition, moveLength, size, maxSize;
	bool reduce = false;
	TArray<int> fields;


};

USTRUCT(BlueprintType)
struct FFieldData {

	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		TEnumAsByte<EHouseData> houseData;
	UPROPERTY()
		FCityData cityData;
	UPROPERTY()
		TEnumAsByte<ETerrainClass> terrainClass;
	UPROPERTY()
		AActor* actor;
	UPROPERTY()
		FVector position;
	UPROPERTY()
		FRotator rotation;

	FFieldData() {}

	FFieldData(EHouseData _houseData, FVector _position = FVector(0.0f, 0.0f, 0.0f), FRotator _rotation = FRotator(0.0f, 0.0f, 0.0f)) {
		houseData = _houseData;
		position = _position;
		rotation = _rotation;
		actor = nullptr;

		cityData.ID = CITY_ID_NO_CITY;
	}

};

class FMyWorker : public FRunnable
{
public:

	FMyWorker();

	~FMyWorker();

	virtual bool Init() override; // Do your setup here, allocate memory, ect.
	virtual uint32 Run() override; // Main data processing happens here
	virtual void Stop() override; // Clean up any memory you allocated here

	AMyActor* actor = nullptr;

private:

	bool bRunThread;
};

class FManageMoveables : public FRunnable
{
public:

	FManageMoveables();

	~FManageMoveables();

	virtual bool Init() override; // Do your setup here, allocate memory, ect.
	virtual uint32 Run() override; // Main data processing happens here
	virtual void Stop() override; // Clean up any memory you allocated here

	AMyActor* actor = nullptr;

private:

	bool bRunThread;
};

class FInterfaceUpdate : public FRunnable
{
public:

	FInterfaceUpdate();

	~FInterfaceUpdate();

	virtual bool Init() override; // Do your setup here, allocate memory, ect.
	virtual uint32 Run() override; // Main data processing happens here
	virtual void Stop() override; // Clean up any memory you allocated here

	AMyActor* actor = nullptr;

private:

	bool bRunThread;
};


UCLASS()
class MYPROJECT2_API AMyActor : public AActor
{
	GENERATED_BODY()

		friend class FMyWorker;
	friend class FManageMoveables;
	friend class FInterfaceUpdate;

public:
	// Sets default values for this actor's properties
	AMyActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
		UProceduralMeshComponent* CustomMesh;

	//FMyWorker worker;
	/* The vertices of the mesh */
	TArray<FVector> Vertices;

	/* The triangles of the mesh */
	TArray<int32> Triangles;
	TArray<FVector2D> UVs;
	TArray<FVector> normals;
	TArray<int32> triIndices;
	TArray<int> borderArr, polyArr, continentIDs;
	TArray<std::map<int, TArray<int>>> continentIndices;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> tangents;
	TArray<float> heights;
	TArray<FVector> pointsOnSphere, initialSphere;
	float maxHeight, averageHeight;
	APlayerController* pc;
	float mouseX, mouseY;
	int32 ViewportSizeX, ViewportSizeY;
	FVector WorldLocation, WorldDirection, actorLocation;
	TQueue<int> houseBuildQueue;

	TArray<FCityData> cityDataArr;
	TQueue<FCityData> cityDataQueue;

	TArray<FCycloneData> cycloneDataArr;
	TQueue<FCycloneData> cycloneDataQueue;
	TQueue<AActor*> actorRemoveQueue;

	TArray<FFloodData> floodDataArr;
	TQueue<FFloodData> floodDataQueue;

	FMyWorker* worker = nullptr;
	FManageMoveables* manageMoveablesWorker = nullptr;
	FInterfaceUpdate* interfaceUpdateWorker = nullptr;
	FRunnableThread* Thread, * manageMoveablesThread, * interfaceUpdateThread;
	EHouseData currentHouseData;

	std::map<AActor*, int> cityInterfaceMap;

	bool isLoaded;
	int clickIndex;
	int cityIDs;
	int l = 0;

	UPROPERTY()
		bool setCity = false;
	bool setBuilding = false;

	UPROPERTY(EditAnywhere)
		UMaterial* StoredMaterial;


	UMaterialInstanceDynamic* DynamicMaterialInst;
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sphere")
		int radius = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sphere")
		int resolution = 1024;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
		float noiseStrength = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
		float noiseRoughness = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
		int numLayers = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
		float persistence = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
		FVector noiseCenter = FVector(0,0,0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Noise")
		AActor* HouseObjectAct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PlaceableObjects")
		TArray<TSubclassOf<AActor>> HouseObjects;

	UPROPERTY(BlueprintReadWrite, Category = "PlanetData")
		int globalUsableCellCount;

	UPROPERTY(BlueprintReadWrite, Category = "PlanetData")
		int globalNonUsableCellCount;

	/*map continent ID to the number of it's usable Cells*/
	UPROPERTY(BlueprintReadWrite, Category = "PlanetData")
		TMap<int, int> continentUsableCellCount;

	/*map continent ID to the number of it's non usable Cells*/
	UPROPERTY(BlueprintReadWrite, Category = "PlanetData")
		TMap<int, int> continentNonUsableCellCount;

	UFUNCTION(BlueprintCallable)
		void CreateMesh();

	FVector CubeToSphere(FVector point);
	FVector2D PointOnSphereToUV(FVector point);
	FColor interpolateColor(float percent, FColor colorLeft, FColor colorRight);
	float ArrayMax(TArray<float>& heightArr);
	void VertexColorByHeight(TArray<float>& heightArr, float maxArrayVal);
	void FlattenMids(TArray<float>& heightArr, float maxArrayVal);
	void ExpandHeights(TArray<float>& heightArr, float maxArrayVal);
	void ColorPolygonBorder(TArray<float>& heightArr);
	void CreatePolygonBorder(TArray<float>& heightArr);
	void CreateContinents(TArray<float>& heightArr);
	int CPUClickDetection();
	int CurrentSideFromIndex(int index);

	std::array<int, 2> GetCubeSideOffset(int nextindexX, int nextindexY, int currSide/*, int resolution*/);

	template <class T>
	T& BorderlessArrayAccess(TArray<T>& arr, int indexX, int indexY, int xSize, int ySize, int currSide/*, resolution*/);

	template <class T>
	int BorderlessIndexMap(TArray<T>& arr, int indexX, int indexY, int xSize, int ySize, int currSide/*, resolution*/);

	UFUNCTION(BlueprintCallable)
		void SetCurrentHouseData(EHouseData status);

	UFUNCTION(BlueprintCallable, Category = "PlanetData")
		int CitySize(int cityID);

	UFUNCTION(BlueprintCallable, Category = "PlanetData")
		int CityCount();

	UFUNCTION(BlueprintCallable, Category = "PlanetData")
		TArray<int> GetAllContinentIDs();

	//UFUNCTION(BlueprintCallable, Category = "PlanetData")
	int GlobalUsableCellCount();

	//UFUNCTION(BlueprintCallable, Category = "PlanetData")
	int GlobalNonUsableCellCount();

	UFUNCTION(BlueprintCallable, Category = "PlanetData")
		int ContinentUsableCellCount(int continentID);

	UFUNCTION(BlueprintCallable, Category = "PlanetData")
		int ContinentNonUsableCellCount(int continentID);

	UFUNCTION(BlueprintCallable, Category = "PlanetData")
		int PlanetCellCount();

	UFUNCTION(BlueprintCallable, Category = "PlanetData")
		TArray<int> NeighboringCells(int cell, int rad);

	UFUNCTION(BlueprintCallable, Category = "PlanetInteraction")
		int HightlightContinent(int lastIndex);

	UFUNCTION(BlueprintCallable, Category = "PlanetInteraction")
		int GetSelectedContinentID();

	UFUNCTION(BlueprintCallable, Category = "PlanetInteraction")
		FFieldData GetSelectedCellInfo();

	UFUNCTION(BlueprintCallable, Category = "PlanetInteraction")
		void SetMultipleFields(int clickPos, int rad, EHouseData status);

	UFUNCTION(BlueprintCallable, Category = "PlanetInteraction")
		void GrowCity(TArray<FLinearColor>& colorArray, int posOffset, int fieldsToPaint, FCityData& cityData);

	UFUNCTION(BlueprintCallable, Category = "PlanetInteraction")
		void GrowCityAmount(int cityID, int amount);

	UFUNCTION(BlueprintCallable, Category = "PlanetInteraction")
		void SetCityNow(bool val);

	UFUNCTION(BlueprintCallable, Category = "PlanetInteraction")
		void SetBuildingNow(bool val);

	UFUNCTION(BlueprintCallable, Category = "PlanetInteraction")
		void CreateCyclone(int spawnLocation, FVector2D direction, int moveRange);

	UFUNCTION(BlueprintCallable, Category = "PlanetInteraction")
		void CreateFlood(int moveLength, int size);

	UFUNCTION(BlueprintCallable, Category = "PlanetInteraction")
		int GetCityIDs(AActor* actor);

	void MoveCyclone(FCycloneData& elem);
	void ExtendFlood(FFloodData& floodObj);

	void ReduceFlood(FFloodData& floodObj);

	void TestCity(TArray<FLinearColor>& colorArray, int posOffset, int fieldsToPaint, FCityData& cityData);

	EHouseData GetCurrentHouseData();
	void ChangeFieldStatus(int position, EHouseData status);
	void ClassifyFields(TArray<float>& heightArr);
	void PlaceCities(int posOffset, int fieldsToPaint);
	bool PlaceCity(int pos, int fieldsToPaint);
	bool CheckSurroundingFields(int x, int y, int currentSide, int dim, int ID, int index);
	bool PlaceCityField(TArray<FLinearColor>& colorArray, FCityData& city, int currentSide);

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable)
		void SelectKey(AActor* actor, FKey key);

	UFUNCTION(BlueprintCallable)
		bool isReady();

	TArray<FFieldData> fieldDataArray;
};