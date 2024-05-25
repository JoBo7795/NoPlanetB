// Fill out your copyright notice in the Description page of Project Settings.

#include "MyActor.h"

// Sets default values
AMyActor::AMyActor()
{
	isLoaded = false;
	worker = nullptr;
	cityIDs = 0;
	currentHouseData = EHouseData::Empty;
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	OnClicked.AddDynamic(this, &AMyActor::SelectKey);
	CustomMesh = CreateDefaultSubobject<UProceduralMeshComponent>("CustomMesh");
	SetRootComponent(CustomMesh);

	static ConstructorHelpers::FObjectFinder<UMaterial> FoundMaterial(TEXT("/Game/VertColorMat.VertColorMat"));

	if (FoundMaterial.Succeeded())
	{
		StoredMaterial = FoundMaterial.Object;
	}
}


bool AMyActor::isReady() {
	return isLoaded;
}

void AMyActor::SetCityNow(bool val) {
	setCity = val;
}

void AMyActor::SetBuildingNow(bool val) {
	setBuilding = val;
}

FVector FindLineSphereIntersections(FVector linePoint0, FVector linePoint1, FVector circleCenter, double innerCircleRadius)
{

	double cx = circleCenter.X;
	double cy = circleCenter.Y;
	double cz = circleCenter.Z;

	double px = linePoint0.X;
	double py = linePoint0.Y;
	double pz = linePoint0.Z;

	double vx = linePoint1.X - px;
	double vy = linePoint1.Y - py;
	double vz = linePoint1.Z - pz;

	double A = vx * vx + vy * vy + vz * vz;
	double B = 2.0 * (px * vx + py * vy + pz * vz - vx * cx - vy * cy - vz * cz);
	double C = px * px - 2 * px * cx + cx * cx + py * py - 2 * py * cy + cy * cy +
		pz * pz - 2 * pz * cz + cz * cz - innerCircleRadius * innerCircleRadius;

	// discriminant
	double D = B * B - 4 * A * C;

	double t1 = (-B - sqrt(D)) / (2.0 * A);

	FVector solution1 = FVector(linePoint0.X * (1 - t1) + t1 * linePoint1.X,
		linePoint0.Y * (1 - t1) + t1 * linePoint1.Y,
		linePoint0.Z * (1 - t1) + t1 * linePoint1.Z);

	double t2 = (-B + sqrt(D)) / (2.0 * A);
	FVector solution2 = FVector(linePoint0.X * (1 - t2) + t2 * linePoint1.X,
		linePoint0.Y * (1 - t2) + t2 * linePoint1.Y,
		linePoint0.Z * (1 - t2) + t2 * linePoint1.Z);

	if (D == 0)
	{
		return  solution1;
	}
	else
	{

		if (FVector::Distance(solution1, linePoint0) < FVector::Distance(solution2, linePoint0))
			return solution1;

		return solution2;
	}
}

int AMyActor::CPUClickDetection() {

	FVector mouseLocationFar, mouseLocationNear, mouseDirection, mousePos, mouseLocation1, mouseLocation2;

	FVector offset = FVector(0.0f, 0.0f, 0.0f) - GetActorLocation();

	mouseLocationFar = FindLineSphereIntersections(actorLocation, WorldLocation, GetActorLocation(), radius * (1 + maxHeight / 2));

	mouseLocationNear = FindLineSphereIntersections(actorLocation, WorldLocation, GetActorLocation(), radius);
	int smallestIndexNear = 0, smallestIndexFar = 0;
	float smallestDistNear = FVector::Distance(mouseLocationNear, Vertices[0]), smallestDistFar = FVector::Distance(mouseLocationFar, Vertices[0]);

	for (int i = 0; i < 6; i++) {
		for (int x = 0; x < resolution; x++) {
			for (int y = 0; y < resolution; y++) {
				if (FVector::Distance(mouseLocationNear, Vertices[i * resolution * resolution + (x * resolution + y)]) < smallestDistNear) {
					smallestDistNear = FVector::Distance(mouseLocationNear, Vertices[i * resolution * resolution + (x * resolution + y)]);
					smallestIndexNear = i * resolution * resolution + (x * resolution + y);
				}

				if (FVector::Distance(mouseLocationFar, Vertices[i * resolution * resolution + (x * resolution + y)]) < smallestDistFar) {
					{
						smallestDistFar = FVector::Distance(mouseLocationFar, Vertices[i * resolution * resolution + (x * resolution + y)]);
						smallestIndexFar = i * resolution * resolution + (x * resolution + y);
					}
				}
			}
		}
	}

	return polyArr[smallestIndexFar] != -1 ? smallestIndexFar : smallestIndexNear;

}

void AMyActor::SelectKey(AActor* actor, FKey key)
{

	if (key.GetFName() == "LeftMouseButton") {

		//ChangeFieldStatus(clickIndex,currentHouseData);
		//auto test = GetSelectedCellInfo();	

		if (setBuilding) {
			SetMultipleFields(clickIndex, 1, currentHouseData);

			setBuilding = false;
		}
		setCity = true;
		if (setCity) {
			if(PlaceCity(clickIndex, 10))
				setCity = false;
		}


		UE_LOG(LogTemp, Warning, TEXT("index %i"), clickIndex);
		UE_LOG(LogTemp, Warning, TEXT("Continent %i"), GetSelectedContinentID());

	}

	if (key.GetFName() == "RightMouseButton") {
		UE_LOG(LogTemp, Warning, TEXT("right"));
			
		//CreateCyclone(clickIndex,FVector2D(1,0),100);

		UE_LOG(LogTemp, Warning, TEXT("index %i"), clickIndex);
		UE_LOG(LogTemp, Warning, TEXT("Continent %i"), GetSelectedContinentID());
	}

}

// Called when the game starts or when spawned
void AMyActor::BeginPlay()
{
	Super::BeginPlay();
	CreateMesh();	

	for (auto elem : continentIndices) {
		continentUsableCellCount.Add(elem.begin()->first, 0);
		continentNonUsableCellCount.Add(elem.begin()->first, 0);
	}

	if (worker == nullptr) {
		pc = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		worker = new FMyWorker();
		worker->actor = this;
		Thread = FRunnableThread::Create(worker, TEXT("Mouse Thread"));
		Thread->SetThreadPriority(TPri_BelowNormal);
	}

	if (manageMoveablesWorker == nullptr) {

		manageMoveablesWorker = new FManageMoveables();
		manageMoveablesWorker->actor = this;
		manageMoveablesThread = FRunnableThread::Create(manageMoveablesWorker, TEXT("Manage Moveables Thread"));
		manageMoveablesThread->SetThreadPriority(TPri_BelowNormal);
	}

	if (interfaceUpdateWorker == nullptr) {

		interfaceUpdateWorker = new FInterfaceUpdate();
		interfaceUpdateWorker->actor = this;
		interfaceUpdateThread = FRunnableThread::Create(interfaceUpdateWorker, TEXT("Interface Update Thread"));
		interfaceUpdateThread->SetThreadPriority(TPri_BelowNormal);
	}

	ContinentUsableCellCount(0);
	ContinentNonUsableCellCount(0);

	//CreateFlood(100,5);
}

void AMyActor::EndPlay(const EEndPlayReason::Type EndPlayReason) {
	Super::EndPlay(EndPlayReason);

	Thread->Kill();
	manageMoveablesThread->Kill();
	interfaceUpdateThread->Kill();
	delete worker;
	delete manageMoveablesThread;
	delete interfaceUpdateWorker;
}

// Called every frame
void AMyActor::Tick(float DeltaTime) {

	Super::Tick(DeltaTime);

	CustomMesh->UpdateMeshSection_LinearColor(0, Vertices, normals, UVs, VertexColors, tangents);

	pc->GetMousePosition(mouseX, mouseY);

	pc->GetViewportSize(ViewportSizeX, ViewportSizeY);

	pc->DeprojectScreenPositionToWorld(mouseX, mouseY, WorldLocation, WorldDirection);

	actorLocation = pc->GetPawn()->GetActorLocation();

	while (!houseBuildQueue.IsEmpty()) {
		int* housePos = houseBuildQueue.Peek();
		ChangeFieldStatus(*housePos, EHouseData::House);
		houseBuildQueue.Pop();
	}
	
	while (!actorRemoveQueue.IsEmpty()) {
		AActor** actor = actorRemoveQueue.Peek();
		actor[0]->Destroy();
		actorRemoveQueue.Pop();
	}

	for (FCycloneData& elem : cycloneDataArr) {
		elem.actorObj->SetActorLocation(elem.actorLocation);
		elem.actorObj->SetActorRotation(elem.actorRotation);
	}

}

float Evaluate(FVector point, float roughness, float strength, int numLayers, float persistence, FVector center)
{
	// create noise value, normalize to 0 - 1

	float noiseVal = 0, amplitude = 1.0f;
	float frequency = 1.0f;//roughness;
	for (int i = 0; i < numLayers; i++)
	{

		noiseVal += ((FMath::PerlinNoise3D(point * frequency + center) + 1) / 2) * amplitude;
		frequency *= roughness;
		amplitude *= persistence;
	}
	noiseVal = FMath::Max(0.0f, noiseVal - 0.4f);
	return noiseVal * strength;
}

FVector AMyActor::CubeToSphere(FVector point)
{

	float x2 = point.X * point.X;
	float y2 = point.Y * point.Y;
	float z2 = point.Z * point.Z;

	float x = point.X * std::sqrt(1 - (y2 + z2) / 2 + (y2 * z2) / 3);
	float y = point.Y * std::sqrt(1 - (z2 + x2) / 2 + (x2 * z2) / 3);
	float z = point.Z * std::sqrt(1 - (x2 + y2) / 2 + (x2 * y2) / 3);

	return FVector(x, y, z);
}

FVector2D AMyActor::PointOnSphereToUV(FVector point)
{

	point.Normalize();

	float lon = std::atan2(point.X, -point.Z);
	float lat = std::asin(point.Y);

	float u = (lon / PI + 1) / 2;
	float v = (lat / PI + 0.5f);

	return FVector2D(u, v);
}

FColor AMyActor::interpolateColor(float percent, FColor colorLeft, FColor colorRight) {

	return FColor(colorLeft.R * (percent)+colorRight.R * (1 - percent), colorLeft.G * (percent)+colorRight.G * (1 - percent), colorLeft.B * (percent)+colorRight.B * (1 - percent), colorLeft.A);

}

float AMyActor::ArrayMax(TArray<float>& heightArr) {

	float currMax = heightArr[0];

	for (float val : heightArr) {
		if (currMax < val)
			currMax = val;
	}
	return currMax;

}

void AMyActor::VertexColorByHeight(TArray<float>& heightArr,/* TArray<FLinearColor> &colorArray,*/ float maxArrayVal) {

	for (float val : heightArr) {

		FColor color;
		if (val == 0.0f)
			color = FColor(0.0f, 0.0f, 255.0f);
		else if (val < (maxArrayVal / 3 * 2))
			color = FColor(0.0f, 255.0f, 0.0f);
		else
			color = FColor(255.0f, 0.0f, 0.0f);

		VertexColors.Add(color);
	}

}

void AMyActor::FlattenMids(TArray<float>& heightArr, float maxArrayVal) {
	for (float& val : heightArr) {
		if (val < (maxArrayVal / 3))
			val = 0;
		else if (val > (maxArrayVal / 3) && val < (maxArrayVal / 3 * 2))
			val = (maxArrayVal / 2) * .1f;
		else
			val = val * .1f;
	}

}

void AMyActor::ExpandHeights(TArray<float>& heightArr, float maxArrayVal) {
	for (float& val : heightArr) {
		if (val > (maxArrayVal / 3 * 2))
			val *= 1.5;
	}
}

// map side according to order of side creation
std::array<int, 2> AMyActor::GetCubeSideOffset(int nextindexX, int nextindexY, int currSide/*,int res*/) {

	// top => side 0
	if (currSide == 0) {

		if (nextindexX >= resolution)
			return { 2, 1 };

		if (nextindexX < 0)
			return { 3, 3 };

		if (nextindexY >= resolution)
			return { 5, 3 };

		if (nextindexY < 0)
			return { 4, 3 };

		return { 0, 0 };
	}

	// bottom => side 1
	if (currSide == 1) {

		if (nextindexX >= resolution)
			return { 3, 3 };

		if (nextindexX < 0)
			return { 2, 1 };

		if (nextindexY >= resolution)
			return { 5, 1 };

		if (nextindexY < 0)
			return { 4, 1 };

		return { 1, 0 };
	}

	// right => side 2
	if (currSide == 2) {

		if (nextindexX >= resolution)
			return { 4, 1 };

		if (nextindexX < 0)
			return { 5, 3 };

		if (nextindexY >= resolution)
			return { 1, 3 };

		if (nextindexY < 0)
			return { 0, 3 };

		return { 2, 0 };
	}


	// left side 3
	if (currSide == 3) {

		if (nextindexX >= resolution)
			return { 5, 3 };

		if (nextindexX < 0)
			return { 4, 1 };

		if (nextindexY >= resolution)
			return { 1, 1 };

		if (nextindexY < 0)
			return { 0, 1 };

		return { 3, 0 };
	}


	// front side 4
	if (currSide == 4) {

		if (nextindexX >= resolution)
			return { 0, 1 };

		if (nextindexX < 0)
			return { 1, 3 };

		if (nextindexY >= resolution)
			return { 3, 3 };

		if (nextindexY < 0)
			return { 2, 3 };

		return { 4, 0 };
	}

	// back side 5
	if (currSide == 5) {

		if (nextindexX >= resolution)
			return { 1, 3 };

		if (nextindexX < 0)
			return { 0, 1 };

		if (nextindexY >= resolution)
			return { 3, 1 };

		if (nextindexY < 0)
			return { 2, 1 };

		return { 5, 0 };
	}

	return { 0, 0 };
}

template <class T>
T& AMyActor::BorderlessArrayAccess(TArray<T>& arr, int indexX, int indexY, int xSize, int ySize, int currSide/*, int resolution*/) {

	int sideOffset = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[0];
	int turn = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[1];

	if (indexX >= xSize) {
		sideOffset = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[0];
		turn = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[1];

		indexX = indexX % xSize;

	}

	if (indexX < 0) {
		sideOffset = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[0];
		turn = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[1];

		//indexX = xSize - 1;
		indexX = xSize - 1 + indexX + 1;

	}

	if (indexY >= ySize) {
		sideOffset = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[0];
		turn = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[1];

		indexY = indexY % ySize;
	}

	if (indexY < 0) {
		sideOffset = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[0];
		turn = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[1];

		//indexY = ySize - 1;
		indexY = ySize - 1 + indexY + 1;

	}

	// turn 90 degrees left
	if (turn == 1) {
		int tmp = indexX;
		indexX = std::abs(indexY - ySize) - 1;
		indexY = tmp;
	}

	// turn 180 degrees
	if (turn == 2) {
		indexX = std::abs(xSize - indexX) - 1;
		indexY = std::abs(ySize - indexY) - 1;

	}

	// turn 90 degrees right
	if (turn == 3) {
		int tmp = std::abs(xSize - indexX) - 1;
		indexX = indexY;
		indexY = tmp;
	}

	if ((resolution * resolution * sideOffset) + indexX * ySize + indexY > resolution * resolution * 6 - 1 ||
		(resolution * resolution * sideOffset) + indexX * ySize + indexY < 0)
		return arr[0];

	return arr[(resolution * resolution * sideOffset) + indexX * ySize + indexY];
}

template <class T>
int AMyActor::BorderlessIndexMap(TArray<T>& arr, int indexX, int indexY, int xSize, int ySize, int currSide/*, int resolution*/) {

	int sideOffset = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[0];
	int turn = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[1];

	if (indexX >= xSize) {
		sideOffset = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[0];
		turn = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[1];

		indexX = indexX % xSize;
	}

	if (indexX < 0) {
		sideOffset = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[0];
		turn = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[1];

		//indexX = xSize - 1;// +indexX + 1;
		indexX = xSize - 1 + indexX + 1;

	}

	if (indexY >= ySize) {
		sideOffset = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[0];
		turn = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[1];

		indexY = indexY % ySize;
	}

	if (indexY < 0) {
		sideOffset = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[0];
		turn = GetCubeSideOffset(indexX, indexY, currSide/*, resolution*/)[1];

		//indexY = ySize - 1;// +indexY + 1;
		indexY = ySize - 1 + indexY + 1;

	}

	// turn 90 degrees left
	if (turn == 1) {
		int tmp = indexX;
		indexX = std::abs(indexY - ySize) - 1;
		indexY = tmp;
	}

	// turn 180 degrees
	if (turn == 2) {
		indexX = std::abs(xSize - indexX) - 1;
		indexY = std::abs(ySize - indexY) - 1;

	}

	// turn 90 degrees right
	if (turn == 3) {
		int tmp = std::abs(xSize - indexX) - 1;
		indexX = indexY;
		indexY = tmp;
	}

	return (resolution * resolution * sideOffset) + indexX * ySize + indexY;
}

// use kernel to determin heights of neighboring points, if there is 1 with height 0 in kernel values -> border = yellow color
void AMyActor::ColorPolygonBorder(TArray<float>& heightArr/*, TArray<FLinearColor>& colorArray*/) {

	int size = resolution;
	int xSize = size;
	int ySize = size;
	int maxVal;

	for (int i = 0; i < 6; i++) {
		for (int x = 0; x < xSize; x++) {
			for (int y = 0; y < ySize; y++) {

				if ((BorderlessArrayAccess(heightArr, x - 1, y - 1, xSize, size, i/*, resolution*/) == 0 || BorderlessArrayAccess(heightArr, x, y - 1, xSize, size, i/*, resolution*/) == 0 || BorderlessArrayAccess(heightArr, x + 1, y - 1, xSize, size, i/*, resolution*/) == 0 ||
					BorderlessArrayAccess(heightArr, x - 1, y, xSize, size, i/*, resolution*/) == 0 || BorderlessArrayAccess(heightArr, x + 1, y, xSize, size, i/*, resolution*/) == 0 ||
					BorderlessArrayAccess(heightArr, x - 1, y + 1, xSize, size, i/*, resolution*/) == 0 || BorderlessArrayAccess(heightArr, x, y + 1, xSize, size, i/*, resolution*/) == 0 || BorderlessArrayAccess(heightArr, x + 1, y + 1, xSize, size, i/*, resolution*/) == 0) &&
					BorderlessArrayAccess(heightArr, x, y, xSize, size, i/*, resolution*/) != 0) {
					VertexColors[i * resolution * resolution + (x * size + y)] = FLinearColor::Yellow;
					//borderArr[i * resolution * resolution + (x * size + y)] = 1;
					borderArr.Add(i * resolution * resolution + (x * size + y));
				}

				maxVal = i * resolution * resolution + (x * size + y);
			}
		}

	}

	UE_LOG(LogTemp, Warning, TEXT("maxVal: %i"), maxVal);
	UE_LOG(LogTemp, Warning, TEXT("colorArrSize:: %i"), VertexColors.Num());

}

void AMyActor::CreatePolygonBorder(TArray<float>& heightArr/*, TArray<FLinearColor>& colorArray*/) {

	int size = resolution;
	int xSize = size;
	int ySize = size;
	int maxVal;

	for (int i = 0; i < 6; i++) {
		for (int x = 0; x < xSize; x++) {
			for (int y = 0; y < ySize; y++) {

				if ((BorderlessArrayAccess(heightArr, x - 1, y - 1, xSize, size, i/*, resolution*/) == 0 || BorderlessArrayAccess(heightArr, x, y - 1, xSize, size, i/*, resolution*/) == 0 || BorderlessArrayAccess(heightArr, x + 1, y - 1, xSize, size, i/*, resolution*/) == 0 ||
					BorderlessArrayAccess(heightArr, x - 1, y, xSize, size, i/*, resolution*/) == 0 || BorderlessArrayAccess(heightArr, x + 1, y, xSize, size, i/*, resolution*/) == 0 ||
					BorderlessArrayAccess(heightArr, x - 1, y + 1, xSize, size, i/*, resolution*/) == 0 || BorderlessArrayAccess(heightArr, x, y + 1, xSize, size, i/*, resolution*/) == 0 || BorderlessArrayAccess(heightArr, x + 1, y + 1, xSize, size, i/*, resolution*/) == 0) &&
					BorderlessArrayAccess(heightArr, x, y, xSize, size, i/*, resolution*/) != 0) {
					borderArr.Add(i * resolution * resolution + (x * size + y));
				}

				maxVal = i * resolution * resolution + (x * size + y);
			}
		}

	}

	UE_LOG(LogTemp, Warning, TEXT("maxVal: %i"), maxVal);
	UE_LOG(LogTemp, Warning, TEXT("colorArrSize:: %i"), VertexColors.Num());

}

void AMyActor::CreateContinents(/*TArray<FLinearColor>& colorArray,*/ TArray<float>& heightArr) {

	int size = resolution;
	int xSize = size;
	int ySize = size;
	bool inPoly = false;
	FLinearColor randCol = FLinearColor::White;
	int polyCounter = 0;
	TArray<std::array<int, 2>> associations;
	TArray<std::map<int, TArray<int>>> finalAssoc;

	finalAssoc.Reserve(6 * resolution * resolution);
	associations.Reserve(6 * resolution * resolution);

	for (int i = 0; i < 6; i++) {
		for (int x = 0; x < xSize; x++) {
			for (int y = 0; y < ySize; y++) {

				if (BorderlessArrayAccess(heightArr, x, y, xSize, size, i/*, resolution*/) > 0.0f && !inPoly) {
					inPoly = true;

				}

				else if (BorderlessArrayAccess(heightArr, x, y, xSize, size, i/*, resolution*/) == 0.0f && inPoly) {
					inPoly = false;
					polyCounter++;
				}

				if (inPoly)
				{

					polyArr[i * resolution * resolution + (x * size + y)] = polyCounter;
					bool valFound = false;
					int minVal = polyCounter;

					for (int j = -1; j < 2; j++)
						for (int k = -1; k < 2; k++) {
							int polyArrVal = BorderlessArrayAccess(polyArr, x + j, y + k, xSize, size, i/*, resolution*/);
							if (polyArrVal > -1) {
								valFound = true;
								if (polyArrVal < minVal && polyArrVal != -1)
									minVal = polyArrVal;
							}

						}

					if (valFound)
					{
						BorderlessArrayAccess(polyArr, x, y, xSize, size, i/*, resolution*/) = minVal;
						for (int j = -1; j < 2; j++) {
							for (int k = -1; k < 2; k++) {
								int arrVal = BorderlessArrayAccess(polyArr, x + j, y + k, xSize, size, i/*, resolution*/);

								if (arrVal != -1 && minVal != -1 && arrVal != minVal && associations.Find({ arrVal,minVal }) == INDEX_NONE) {
									associations.Push({ arrVal,minVal });
								}
							}
						}
					}
				}
			}
		}
	}

	std::map<int, TArray<int>>::iterator it;
	for (auto& elem : associations) {

		if (finalAssoc.Num() <= 0) {
			TArray<int> newArr;
			newArr.Push(elem[0]);
			finalAssoc.Push({ {elem[1],newArr} });
		}

		for (int i = 0; i < finalAssoc.Num(); i++) {

			it = finalAssoc[i].find(elem[1]);

			if (it != finalAssoc[i].end())
			{
				if (it->second.Find(elem[0]) == INDEX_NONE) {
					it->second.Push(elem[0]);
					break;
				}
			}

			else
			{
				TArray<int> newArr;
				newArr.Push(elem[0]);
				finalAssoc.Push({ {elem[1],newArr} });
				break;
			}
		}
	}


	finalAssoc.Sort([](auto& elem1, auto& elem2) {
		return elem1.begin()->first > elem2.begin()->first;
		});


	for (int i = finalAssoc.Num() - 1; i > 0; i--) {
		for (int j = i - 1; j > 0; j--) {
			for (int iVal : finalAssoc[i].begin()->second)
				if (finalAssoc[j].begin()->second.Find(iVal) != INDEX_NONE && iVal != finalAssoc[i].begin()->first) {

					finalAssoc[j].begin()->second.Push(finalAssoc[i].begin()->first);
					break;
				}
		}
	}

	while (finalAssoc.Num() > 0) {
		TArray<std::map<int, TArray<int>>> removeArr;

		removeArr.Reserve(finalAssoc.Num());
		for (int i = finalAssoc.Num() - 1; i > 0; i--) {
			for (int j = i - 1; j > 0; j--) {

				if (finalAssoc[i].begin()->first == finalAssoc[j].begin()->first || finalAssoc[j].begin()->second.Find(finalAssoc[i].begin()->first) != INDEX_NONE) {

					for (int& val : finalAssoc[j].begin()->second) {
						if (finalAssoc[i].begin()->second.Find(val) == INDEX_NONE)
							finalAssoc[i].begin()->second.Push(val);
					}


					if (removeArr.Find(finalAssoc[j]) == INDEX_NONE)
						removeArr.Push(finalAssoc[j]);

				}

			}
		}
		if (removeArr.Num() == 0)
			break;

		for (auto val : removeArr) {
			finalAssoc.Remove(val);
		}
	}

	while (finalAssoc.Num() > 0) {
		TArray<std::map<int, TArray<int>>> removeArr;
		removeArr.Reserve(finalAssoc.Num());

		for (int i = 0; i < 6; i++) {
			for (int x = 0; x < xSize; x++) {
				for (int y = 0; y < ySize; y++) {
					int arrSize = finalAssoc.Num();
					for (int j = 0; j < arrSize; j++) {

						auto fElem = finalAssoc[j];
						for (int val : fElem.begin()->second) {
							if (val == polyArr[i * resolution * resolution + (x * size + y)]) {
								polyArr[i * resolution * resolution + (x * size + y)] = fElem.begin()->first;
								int foundIndex = removeArr.Find(fElem);
								if (foundIndex == INDEX_NONE) {
									TArray<int> newArr;
									newArr.Push(i * resolution * resolution + (x * size + y));
									continentIndices.Push({ {fElem.begin()->first, newArr} });
								}
								else {
									continentIndices[foundIndex][fElem.begin()->first].Push(i * resolution * resolution + (x * size + y));
								}
								if (removeArr.Find(fElem) == INDEX_NONE)
									removeArr.Push(fElem);

							}
						}
					}
				}
			}
		}
		if (removeArr.Num() == 0)
			break;

		for (auto& val : removeArr) {
			finalAssoc.Remove(val);
		}
	}

	for (auto& elem : continentIndices) {
		continentIDs.Add(elem.begin()->first);
	}

	for (int i = 0; i < 6; i++) {
		for (int x = 0; x < xSize; x++) {
			for (int y = 0; y < ySize; y++) {

				if (polyArr[i * resolution * resolution + (x * size + y)] != -1) {
					if (polyArr[i * resolution * resolution + (x * size + y)] % 8 == 0)
						VertexColors[i * resolution * resolution + (x * size + y)] = FLinearColor::Red;
					if (polyArr[i * resolution * resolution + (x * size + y)] % 8 == 1)
						VertexColors[i * resolution * resolution + (x * size + y)] = FLinearColor::Green;
					if (polyArr[i * resolution * resolution + (x * size + y)] % 8 == 2)
						VertexColors[i * resolution * resolution + (x * size + y)] = FLinearColor::Black;
					if (polyArr[i * resolution * resolution + (x * size + y)] % 8 == 3)
						VertexColors[i * resolution * resolution + (x * size + y)] = FLinearColor::Yellow;
					if (polyArr[i * resolution * resolution + (x * size + y)] % 8 == 4)
						VertexColors[i * resolution * resolution + (x * size + y)] = FLinearColor(1.0f, 1.0f, 0.0f);
					if (polyArr[i * resolution * resolution + (x * size + y)] % 8 == 5)
						VertexColors[i * resolution * resolution + (x * size + y)] = FLinearColor::White;
					if (polyArr[i * resolution * resolution + (x * size + y)] % 8 == 6)
						VertexColors[i * resolution * resolution + (x * size + y)] = FLinearColor(0.0f, 1.0f, 1.0f);
					if (polyArr[i * resolution * resolution + (x * size + y)] % 8 == 7)
						VertexColors[i * resolution * resolution + (x * size + y)] = FLinearColor(1.0f, 0.0f, 1.0f);
				}
			}
		}
	}

}

/*Classify fields with water land and mountain enum dependend on it's height value*/
void AMyActor::ClassifyFields(TArray<float>& heightArr) {

	for (int i = 0; i < heightArr.Num(); i++) {
		if (heightArr[i] < (maxHeight / 3))
			fieldDataArray[i].terrainClass = ETerrainClass::Water;
		else if (heightArr[i] > (maxHeight / 3) && heightArr[i] < (maxHeight / 3 * 2))
			fieldDataArray[i].terrainClass = ETerrainClass::LandNoCity;
		else
			fieldDataArray[i].terrainClass = ETerrainClass::Mountain;
	}

}

void AMyActor::CreateMesh() {

	TArray<FVector> directions;

	directions.Add(FVector(0.0f, 0.0f, 1.0f));

	directions.Add(FVector(0.0f, 0.0f, -1.0f));

	directions.Add(FVector(0.0f, 1.0f, 0.0f));

	directions.Add(FVector(0.0f, -1.0f, 0.0f));

	directions.Add(FVector(1.0f, 0.0f, 0.0f));

	directions.Add(FVector(-1.0f, 0.0f, 0.0f));

	int vertOffset = resolution * resolution, triOffset = 0, offset = 0;

	int coordOffset = 100;
	//srand(static_cast <unsigned> (time(0)));
	//
	//noiseCenter = FVector(
	//	static_cast <float> (rand()) / static_cast <float> (RAND_MAX),
	//	static_cast <float> (rand()) / static_cast <float> (RAND_MAX),
	//	static_cast <float> (rand()) / static_cast <float> (RAND_MAX));

	//borderArr.Init(0, (resolution) * (resolution) * 6);
	polyArr.Init(-1, (resolution) * (resolution) * 6);
	heights.Init(0.0f, (resolution) * (resolution) * 6);
	fieldDataArray.Init(EHouseData(EHouseData::Empty), (resolution) * (resolution) * 6);

	for (FVector Up : directions)
	{
		FVector axisX = FVector(Up.Y, Up.Z, Up.X);
		FVector axisY = Up;
		FVector axisZ = FVector().CrossProduct(Up, axisX);

		int triIndex = 0;

		for (int i = 0; i < (resolution) * (resolution); i++)
		{
			int x = i / (resolution), y = i % (resolution);

			FVector2D percentage = FVector2D(x, y) / (resolution - 1);

			FVector pointOnUnitSphere = CubeToSphere(axisY + (percentage.X - 0.5f) * 2 * axisX + (percentage.Y - 0.5f) * 2 * axisZ);
			//FVector pointOnUnitSphere = axisY + (percentage.X - 0.5f) * 2 * axisX + (percentage.Y - 0.5f) * 2 * axisZ;

			initialSphere.Add(pointOnUnitSphere * radius);


			UVs.Add(PointOnSphereToUV(pointOnUnitSphere));
			float height = Evaluate(pointOnUnitSphere, noiseRoughness, noiseStrength, numLayers, persistence, noiseCenter);
			//float height = 0;// Evaluate(pointOnUnitSphere, noiseRoughness, noiseStrength, numLayers, persistence, noiseCenter);
			
			if (offset == 7)
				height = Evaluate(pointOnUnitSphere, noiseRoughness, noiseStrength, numLayers, persistence, noiseCenter);
			else
				height = 0.5f;
			
			if ((y > 800 || y < 300) && (offset == 2) || ((y > 800 || y < 300) && (offset == 3)) || ((x > 800 || x < 300) && (offset == 1)) || ((x > 800 || x < 300) && (offset == 0)))
				height = 0.0f;
			
			height = Evaluate(pointOnUnitSphere, noiseRoughness, noiseStrength, numLayers, persistence, noiseCenter);
			
			
			if (height > 0.1f)
				height += Evaluate(pointOnUnitSphere, noiseRoughness + 5, 1.2f, numLayers, -.5f, noiseCenter);

			heights[x * (resolution)+y + resolution * resolution * offset] = height;
			pointsOnSphere.Add(pointOnUnitSphere);
			
			normals.Add(pointOnUnitSphere);
			tangents.Add(FProcMeshTangent(0, 1, 0));

			if (x < (resolution - 1) && y < (resolution - 1))
			{
				triIndices.Add((vertOffset * offset) + i);
				triIndices.Add((vertOffset * offset) + i + resolution + 1);
				triIndices.Add((vertOffset * offset) + i + resolution);

				triIndices.Add((vertOffset * offset) + i);
				triIndices.Add((vertOffset * offset) + i + 1);
				triIndices.Add((vertOffset * offset) + i + resolution + 1);

				triIndex += 6;
			}
		}
		offset++;
	}

	maxHeight = ArrayMax(heights);
	FlattenMids(heights, maxHeight);
	averageHeight = (maxHeight / 2) * .1f;
	maxHeight = ArrayMax(heights);
	VertexColorByHeight(heights, maxHeight);
	ExpandHeights(heights, maxHeight);
	CreateContinents(heights);
	ClassifyFields(heights);
	
	//ColorPolygonBorder(heights/*, VertexColors/*, borderArr*/);
	CreatePolygonBorder(heights);



	for (int i = 0; i < pointsOnSphere.Num(); i++) {
		FVector height = pointsOnSphere[i] * radius * (1 + heights[i]);
		Vertices.Add(height);
		fieldDataArray[i].position = height;

		FVector rotationAxis = FVector::CrossProduct(FVector::UpVector, pointsOnSphere[i] * radius);
		float angle = FMath::Acos(FVector::DotProduct(FVector::UpVector.GetSafeNormal(), height.GetSafeNormal())) * 180 / PI;

		fieldDataArray[i].rotation = UKismetMathLibrary::RotatorFromAxisAndAngle(rotationAxis, angle);

	}

	CustomMesh->CreateMeshSection_LinearColor(0, Vertices, triIndices, normals, UVs, VertexColors, tangents, false);

	DynamicMaterialInst = UMaterialInstanceDynamic::Create(StoredMaterial, CustomMesh);

	CustomMesh->SetMaterial(0, DynamicMaterialInst);

	isLoaded = true;

}

int AMyActor::CurrentSideFromIndex(int index) {
	return index / (resolution * resolution);
}

void AMyActor::SetCurrentHouseData(EHouseData status) {
	currentHouseData = status;
}

int AMyActor::GetSelectedContinentID() {
	return polyArr[clickIndex];
}

FFieldData AMyActor::GetSelectedCellInfo() {
	return fieldDataArray[clickIndex];
}

void AMyActor::SetMultipleFields(int clickPos, int rad, EHouseData status) {
	int currentSide = CurrentSideFromIndex(clickPos);
	int x = (clickPos - currentSide * (resolution * resolution)) / resolution;
	int y = (clickPos - currentSide * (resolution * resolution)) % resolution;

	for (int i = -rad; i <= rad; i++)
		for (int j = -rad; j <= rad; j++) {

			if (BorderlessArrayAccess(fieldDataArray, x + i, y + j, resolution, resolution, currentSide).terrainClass != ETerrainClass::LandNoCity)
				return;

		}

	for (int i = -rad; i <= rad; i++)
		for (int j = -rad; j <= rad; j++) {

			BorderlessArrayAccess(fieldDataArray, x + i, y + j, resolution, resolution, currentSide).terrainClass = ETerrainClass::LandObject;
			BorderlessArrayAccess(VertexColors, x + i, y + j, resolution, resolution, CurrentSideFromIndex(clickPos)) = FLinearColor::Blue;

			if (i == 0 && j == 0) {
				ChangeFieldStatus(BorderlessIndexMap(VertexColors, x + i, y + j, resolution, resolution, currentSide), status);
				BorderlessArrayAccess(fieldDataArray, x + i, y + j, resolution, resolution, currentSide).houseData = status;
			}
		}
}

// Function that returns the size of the city with ID "cityID"
// Return INDEX_NONE if nothing is found
int AMyActor::CitySize(int cityID) {

	for (auto& elem : cityDataArr) {
		if (elem.ID == cityID)
			return elem.cityFields.Num();
	}

	return INDEX_NONE;
}

// Function that returns the number of cities on the whole planet
int AMyActor::CityCount() {
	return cityDataArr.Num();
}

TArray<int> AMyActor::GetAllContinentIDs() {
	return continentIDs;
}

// Function that returns the Number of the cells of the whole planet
int AMyActor::PlanetCellCount() {
	return resolution * resolution * 6;
}

// Function that returns the Number of all usable cells of the whole planet
int AMyActor::GlobalUsableCellCount() {

	int usableFieldCount = 0;

	for (auto& elem : continentIndices) {
		if (elem.begin()->second.Num() > MINIMUM_CONTINENT_SIZE) {
			for (int i : elem.begin()->second) {
				if (fieldDataArray[i].terrainClass == ETerrainClass::LandNoCity)
					usableFieldCount++;
			}
		}
	}
	return usableFieldCount;
}

// Function that returns the Number of all non usable cells of the whole planet
int AMyActor::GlobalNonUsableCellCount() {

	int usableFieldCount = 0;

	for (auto& elem : continentIndices) {
		if (elem.begin()->second.Num() > MINIMUM_CONTINENT_SIZE) {
			for (int i : elem.begin()->second) {
				if (fieldDataArray[i].terrainClass != ETerrainClass::LandNoCity)
					usableFieldCount++;
			}
		}
	}
	return usableFieldCount;
}

// Function that returns the Number of usable cells of the continent with continentID
int AMyActor::ContinentUsableCellCount(int continentID) {

	int usableFieldCount = 0;

	for (auto& elem : continentIndices) {
		if (elem.begin()->first == continentID) {
			for (int i : elem.begin()->second) {
				if (fieldDataArray[i].terrainClass == ETerrainClass::LandNoCity)
					usableFieldCount++;
			}
		}
	}
	return usableFieldCount;
}

// Function that returns the Number of non usable cells of the continent with continentID
int AMyActor::ContinentNonUsableCellCount(int continentID) {

	int usableFieldCount = 0;

	for (auto& elem : continentIndices) {
		if (elem.begin()->first == continentID) {
			for (int i : elem.begin()->second) {
				if (fieldDataArray[i].terrainClass != ETerrainClass::LandNoCity)
					usableFieldCount++;
			}
		}
	}
	return usableFieldCount;
}

//Function that return the neighboring cells of the specified cell
TArray<int> AMyActor::NeighboringCells(int cell, int rad) {

	TArray<int> neighbors;
	int currentSide = CurrentSideFromIndex(cell);

	int initX = (cell - currentSide * (resolution * resolution)) / resolution;
	int initY = (cell - currentSide * (resolution * resolution)) % resolution;

	for (int x = initX - rad; x <= initX + rad; x++)
		for (int y = initY - rad; y <= initY + rad; y++) {
			neighbors.Add(BorderlessIndexMap(fieldDataArray, initX + x, initY + y, resolution, resolution, currentSide/*, resolution*/));
		}

	return neighbors;
}

EHouseData AMyActor::GetCurrentHouseData() {
	return currentHouseData;
}

/*Use to update the status of the selected field at position*/
void AMyActor::ChangeFieldStatus(int position, EHouseData status) {

	if (!(position < resolution * resolution * 6))
		return;

	FActorSpawnParameters spawnParams;
	fieldDataArray[position].houseData = status;

	if (fieldDataArray[position].actor != nullptr)
		fieldDataArray[position].actor->Destroy();

	if (status == EHouseData::House)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[24], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AddActorWorldRotation(fieldDataArray[position].rotation);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

	}
	if (status == EHouseData::Tree)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[24], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AddActorWorldRotation(fieldDataArray[position].rotation);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

	}
	if (status == EHouseData::BioFuel)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[0], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::BioFuelP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[1], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::BioFuelPP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[2], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::Coal)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[3], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::CoalP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[4], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::CoalPP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[5], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::Gas)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[6], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::GasP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[7], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::GasPP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[8], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::Hydro)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[9], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::HydroP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[10], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::HydroPP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[11], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::Nuclear)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[12], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::NuclearP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[13], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::NuclearPP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[14], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::Oil)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[15], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::OilP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[16], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::OilPP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[17], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::Solar)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[18], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::SolarP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[19], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::SolarPP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[20], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::Wind)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[21], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::WindP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[22], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::WindPP)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[23], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}
	if (status == EHouseData::CityReference)
	{
		fieldDataArray[position].actor = GetWorld()->SpawnActor<AActor>(HouseObjects[26], fieldDataArray[position].position, fieldDataArray[position].rotation, spawnParams);
		fieldDataArray[position].actor->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);
	}

}

bool AMyActor::CheckSurroundingFields(int x, int y, int currentSide, int dim, int ID, int index) {

	for (int j = -dim; j <= dim; j++) {
		for (int k = -dim; k <= dim; k++) {
			if (BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, currentSide/*, resolution*/).terrainClass == ETerrainClass::LandCity) {
				//if (fieldDataArray[(resolution * resolution * currentSide) + (x+j) * resolution + (y+k)].terrainClass == ETerrainClass::LandCity) {
				for (int j2 = -dim; j2 <= dim; j2++) {
					for (int k2 = -dim; k2 <= dim; k2++) {
						int val = BorderlessArrayAccess(fieldDataArray, x + j2, y + k2, resolution, resolution, currentSide/*, resolution*/).cityData.ID;
						//int val = fieldDataArray[(resolution * resolution * currentSide) + (x + j) * resolution + (y + k)].cityData.ID;

						if (val == ID)
							return true;
					}
				}
			}
		}
	}
	return false;
}

bool AMyActor::PlaceCity(int pos, int fieldsToPaint) {

	int currentSide = CurrentSideFromIndex(pos);

	FCityData cityData;
	cityData.actor = this;
	cityData.ID = cityIDs;
	cityData.pos = pos;
	cityData.lastSetFieldPos = cityData.pos;
	cityData.lastDirection = 0;
	cityData.lastRad = 1;

	cityData.initX = (cityData.lastSetFieldPos - currentSide * (resolution * resolution)) / resolution;
	cityData.x = (cityData.lastSetFieldPos - currentSide * (resolution * resolution)) / resolution;
	cityData.initY = (cityData.lastSetFieldPos - currentSide * (resolution * resolution)) % resolution;
	cityData.y = (cityData.lastSetFieldPos - currentSide * (resolution * resolution)) % resolution;

	cityData.mappedX = cityData.x;
	cityData.mappedY = cityData.y;

	cityData.p_x = &cityData.mappedX;
	cityData.p_y = &cityData.mappedY;

	cityData.posX = cityData.initX - cityData.lastRad;
	cityData.posY = cityData.initY - cityData.lastRad;

	if (BorderlessArrayAccess(fieldDataArray, cityData.initX, cityData.initY, resolution, resolution, currentSide/*, resolution*/).terrainClass == ETerrainClass::Water) {
		return false;
	}

	cityData.lastSideDownLeft = currentSide;
	cityData.lastSideDownRight = currentSide;
	cityData.lastSideUpLeft = currentSide;
	cityData.lastSideUpRight = currentSide;
	cityData.currSide = currentSide;
	cityData.lastSide = -1;//currentSide;//cityData.currSide;
	cityData.turnI = GetCubeSideOffset(cityData.x, cityData.y, cityData.currSide)[1];
	cityData.cubeSideList.Add(GetCubeSideOffset(cityData.x, cityData.y, cityData.currSide));

	if ((BorderlessArrayAccess(fieldDataArray, cityData.x, cityData.y, resolution, resolution, currentSide/*, resolution*/).terrainClass == ETerrainClass::Water) ||
		(BorderlessArrayAccess(fieldDataArray, cityData.x, cityData.y, resolution, resolution, currentSide/*, resolution*/).terrainClass == ETerrainClass::Mountain) ||
		(BorderlessArrayAccess(fieldDataArray, cityData.x, cityData.y, resolution, resolution, currentSide/*, resolution*/).terrainClass == ETerrainClass::LandObject) ||
		(!(BorderlessArrayAccess(fieldDataArray, cityData.x, cityData.y, resolution, resolution, currentSide/*, resolution*/).cityData.ID == CITY_ID_NO_CITY)))
		return false;


	FLinearColor col;

	if (cityData.ID % 8 == 0)
		col = FLinearColor::Red;
	if (cityData.ID % 8 == 1)
		col = FLinearColor::Green;
	if (cityData.ID % 8 == 2)
		col = FLinearColor::Black;
	if (cityData.ID % 8 == 3)
		col = FLinearColor::Yellow;
	if (cityData.ID % 8 == 4)
		col = FLinearColor(1.0f, 1.0f, 0.0f);
	if (cityData.ID % 8 == 5)
		col = FLinearColor::White;
	if (cityData.ID % 8 == 6)
		col = FLinearColor(0.0f, 1.0f, 1.0f);
	if (cityData.ID % 8 == 7)
		col = FLinearColor(1.0f, 0.0f, 1.0f);

	// choose new city color if it fit's the ground
	if (col == BorderlessArrayAccess(VertexColors, cityData.initX, cityData.initY, resolution, resolution, currentSide)) {
		if (cityData.ID % 8 + 1 == 0)
			col = FLinearColor::Red;
		if (cityData.ID % 8 + 1 == 1)
			col = FLinearColor::Green;
		if (cityData.ID % 8 + 1 == 2)
			col = FLinearColor::Black;
		if (cityData.ID % 8 + 1 == 3)
			col = FLinearColor::Yellow;
		if (cityData.ID % 8 + 1 == 4)
			col = FLinearColor(1.0f, 1.0f, 0.0f);
		if (cityData.ID % 8 + 1 == 5)
			col = FLinearColor::White;
		if (cityData.ID % 8 + 1 == 6)
			col = FLinearColor(0.0f, 1.0f, 1.0f);
		if (cityData.ID % 8 + 1 == 7)
			col = FLinearColor(1.0f, 0.0f, 1.0f);

	}

	cityData.color = col;

	BorderlessArrayAccess(VertexColors, cityData.initX, cityData.initY, resolution, resolution, currentSide/*, resolution*/) = FLinearColor::Black;
	BorderlessArrayAccess(fieldDataArray, cityData.initX, cityData.initY, resolution, resolution, currentSide/*, resolution*/).terrainClass = ETerrainClass::LandCity;
	BorderlessArrayAccess(fieldDataArray, cityData.initX, cityData.initY, resolution, resolution, currentSide/*, resolution*/).cityData = cityData;

	int index = BorderlessIndexMap(fieldDataArray, cityData.initX, cityData.initY, resolution, resolution, currentSide/*, resolution*/);

	ChangeFieldStatus(index, EHouseData::CityReference);

	cityInterfaceMap.insert({ fieldDataArray[index].actor,cityIDs });

	cityDataQueue.Enqueue(cityData);
	cityIDs++;

	return true;
}

int AMyActor::GetCityIDs(AActor* actor) {
	return cityInterfaceMap[actor];
}

void AMyActor::PlaceCities(int seed, int fieldsToPaint) {

	for (auto& elem : continentIndices) {
		if (elem.begin()->second.Num() > MINIMUM_CONTINENT_SIZE) {
			int currentSide = CurrentSideFromIndex(elem.begin()->second[elem.begin()->second.Num() / 2]);

			FCityData cityData;
			cityData.actor = this;
			cityData.ID = cityIDs;
			cityData.pos = elem.begin()->second[elem.begin()->second.Num() / 2];
			cityData.lastSetFieldPos = cityData.pos;
			cityData.lastDirection = 0;
			cityData.lastRad = 1;


			cityData.initX = (cityData.lastSetFieldPos - currentSide * (resolution * resolution)) / resolution;
			cityData.x = (cityData.lastSetFieldPos - currentSide * (resolution * resolution)) / resolution;
			cityData.initY = (cityData.lastSetFieldPos - currentSide * (resolution * resolution)) % resolution;
			cityData.y = (cityData.lastSetFieldPos - currentSide * (resolution * resolution)) % resolution;

			cityData.mappedX = cityData.x;
			cityData.mappedY = cityData.y;

			cityData.p_x = &cityData.x;
			cityData.p_y = &cityData.y;

			cityData.posX = cityData.initX - cityData.lastRad;
			cityData.posY = cityData.initY - cityData.lastRad;

			cityData.lastSideDownLeft = currentSide;
			cityData.lastSideDownRight = currentSide;
			cityData.lastSideUpLeft = currentSide;
			cityData.lastSideUpRight = currentSide;
			cityData.currSide = GetCubeSideOffset(cityData.x, cityData.y, currentSide)[0];//currentSide;
			cityData.lastSide = currentSide;
			cityData.cubeSideList.Add(GetCubeSideOffset(cityData.x, cityData.y, cityData.currSide));

			FLinearColor col;

			if (cityData.ID % 8 == 0)
				col = FLinearColor::Red;
			if (cityData.ID % 8 == 1)
				col = FLinearColor::Green;
			if (cityData.ID % 8 == 2)
				col = FLinearColor::Black;
			if (cityData.ID % 8 == 3)
				col = FLinearColor::Yellow;
			if (cityData.ID % 8 == 4)
				col = FLinearColor(1.0f, 1.0f, 0.0f);
			if (cityData.ID % 8 == 5)
				col = FLinearColor::White;
			if (cityData.ID % 8 == 6)
				col = FLinearColor(0.0f, 1.0f, 1.0f);
			if (cityData.ID % 8 == 7)
				col = FLinearColor(1.0f, 0.0f, 1.0f);

			// choose new city color if it fit's the ground
			if (col == BorderlessArrayAccess(VertexColors, cityData.initX, cityData.initY, resolution, resolution, currentSide)) {
				if (cityData.ID % 8 + 1 == 0)
					col = FLinearColor::Red;
				if (cityData.ID % 8 + 1 == 1)
					col = FLinearColor::Green;
				if (cityData.ID % 8 + 1 == 2)
					col = FLinearColor::Black;
				if (cityData.ID % 8 + 1 == 3)
					col = FLinearColor::Yellow;
				if (cityData.ID % 8 + 1 == 4)
					col = FLinearColor(1.0f, 1.0f, 0.0f);
				if (cityData.ID % 8 + 1 == 5)
					col = FLinearColor::White;
				if (cityData.ID % 8 + 1 == 6)
					col = FLinearColor(0.0f, 1.0f, 1.0f);
				if (cityData.ID % 8 + 1 == 7)
					col = FLinearColor(1.0f, 0.0f, 1.0f);

			}

			cityData.color = col;

			BorderlessArrayAccess(VertexColors, cityData.initX, cityData.initY, resolution, resolution, currentSide/*, resolution*/) = FLinearColor::Black;
			BorderlessArrayAccess(fieldDataArray, cityData.initX, cityData.initY, resolution, resolution, currentSide/*, resolution*/).terrainClass = ETerrainClass::LandCity;
			BorderlessArrayAccess(fieldDataArray, cityData.initX, cityData.initY, resolution, resolution, currentSide/*, resolution*/).cityData = cityData;

			cityDataQueue.Enqueue(cityData);
			cityIDs++;
		}
	}
}

bool AMyActor::PlaceCityField(TArray<FLinearColor>& colorArray, FCityData& city, int currentSide) {

	if (!(BorderlessArrayAccess(fieldDataArray, city.x, city.y, resolution, resolution, currentSide/*, resolution*/).terrainClass == ETerrainClass::Water) &&
		!(BorderlessArrayAccess(fieldDataArray, city.x, city.y, resolution, resolution, currentSide/*, resolution*/).terrainClass == ETerrainClass::Mountain) &&
		!(BorderlessArrayAccess(fieldDataArray, city.x, city.y, resolution, resolution, currentSide/*, resolution*/).terrainClass == ETerrainClass::LandObject) &&
		CheckSurroundingFields(city.x, city.y, currentSide, 1, city.ID, 0) &&
		((BorderlessArrayAccess(fieldDataArray, city.x, city.y, resolution, resolution, currentSide/*, resolution*/).cityData.ID == CITY_ID_NO_CITY)))
	{

		BorderlessArrayAccess(colorArray, city.x, city.y, resolution, resolution, currentSide/*, resolution*/) = city.color;
		BorderlessArrayAccess(fieldDataArray, city.x, city.y, resolution, resolution, currentSide/*, resolution*/).cityData = city;
		BorderlessArrayAccess(fieldDataArray, city.x, city.y, resolution, resolution, currentSide/*, resolution*/).terrainClass = ETerrainClass::LandCity;

		city.cityFields.Add(BorderlessIndexMap(fieldDataArray, city.x, city.y, resolution, resolution, currentSide/*, resolution*/));

		city.lastSetFieldPos = (resolution * resolution * currentSide) + city.x * resolution + city.y;
		return true;
	}

	return false;
}


void FMovingObject::IncreaseX() {
	x++;

	int val = 0;
	if (swapX) {
		mappedY++;
		val = mappedY;
	}
	else {
		mappedX++;
		val = mappedX;
	}

	//mappedX++;
	std::array<int, 2> offset;

	//if (mappedX >= actor->resolution) {
	if (val >= actor->resolution) {

		if (turnI == 1) {
			int tmp = mappedX;
			mappedX = mappedY;//actor->resolution - mappedY;
			//mappedX = actor->resolution - mappedY;
			mappedY = tmp;

			swapX = !swapX;

		}

		// turn 180 degrees
		if (turnI == 2) {
			mappedX = std::abs(actor->resolution - mappedX);
			mappedY = std::abs(actor->resolution - mappedY);

		}

		// turn 90 degrees right
		if (turnI == 3) {
			int tmp = std::abs(mappedX - actor->resolution) - 1;//mappedX; //std::abs(actor->resolution - mappedX);			
			mappedX = mappedY;
			mappedY = tmp;

			swapX = !swapX;

		}


		offset = actor->GetCubeSideOffset(mappedX, mappedY, currSide);
		//lastSide = currSide;
		currSide = offset[0];
		turnI = offset[1];


		cubeSideList.Add(offset);

		//mappedX = mappedX % actor->resolution;

		if (mappedX > 99)
			mappedX = 0;

		if (mappedX < 0)
			mappedX = 0;

		if (mappedY < 0)
			mappedY = 0;

		if (mappedY > 99)
			mappedY = 0;

	}

}


void FMovingObject::DecreaseX() {

	x--;
	int val = 0;
	if (swapX) {
		mappedY--;
		val = mappedY;
	}
	else {
		mappedX--;
		val = mappedX;
	}
	//mappedX--;
	std::array<int, 2> offset;

	//if (mappedX < 0) {
	if (val < 0) {

		if (turnI == 1) {
			// TODO hier der Fehler mit der Indexierung!
			int tmp = mappedX;
			//mappedX = std::abs(actor->resolution - mappedY) - 1;
			//mappedX = actor->resolution - mappedY;
			mappedX = mappedY;
			mappedY = tmp;
			swapX = !swapX;
		}

		// turn 180 degrees
		if (turnI == 2) {
			mappedX = std::abs(actor->resolution - mappedX);
			mappedY = std::abs(actor->resolution - mappedY);

		}

		// turn 90 degrees right
		if (turnI == 3) {
			//int tmp = std::abs(mappedX - actor->resolution) - 1;//mappedX; //std::abs(actor->resolution - mappedX);			
			int tmp = std::abs(mappedX - actor->resolution) - 1;//mappedX; //std::abs(actor->resolution - mappedX);			
			//int tmp = mappedX; //std::abs(actor->resolution - mappedX);			
			mappedX = mappedY;
			mappedY = tmp;
			swapX = !swapX;

		}


		offset = actor->GetCubeSideOffset(mappedX, mappedY, currSide);
		//lastSide = currSide;

		currSide = offset[0];
		turnI = offset[1];

		cubeSideList.Add(offset);
		//cubeSideList.Insert(offset,0);

		//mappedX = mappedX % actor->resolution;

		if (mappedX > 99)
			mappedX = 99;

		if (mappedX < 0)
			mappedX = 99;

		if (mappedY < 0)
			mappedY = 99;

		if (mappedY > 99)
			mappedY = 99;

	}

}

void FMovingObject::IncreaseY() {

	y++;

	int val = 0;
	if (swapX) {
		mappedX++;
		val = mappedX;
	}
	else {
		mappedY++;
		val = mappedY;
	}

	std::array<int, 2> offset;

	if (val >= actor->resolution) {

		if (turnI == 1) {

			int tmp = mappedX;

			mappedX = mappedY;//std::abs(actor->resolution - mappedY) - 1;//mappedY;			
			mappedY = tmp;

			swapX = !swapX;

		}

		// turn 180 degrees
		else if (turnI == 2) {
			mappedX = std::abs(actor->resolution - mappedX);
			mappedY = std::abs(actor->resolution - mappedY);

		}

		// turn 90 degrees right
		else if (turnI == 3) {

			int tmp = std::abs(mappedX - actor->resolution) - 1;
			mappedX = mappedY;
			mappedY = tmp;

			//swapY = !swapY;
			swapX = !swapX;

		}


		offset = actor->GetCubeSideOffset(mappedX, mappedY, currSide);

		//lastSide = currSide;
		currSide = offset[0];
		turnI = offset[1];


		cubeSideList.Add(offset);

		if (mappedX > 99)
			mappedX = 0;

		if (mappedX < 0)
			mappedX = 0;

		if (mappedY < 0)
			mappedY = 0;

		if (mappedY > 99)
			mappedY = 0;

	}
}

void FMovingObject::DecreaseY() {
	y--;
	int val = 0;
	if (swapX) {
		mappedX--;
		val = mappedX;
	}
	else {
		mappedY--;
		val = mappedY;
	}

	//mappedX--;
	std::array<int, 2> offset;

	//if (mappedX < 0) {
	if (val < 0) {

		if (turnI == 1) {
			// TODO hier der Fehler mit der Indexierung!
			int tmp = mappedX;
			//mappedX = std::abs(actor->resolution - mappedY) - 1;
			//mappedX = actor->resolution - mappedY;
			mappedX = mappedY;
			mappedY = tmp;

			//swapY = !swapY;
			swapX = !swapX;
		}

		// turn 180 degrees
		if (turnI == 2) {
			mappedX = std::abs(actor->resolution - mappedX);
			mappedY = std::abs(actor->resolution - mappedY);

		}

		// turn 90 degrees right
		if (turnI == 3) {
			//int tmp = std::abs(mappedX - actor->resolution) - 1;//mappedX; //std::abs(actor->resolution - mappedX);			
			int tmp = std::abs(mappedX - actor->resolution) - 1;//mappedX; //std::abs(actor->resolution - mappedX);			
			//int tmp = mappedX; //std::abs(actor->resolution - mappedX);			
			mappedX = mappedY;
			mappedY = tmp;

			//swapY = !swapY;
			swapX = !swapX;

		}


		offset = actor->GetCubeSideOffset(mappedX, mappedY, currSide);
		//lastSide = currSide;

		currSide = offset[0];
		turnI = offset[1];

		cubeSideList.Add(offset);
		//cubeSideList.Insert(offset,0);

		//mappedX = mappedX % actor->resolution;

		if (mappedX > 99)
			mappedX = 99;

		if (mappedX < 0)
			mappedX = 99;

		if (mappedY < 0)
			mappedY = 99;

		if (mappedY > 99)
			mappedY = 99;

	}

}
template<class T>
int FMovingObject::MapCoordsRelative(TArray<T>& arr, int posX, int posY, int resolution) {

	int mappedIndex = 0;

	int indexX = x % resolution, indexY = y % resolution;
	if (indexX < 0)
		indexX = resolution + (indexX % (resolution));//(((x) % (resolution + 1)));

	if (indexY < 0)
		indexY = resolution + (indexY % (resolution));
	//int indexX = resolution - mappedX, indexY = std::abs(y % resolution);
	int sideL = cubeSideList.Last()[0];
	int sideF = cubeSideList[0][0];

	int newIndex = (resolution * resolution * sideL) + indexX * resolution + indexY;//actor->BorderlessIndexMap(arr, indexX, indexY, resolution, resolution, sideF);
	indexX = (newIndex - sideL * (resolution * resolution)) / resolution;
	indexY = (newIndex - sideL * (resolution * resolution)) % resolution;

	//indexX = mappedX;
	//indexY = mappedY;

	//for (auto i : cubeSideList) {
	for (int i = cubeSideList.Num() - 1; i >= 0; i--) {
		//for (int i = 0; i < cubeSideList.Num(); i++) {

		auto newSide = cubeSideList[i];

		//auto offset = actor->GetCubeSideOffset(posX, posY, newSide);
		int turn = newSide[1];

		// turn 90 degrees left
		if (turn == 1) {
			int tmp = indexX;
			indexX = std::abs(indexY - resolution) - 1;
			indexY = tmp;
		}

		// turn 180 degrees
		if (turn == 2) {
			indexX = std::abs(resolution - indexX) - 1;
			indexY = std::abs(resolution - indexY) - 1;

		}

		// turn 90 degrees right
		if (turn == 3) {
			int tmp = std::abs(resolution - indexX) - 1;
			indexX = indexY;
			indexY = tmp;
		}
		//mappedIndex = actor->BorderlessIndexMap(arr, indexX, indexY, resolution, resolution, newSide[0]);
	}

	return (resolution * resolution * sideL) + indexX * resolution + indexY;//mappedIndex;
	//return (resolution * resolution * sideL) + mappedX * resolution + mappedY;//mappedIndex;
}

void AMyActor::TestCity(TArray<FLinearColor>& colorArray, int cityID, int fieldsToPaint, FCityData& cityData) {

	/*if (l == 0) {
		cityData.IncreaseX();
		if (cityData.x >= cityData.initX + 100)
			l++;
	}
	if (l == 1) {
		cityData.IncreaseY();
		if (cityData.y >= cityData.initY + 100)
			l++;
	}
	if (l == 2) {
		cityData.DecreaseX();
		if (cityData.x <= cityData.initX)
			l++;
	}
	if (l == 3) {
		cityData.DecreaseY();
		if (cityData.y <= cityData.initY)
			l=0;
	}*/

	//cityData.IncreaseX();
	//cityData.IncreaseY();
	//cityData.DecreaseY();

	int val2 = cityData.MapCoordsRelative(fieldDataArray, cityData.x, cityData.y, resolution);

	colorArray[val2] = cityData.color;
	fieldDataArray[val2].terrainClass = ETerrainClass::LandCity;
	fieldDataArray[val2].cityData = cityData;
}

void AMyActor::GrowCity(TArray<FLinearColor>& colorArray, int cityID, int fieldsToPaint, FCityData& cityData) {

	int currentSide = CurrentSideFromIndex(cityData.lastSetFieldPos);
	int stepsDone = 0;

	for (int i = fieldsToPaint; i > 0; ) {
		// down left		
		if (cityData.lastDirection == 0) {
			//cityData.posX++;
			cityData.IncreaseX();

			if (PlaceCityField(colorArray, cityData, cityData.currSide)) {

				if (i % 3 == 0) {
					int position = (resolution * resolution * cityData.currSide) + cityData.x * resolution + cityData.y;
					if (position < 60000) {

						houseBuildQueue.Enqueue(BorderlessIndexMap(fieldDataArray, cityData.x, cityData.y, resolution, resolution, cityData.currSide));
					}
				}
				i--;
			}

			if (cityData.x >= cityData.initX + cityData.lastRad) {
				cityData.lastDirection = 1;
			}

		}

		// down right
		if (cityData.lastDirection == 1) {
			//cityData.posY--;
			cityData.DecreaseY();

			if (PlaceCityField(colorArray, cityData, cityData.currSide)) {

				if (i % 23 == 0) {

					if ((resolution * resolution * cityData.currSide) + cityData.x * resolution + cityData.y < 60000) {
						houseBuildQueue.Enqueue(BorderlessIndexMap(fieldDataArray, cityData.x, cityData.y, resolution, resolution, cityData.currSide));
					}
				}
				i--;
			}

			if (cityData.y <= cityData.initY - cityData.lastRad) {
				cityData.lastDirection = 2;
			}
		}

		// up right
		if (cityData.lastDirection == 2) {
			//cityData.posX--;
			cityData.DecreaseX();
			//cityData.pos = cityData.MapCoordsRelative(fieldDataArray, cityData.x, cityData.y, resolution);
			if (PlaceCityField(colorArray, cityData, cityData.currSide)) {

				if (i % 3 == 0) {

					if ((resolution * resolution * cityData.currSide) + cityData.x * resolution + cityData.y < 60000) {
						houseBuildQueue.Enqueue(BorderlessIndexMap(fieldDataArray, cityData.x, cityData.y, resolution, resolution, cityData.currSide));
					}
				}
				i--;
			}

			if (cityData.x <= cityData.initX - cityData.lastRad) {
				cityData.lastDirection = 3;
			}
		}

		// up left
		if (cityData.lastDirection == 3) {
			//cityData.posY++;
			cityData.IncreaseY();
			//cityData.pos = cityData.MapCoordsRelative(fieldDataArray, cityData.x, cityData.y, resolution);
			if (PlaceCityField(colorArray, cityData, cityData.currSide)) {

				if (i % 23 == 0) {

					if ((resolution * resolution * cityData.currSide) + cityData.x * resolution + cityData.y < 60000) {
						houseBuildQueue.Enqueue(BorderlessIndexMap(fieldDataArray, cityData.x, cityData.y, resolution, resolution, cityData.currSide));
					}
				}
				i--;
			}

			if (cityData.y >= cityData.initY + cityData.lastRad)
			{
				cityData.lastDirection = 0;
				cityData.lastRad++;

				if (i == fieldsToPaint || stepsDone >= 4 * cityData.x * cityData.y)
					return;

			}

			stepsDone++;
		}
	}

}

void AMyActor::GrowCityAmount(int cityID, int amount) {

	for (auto& elem : cityDataArr) {
		if (elem.ID == cityID) {
			elem.cityGrowIndex = amount;
			return;
		}
	}
}

void AMyActor::CreateCyclone(int spawnLocation, FVector2D direction, int moveRange) {

	cycloneDataQueue.Empty();

	FCycloneData cyclone = FCycloneData(spawnLocation, direction, moveRange);
	cyclone.currSide = CurrentSideFromIndex(spawnLocation);
	cyclone.actor = this;

	cyclone.initX = (spawnLocation - cyclone.currSide * (resolution * resolution)) / resolution;
	cyclone.x = (spawnLocation - cyclone.currSide * (resolution * resolution)) / resolution;
	cyclone.initY = (spawnLocation - cyclone.currSide * (resolution * resolution)) % resolution;
	cyclone.y = (spawnLocation - cyclone.currSide * (resolution * resolution)) % resolution;

	cyclone.mappedX = cyclone.x;
	cyclone.mappedY = cyclone.y;

	cyclone.turnI = GetCubeSideOffset(cyclone.x, cyclone.y, cyclone.currSide)[1];
	cyclone.cubeSideList.Add(GetCubeSideOffset(cyclone.x, cyclone.y, cyclone.currSide));

	cyclone.actorObj = GetWorld()->SpawnActor<AActor>(HouseObjects[25], fieldDataArray[spawnLocation].position, fieldDataArray[spawnLocation].rotation, FActorSpawnParameters());
	cyclone.actorObj->AttachToActor(this, FAttachmentTransformRules::KeepRelativeTransform);

	cycloneDataQueue.Enqueue(cyclone);

}

void AMyActor::CreateFlood(int moveLength, int size) {
	srand(static_cast <unsigned> (time(0)));
	int spawnLocation = 100;//rand() % borderArr.Num();

	FFloodData floodData = FFloodData(borderArr[spawnLocation], moveLength, size);

	floodData.currSide = CurrentSideFromIndex(borderArr[spawnLocation]);

	floodData.x = (borderArr[spawnLocation] - floodData.currSide * (resolution * resolution)) / resolution;
	floodData.y = (borderArr[spawnLocation] - floodData.currSide * (resolution * resolution)) % resolution;

	floodDataQueue.Enqueue(floodData);
}

void AMyActor::ExtendFlood(FFloodData& floodObj) {

	for (int i = 0; i < floodObj.moveLength; i++) {

		int x = ((floodObj.spawnPosition + i) - floodObj.currSide * (resolution * resolution)) / resolution;
		int y = ((floodObj.spawnPosition + i) - floodObj.currSide * (resolution * resolution)) % resolution;

		for (int j = -floodObj.size - 1; j <= floodObj.size + 1; j++) {
			for (int k = -floodObj.size - 1; k <= floodObj.size + 1; k++) {
				if (BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).terrainClass != ETerrainClass::Mountain &&
					BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).terrainClass != ETerrainClass::Water) {

					if (BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).terrainClass == ETerrainClass::LandCity) {
						actorRemoveQueue.Enqueue(BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).cityData.actorObj);
						BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).cityData.cityFields.Remove(BorderlessIndexMap(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide));
					}

					BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).terrainClass = ETerrainClass::Water;
					BorderlessArrayAccess(VertexColors, x + j, y + k, resolution, resolution, floodObj.currSide) = FLinearColor::Blue;
					floodObj.fields.Add(BorderlessIndexMap(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide));
				}
			}
		}

	}

	floodObj.size++;
}

void AMyActor::ReduceFlood(FFloodData& floodObj) {

	for (int index = 0; index < floodObj.moveLength; index++) {

		int x = ((floodObj.spawnPosition + index) - floodObj.currSide * (resolution * resolution)) / resolution;
		int y = ((floodObj.spawnPosition + index) - floodObj.currSide * (resolution * resolution)) % resolution;

		for (int j = -floodObj.maxSize - 1; j <= floodObj.maxSize + 1; j++) {
			for (int k = -floodObj.maxSize - 1; k <= floodObj.maxSize + 1; k++) {
				if ((BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).terrainClass != ETerrainClass::Mountain) &&
					(BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).terrainClass != ETerrainClass::Water)) {



					BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).terrainClass = ETerrainClass::LandNoCity;
					BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).cityData.ID = CITY_ID_NO_CITY;
					int i = BorderlessIndexMap(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide);

					if (polyArr[i] % 8 == 0)
						VertexColors[i] = FLinearColor::Red;
					if (polyArr[i] % 8 == 1)
						VertexColors[i] = FLinearColor::Green;
					if (polyArr[i] % 8 == 2)
						VertexColors[i] = FLinearColor::Black;
					if (polyArr[i] % 8 == 3)
						VertexColors[i] = FLinearColor::Yellow;
					if (polyArr[i] % 8 == 4)
						VertexColors[i] = FLinearColor(1.0f, 1.0f, 0.0f);
					if (polyArr[i] % 8 == 5)
						VertexColors[i] = FLinearColor::White;
					if (polyArr[i] % 8 == 6)
						VertexColors[i] = FLinearColor(0.0f, 1.0f, 1.0f);
					if (polyArr[i] % 8 == 7)
						VertexColors[i] = FLinearColor(1.0f, 0.0f, 1.0f);

				}
			}
		}

	}

	for (int i = 0; i < floodObj.moveLength; i++) {

		int x = ((floodObj.spawnPosition + i) - floodObj.currSide * (resolution * resolution)) / resolution;
		int y = ((floodObj.spawnPosition + i) - floodObj.currSide * (resolution * resolution)) % resolution;

		for (int j = -floodObj.size; j <= floodObj.size; j++) {
			for (int k = -floodObj.size; k <= floodObj.size; k++) {
				if ((BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).terrainClass != ETerrainClass::Mountain)) {
					BorderlessArrayAccess(VertexColors, x + j, y + k, resolution, resolution, floodObj.currSide) = FLinearColor::Blue;

					if (floodObj.fields.Find(BorderlessIndexMap(VertexColors, x + j, y + k, resolution, resolution, floodObj.currSide)) != INDEX_NONE) {
						BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).terrainClass = ETerrainClass::LandNoCity;
					}
					else {

						BorderlessArrayAccess(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide).terrainClass = ETerrainClass::Water;
					}
					//floodObj.fields.Add(BorderlessIndexMap(fieldDataArray, x + j, y + k, resolution, resolution, floodObj.currSide));
				}
			}
		}

	}

	floodObj.size--;
}

void AMyActor::MoveCyclone(FCycloneData& elem) {

	if (elem.direction == FVector2D(1, 0)) {
		elem.IncreaseX();
	}

	if (elem.direction == FVector2D(-1, 0)) {
		elem.DecreaseX();
	}

	if (elem.direction == FVector2D(0, 1)) {
		elem.IncreaseY();
	}

	if (elem.direction == FVector2D(0, -1)) {
		elem.DecreaseY();
	}

	int val = elem.MapCoordsRelative(fieldDataArray, elem.x, elem.y, resolution);
	elem.actorLocation = fieldDataArray[val].position;
	elem.actorRotation = fieldDataArray[val].rotation;

	if (fieldDataArray[val].terrainClass == ETerrainClass::LandCity) {
		fieldDataArray[val].houseData = EHouseData::Empty;
		fieldDataArray[val].terrainClass = ETerrainClass::LandNoCity;
		fieldDataArray[val].cityData.cityFields.Remove(val);
		fieldDataArray[val].cityData = FCityData();
		fieldDataArray[val].cityData.ID = CITY_ID_NO_CITY;
		VertexColors[val] = FLinearColor::Black;
	}

}

int AMyActor::HightlightContinent(int lastIndex) {

	clickIndex = CPUClickDetection();

	for (int i = 0; i < 6; i++) {
		for (int x = 0; x < resolution; x++) {
			for (int y = 0; y < resolution; y++) {
				auto old = polyArr[lastIndex];
				auto newI = polyArr[clickIndex];
				if (polyArr[i * resolution * resolution + (x * resolution + y)] == polyArr[lastIndex] && old != newI && polyArr[i * resolution * resolution + (x * resolution + y)] != -1) {
					VertexColors[i * resolution * resolution + (x * resolution + y)] -= FLinearColor(0.5f, 0.5f, 0.5f);
				}

			}
		}
	}

	for (int i = 0; i < 6; i++) {
		for (int x = 0; x < resolution; x++) {
			for (int y = 0; y < resolution; y++) {
				auto old = polyArr[lastIndex];
				auto newI = polyArr[clickIndex];
				if (polyArr[i * resolution * resolution + (x * resolution + y)] == polyArr[clickIndex]
					&& polyArr[i * resolution * resolution + (x * resolution + y)] != polyArr[lastIndex] && old != newI && polyArr[i * resolution * resolution + (x * resolution + y)] != -1) {
					VertexColors[i * resolution * resolution + (x * resolution + y)] += FLinearColor(0.5f, 0.5f, 0.5f);

				}
			}
		}
	}

	//lastIndex = clickIndex;
	return clickIndex;
	//}
}

FMyWorker::FMyWorker()
{

}

FMyWorker::~FMyWorker()
{

}

#pragma endregion
// The code below will run on the new thread.

bool FMyWorker::Init()
{
	UE_LOG(LogTemp, Warning, TEXT("My custom thread has been initialized"));
	// Return false if you want to abort the thread
	return true;
}


uint32 FMyWorker::Run()
{
	while (bRunThread)
	{
		actor->clickIndex = actor->HightlightContinent(actor->clickIndex);
	}

	return 0;
}

void FMyWorker::Stop()
{
	bRunThread = false;
}

FManageMoveables::FManageMoveables()
{

}

FManageMoveables::~FManageMoveables()
{

}

#pragma endregion
// The code below will run on the new thread.

bool FManageMoveables::Init()
{
	UE_LOG(LogTemp, Warning, TEXT("My custom thread has been initialized"));
	// Return false if you want to abort the thread
	return true;
}


uint32 FManageMoveables::Run()
{
	int i = 0;
	int timer = 0;
	while (bRunThread)
	{
		i++;

		while (!actor->cityDataQueue.IsEmpty()) {

			actor->cityDataArr.Add(*actor->cityDataQueue.Peek());
			actor->cityDataQueue.Pop();

		}

		while (!actor->cycloneDataQueue.IsEmpty()) {

			actor->cycloneDataArr.Add(*actor->cycloneDataQueue.Peek());
			actor->cycloneDataQueue.Pop();

		}

		while (!actor->floodDataQueue.IsEmpty()) {

			actor->floodDataArr.Add(*actor->floodDataQueue.Peek());
			actor->floodDataQueue.Pop();

		}

		for (FCityData& elem : actor->cityDataArr) {
			actor->GrowCity(actor->VertexColors, elem.ID, elem.cityGrowIndex, elem);
			elem.cityGrowIndex = 0;
		}

		TArray<int> removeIndex;
		int ind = 0;

		for (FCycloneData& elem : actor->cycloneDataArr) {
			actor->MoveCyclone(elem);
			elem.moveRange--;
			if (elem.moveRange <= 0)
				removeIndex.Add(ind);

			ind++;
		}


		for (int index : removeIndex) {
			actor->actorRemoveQueue.Enqueue(actor->cycloneDataArr[index].actorObj);
			actor->cycloneDataArr.RemoveAt(index);
		}

		removeIndex.Empty();
		ind = 0;

		for (FFloodData& elem : actor->floodDataArr) {

			if (timer < 10) {
				timer++;
				break;
			}
			else {

				timer = 0;

				if (!elem.reduce)
					actor->ExtendFlood(elem);
				if ((elem.size == elem.maxSize + 1)) {
					elem.reduce = true;
				}
				if (elem.reduce) {
					actor->ReduceFlood(elem);
				}
				if (elem.size < -1) {
					removeIndex.Add(ind);
				}

				ind++;
			}
		}

		for (int index : removeIndex) {
			actor->floodDataArr.RemoveAt(index);
		}

		//for (FCityData& elem : actor->cityDataArr)
			//	actor->TestCity(actor->VertexColors, elem.ID, 15, elem);


		//FPlatformProcess::Sleep(.3);
		FPlatformProcess::Sleep(.1);

	}

	return 0;
}


// This function is NOT run on the new thread!
void FManageMoveables::Stop()
{
	bRunThread = false;
}

FInterfaceUpdate::FInterfaceUpdate() {}
FInterfaceUpdate::~FInterfaceUpdate() {}

bool FInterfaceUpdate::Init() {
	return true;
}

uint32 FInterfaceUpdate::Run() {
	while (bRunThread) {

		actor->globalNonUsableCellCount = actor->GlobalNonUsableCellCount();
		actor->globalUsableCellCount = actor->GlobalUsableCellCount();		

		UE_LOG(LogTemp, Warning, TEXT("UsableCells %i"), actor->globalUsableCellCount);

		for (auto elem : actor->continentIndices) {
			actor->continentNonUsableCellCount[elem.begin()->first] = actor->ContinentNonUsableCellCount(elem.begin()->first);
			actor->continentUsableCellCount[elem.begin()->first] = actor->ContinentUsableCellCount(elem.begin()->first);
		}

		FPlatformProcess::Sleep(.1);
	}

	return 0;
}

void FInterfaceUpdate::Stop() {
	bRunThread = false;
}